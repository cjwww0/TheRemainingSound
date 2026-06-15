#include "Events/RemainNurseCorridorEncounterActor.h"

#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Containers/ScriptArray.h"
#include "Debug/RemainInventoryDebugLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/RemainNurseEncounterActor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Puzzle/RemainWorkbenchPartIdProvider.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	static void AddNurseCorridorPartIdFromObject(const UObject* Object, TArray<FName>& OutPartIds)
	{
		if (!IsValid(Object))
		{
			return;
		}

		const UClass* ObjectClass = Object->GetClass();
		if (!ObjectClass)
		{
			return;
		}

		if (ObjectClass->ImplementsInterface(URemainWorkbenchPartIdProvider::StaticClass()))
		{
			const FName PartId = IRemainWorkbenchPartIdProvider::Execute_GetWorkbenchPartId(const_cast<UObject*>(Object));
			if (!PartId.IsNone())
			{
				OutPartIds.AddUnique(PartId);
			}
		}

		if (const FNameProperty* PartIdProperty = FindFProperty<FNameProperty>(ObjectClass, TEXT("PartId")))
		{
			const FName PartId = PartIdProperty->GetPropertyValue_InContainer(Object);
			if (!PartId.IsNone())
			{
				OutPartIds.AddUnique(PartId);
			}
		}
	}

	static void AddNurseCorridorPartIdFromClass(const UClass* Class, TArray<FName>& OutPartIds)
	{
		if (Class)
		{
			AddNurseCorridorPartIdFromObject(Class->GetDefaultObject(), OutPartIds);
		}
	}

	static TArray<FName> ResolveNurseCorridorPartIds(const UObject* Object)
	{
		TArray<FName> Result;
		if (!IsValid(Object))
		{
			return Result;
		}

		AddNurseCorridorPartIdFromObject(Object, Result);
		AddNurseCorridorPartIdFromClass(Object->GetClass(), Result);

		const UClass* ObjectClass = Object->GetClass();
		static const FName ObjectPropertyCandidates[] =
		{
			TEXT("ItemInstanceRef"),
			TEXT("InventoryItemRef"),
			TEXT("ExaminableItem")
		};

		for (const FName PropertyName : ObjectPropertyCandidates)
		{
			if (const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(ObjectClass, PropertyName))
			{
				AddNurseCorridorPartIdFromObject(ObjectProperty->GetObjectPropertyValue_InContainer(Object), Result);
			}
		}

		static const FName ClassPropertyCandidates[] =
		{
			TEXT("InventoryItemClass"),
			TEXT("ItemClass")
		};

		for (const FName PropertyName : ClassPropertyCandidates)
		{
			if (const FClassProperty* ClassProperty = FindFProperty<FClassProperty>(ObjectClass, PropertyName))
			{
				AddNurseCorridorPartIdFromClass(Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Object)), Result);
			}
		}

		return Result;
	}

	static TArray<UObject*> GetNurseCorridorInventoryItems(UObject* Inventory)
	{
		TArray<UObject*> Result;
		if (!IsValid(Inventory))
		{
			return Result;
		}

		FArrayProperty* InventoryItemsProperty = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("InventoryItems"));
		if (!InventoryItemsProperty || !InventoryItemsProperty->Inner)
		{
			return Result;
		}

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(InventoryItemsProperty->Inner);
		if (!ObjectProperty)
		{
			return Result;
		}

		FScriptArrayHelper Helper(InventoryItemsProperty, InventoryItemsProperty->ContainerPtrToValuePtr<void>(Inventory));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			if (UObject* Item = ObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)))
			{
				Result.Add(Item);
			}
		}

		return Result;
	}
}

ARemainNurseCorridorEncounterActor::ARemainNurseCorridorEncounterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(120.0f, 120.0f, 120.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(true);
}

void ARemainNurseCorridorEncounterActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureTriggerBox();
	DebugMessage(FString::Printf(
		TEXT("BeginPlay | Require=%s | RequiredPart=%s | Nurse=%s | StrongLights=%d"),
		bRequirePickupBeforeTrigger ? TEXT("true") : TEXT("false"),
		*RequiredPickupPartId.ToString(),
		*GetNameSafe(NurseEncounter),
		StrongLightActors.Num()));

	HideNurseAndLights();
}

void ARemainNurseCorridorEncounterActor::ConfigureTriggerBox()
{
	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
		TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		TriggerBox->SetGenerateOverlapEvents(true);
		TriggerBox->SetHiddenInGame(true);
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &ARemainNurseCorridorEncounterActor::HandleTriggerBeginOverlap);
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ARemainNurseCorridorEncounterActor::HandleTriggerBeginOverlap);
		TriggerBox->UpdateOverlaps();
	}
}

bool ARemainNurseCorridorEncounterActor::TriggerCorridorEncounter()
{
	if (bTriggered)
	{
		DebugMessage(TEXT("Trigger blocked: already triggered"));
		return false;
	}

	if (!HasRequiredPickup())
	{
		DebugMessage(FString::Printf(
			TEXT("Trigger blocked: missing required pickup %s"),
			*RequiredPickupPartId.ToString()));
		return false;
	}

	DebugMessage(TEXT("Trigger accepted: starting corridor encounter"));
	bTriggered = true;
	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorLightsEnabled(FlickerLightActors, false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SilenceTimerHandle,
			this,
			&ARemainNurseCorridorEncounterActor::ShowNurseAndStrongLight,
			SilenceDuration,
			false);
	}
	else
	{
		ShowNurseAndStrongLight();
	}

	return true;
}

void ARemainNurseCorridorEncounterActor::ResetCorridorEncounter()
{
	bTriggered = false;
	bRequiredPickupCollected = false;
	ConfigureTriggerBox();

	HideNurseAndLights();
}

bool ARemainNurseCorridorEncounterActor::NotifyWorkbenchPickupCollected(AActor* PickupActor)
{
	return NotifyCollectedObject(PickupActor);
}

bool ARemainNurseCorridorEncounterActor::NotifyCollectedObject(UObject* CollectedObject)
{
	if (bTriggered || bRequiredPickupCollected || !bRequirePickupBeforeTrigger)
	{
		DebugMessage(FString::Printf(
			TEXT("Pickup notify ignored | Triggered=%s | AlreadyCollected=%s | Require=%s"),
			bTriggered ? TEXT("true") : TEXT("false"),
			bRequiredPickupCollected ? TEXT("true") : TEXT("false"),
			bRequirePickupBeforeTrigger ? TEXT("true") : TEXT("false")));
		return false;
	}

	if (!DoesObjectMatchRequiredPart(CollectedObject))
	{
		DebugMessage(FString::Printf(
			TEXT("Pickup notify did not match %s | Object=%s"),
			*RequiredPickupPartId.ToString(),
			*GetNameSafe(CollectedObject)));
		return false;
	}

	bRequiredPickupCollected = true;
	DebugMessage(FString::Printf(
		TEXT("Required pickup collected: %s | Object=%s"),
		*RequiredPickupPartId.ToString(),
		*GetNameSafe(CollectedObject)));
	return true;
}

void ARemainNurseCorridorEncounterActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	DebugMessage(FString::Printf(
		TEXT("Overlap | Other=%s | IsPlayer=%s | HasRequiredPickup=%s"),
		*GetNameSafe(OtherActor),
		IsPlayerActor(OtherActor) ? TEXT("true") : TEXT("false"),
		HasRequiredPickup() ? TEXT("true") : TEXT("false")));

	if (IsPlayerActor(OtherActor))
	{
		TriggerCorridorEncounter();
	}
}

void ARemainNurseCorridorEncounterActor::ShowNurseAndStrongLight()
{
	SetActorLightsEnabled(StrongLightActors, false);
	StrongLightFlashStep = 0;
	bStrongLightFlashVisible = false;

	if (StrongLightSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StrongLightSound, GetActorLocation());
	}

	if (NurseEncounter)
	{
		NurseEncounter->ActivateEncounter();
	}
	else
	{
		SetActorVisible(NurseActorFallback, true);
	}

	const int32 FlashSteps = FMath::Max(0, StrongLightFlashCount) * 2;
	if (FlashSteps > 0)
	{
		StepStrongLightFlash();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				StrongLightFlashTimerHandle,
				this,
				&ARemainNurseCorridorEncounterActor::StepStrongLightFlash,
				FMath::Max(0.02f, StrongLightFlashInterval),
				true);
		}
	}
	else
	{
		SetActorLightsEnabled(StrongLightActors, true);
	}

	if (UWorld* World = GetWorld())
	{
		const float FlashDuration = FlashSteps > 0 ? FlashSteps * FMath::Max(0.02f, StrongLightFlashInterval) : 0.0f;
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&ARemainNurseCorridorEncounterActor::HideNurseAndLights,
			FlashDuration + NurseVisibleDuration,
			false);
	}
}

void ARemainNurseCorridorEncounterActor::StepStrongLightFlash()
{
	const int32 FlashSteps = FMath::Max(0, StrongLightFlashCount) * 2;
	if (StrongLightFlashStep >= FlashSteps)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StrongLightFlashTimerHandle);
		}

		SetActorLightsEnabled(StrongLightActors, bHoldStrongLightAfterFlash);
		DebugMessage(FString::Printf(
			TEXT("Strong light flicker finished | Hold=%s"),
			bHoldStrongLightAfterFlash ? TEXT("true") : TEXT("false")));
		return;
	}

	bStrongLightFlashVisible = !bStrongLightFlashVisible;
	SetActorLightsEnabled(StrongLightActors, bStrongLightFlashVisible);
	++StrongLightFlashStep;
}

void ARemainNurseCorridorEncounterActor::HideNurseAndLights()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrongLightFlashTimerHandle);
	}
	StrongLightFlashStep = 0;
	bStrongLightFlashVisible = false;

	SetActorLightsEnabled(StrongLightActors, false);

	if (NurseEncounter)
	{
		NurseEncounter->EndEncounter();
	}
	else
	{
		SetActorVisible(NurseActorFallback, false);
	}

	if (bRestoreFlickerLightsAfterEvent)
	{
		SetActorLightsEnabled(FlickerLightActors, true);
	}
}

void ARemainNurseCorridorEncounterActor::SetActorLightsEnabled(const TArray<TObjectPtr<AActor>>& Actors, bool bEnabled) const
{
	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<ULightComponent*> LightComponents;
		Actor->GetComponents(LightComponents);
		for (ULightComponent* LightComponent : LightComponents)
		{
			if (LightComponent)
			{
				LightComponent->SetVisibility(bEnabled, true);
			}
		}

		Actor->SetActorHiddenInGame(!bEnabled);
	}
}

void ARemainNurseCorridorEncounterActor::SetActorVisible(AActor* Actor, bool bVisible) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorHiddenInGame(!bVisible);
	Actor->SetActorEnableCollision(bVisible);
}

void ARemainNurseCorridorEncounterActor::DebugMessage(const FString& Message) const
{
	if (!bDebugCorridorEncounter)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[NurseCorridor] %s: %s"), *GetName(), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(TEXT("NurseCorridor: %s"), *Message));
	}
}

bool ARemainNurseCorridorEncounterActor::IsPlayerActor(const AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (Actor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return true;
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}

bool ARemainNurseCorridorEncounterActor::HasRequiredPickup() const
{
	return !bRequirePickupBeforeTrigger
		|| bRequiredPickupCollected
		|| RequiredPickupPartId.IsNone()
		|| RuntimeInventoryHasRequiredPickup();
}

bool ARemainNurseCorridorEncounterActor::DoesObjectMatchRequiredPart(const UObject* CollectedObject) const
{
	if (!IsValid(CollectedObject) || RequiredPickupPartId.IsNone())
	{
		return false;
	}

	for (const FName PartId : ResolveNurseCorridorPartIds(CollectedObject))
	{
		if (PartId == RequiredPickupPartId)
		{
			return true;
		}
	}

	return false;
}

bool ARemainNurseCorridorEncounterActor::RuntimeInventoryHasRequiredPickup() const
{
	if (!bRequirePickupBeforeTrigger || bRequiredPickupCollected || RequiredPickupPartId.IsNone())
	{
		return true;
	}

	bool bValidInventory = false;
	UObject* Inventory = URemainInventoryDebugLibrary::GetRuntimeInventory(const_cast<ARemainNurseCorridorEncounterActor*>(this), bValidInventory);
	if (!bValidInventory || !IsValid(Inventory))
	{
		DebugMessage(TEXT("Inventory scan failed: runtime inventory invalid"));
		return false;
	}

	const TArray<UObject*> Items = GetNurseCorridorInventoryItems(Inventory);
	TArray<FString> SeenPartIds;
	for (UObject* Item : Items)
	{
		const TArray<FName> PartIds = ResolveNurseCorridorPartIds(Item);
		for (const FName PartId : PartIds)
		{
			SeenPartIds.AddUnique(PartId.ToString());
			if (PartId == RequiredPickupPartId)
			{
				bRequiredPickupCollected = true;
				DebugMessage(FString::Printf(
					TEXT("Inventory scan matched required pickup %s | Item=%s | Count=%d"),
					*RequiredPickupPartId.ToString(),
					*GetNameSafe(Item),
					Items.Num()));
				return true;
			}
		}
	}

	DebugMessage(FString::Printf(
		TEXT("Inventory scan missing %s | Count=%d | SeenPartIds=%s"),
		*RequiredPickupPartId.ToString(),
		Items.Num(),
		SeenPartIds.Num() > 0 ? *FString::Join(SeenPartIds, TEXT(",")) : TEXT("None")));
	return false;
}
