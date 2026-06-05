#include "Events/RemainPickupShockController.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/RemainBreakableSwapComponent.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Debug/RemainInventoryDebugLibrary.h"
#include "Events/RemainNurseEncounterActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Puzzle/RemainWorkbenchPartIdProvider.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ScriptDelegates.h"
#include "UObject/UnrealType.h"

ARemainPickupShockController::ARemainPickupShockController()
{
	PrimaryActorTick.bCanEverTick = false;
	BreakableCup = CreateDefaultSubobject<URemainBreakableSwapComponent>(TEXT("BreakableCup"));
}

void ARemainPickupShockController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoBindInventoryItemAdded)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ARemainPickupShockController::RetryBindInventoryItemAdded));
		}
	}
}

namespace
{
	static APlayerController* ResolvePlayerController(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			return World->GetFirstPlayerController();
		}

		return nullptr;
	}

	static UCameraComponent* ResolvePlayerCameraComponent(APlayerController* PlayerController)
	{
		if (!IsValid(PlayerController))
		{
			return nullptr;
		}

		APawn* Pawn = PlayerController->GetPawn();
		if (!IsValid(Pawn))
		{
			return nullptr;
		}

		for (UActorComponent* Component : Pawn->GetComponents())
		{
			if (UCameraComponent* CameraComponent = Cast<UCameraComponent>(Component))
			{
				if (CameraComponent->GetName().Contains(TEXT("FirstPersonCamera")))
				{
					return CameraComponent;
				}
			}
		}

		return Pawn->FindComponentByClass<UCameraComponent>();
	}

	static void AddPartIdFromObject(const UObject* Object, TArray<FName>& OutPartIds)
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

	static void AddPartIdFromClass(const UClass* Class, TArray<FName>& OutPartIds)
	{
		if (!Class)
		{
			return;
		}

		if (const UObject* DefaultObject = Class->GetDefaultObject())
		{
			AddPartIdFromObject(DefaultObject, OutPartIds);
		}
	}

	static void CollectPartIdsRecursive(const UObject* Object, TArray<FName>& OutPartIds, TSet<const UObject*>& VisitedObjects, TSet<const UClass*>& VisitedClasses)
	{
		if (!IsValid(Object) || VisitedObjects.Contains(Object))
		{
			return;
		}

		VisitedObjects.Add(Object);
		AddPartIdFromObject(Object, OutPartIds);

		const UClass* ObjectClass = Object->GetClass();
		if (!ObjectClass)
		{
			return;
		}

		if (!VisitedClasses.Contains(ObjectClass))
		{
			VisitedClasses.Add(ObjectClass);
			AddPartIdFromClass(ObjectClass, OutPartIds);
		}

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
				if (UObject* ReferencedObject = ObjectProperty->GetObjectPropertyValue_InContainer(Object))
				{
					CollectPartIdsRecursive(ReferencedObject, OutPartIds, VisitedObjects, VisitedClasses);
				}
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
				if (const UClass* ReferencedClass = Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Object)))
				{
					if (!VisitedClasses.Contains(ReferencedClass))
					{
						VisitedClasses.Add(ReferencedClass);
						AddPartIdFromClass(ReferencedClass, OutPartIds);
					}
				}
			}
		}
	}

	static TArray<FName> ResolvePartIds(const UObject* Object)
	{
		TArray<FName> Result;
		TSet<const UObject*> VisitedObjects;
		TSet<const UClass*> VisitedClasses;
		CollectPartIdsRecursive(Object, Result, VisitedObjects, VisitedClasses);
		return Result;
	}

	static void CollectAudioComponents(AActor* Actor, TArray<UAudioComponent*>& OutComponents)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		TInlineComponentArray<UAudioComponent*> Components;
		Actor->GetComponents(Components);
		for (UAudioComponent* Component : Components)
		{
			if (IsValid(Component))
			{
				OutComponents.AddUnique(Component);
			}
		}
	}

	static void CollectLightComponents(AActor* Actor, TArray<ULightComponent*>& OutComponents)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		TInlineComponentArray<ULightComponent*> Components;
		Actor->GetComponents(Components);
		for (ULightComponent* Component : Components)
		{
			if (IsValid(Component))
			{
				OutComponents.AddUnique(Component);
			}
		}
	}
}

bool ARemainPickupShockController::TriggerShockSequence()
{
	if (bTriggered)
	{
		return false;
	}

	bTriggered = true;
	MuteAmbientAudio();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SilenceTimerHandle);
		World->GetTimerManager().SetTimer(SilenceTimerHandle, this, &ARemainPickupShockController::HandleImpact, SilenceDuration, false);
	}
	else
	{
		HandleImpact();
	}

	return true;
}

bool ARemainPickupShockController::NotifyWorkbenchPickupCollected(AActor* PickupActor)
{
	return DoesPickupMatch(PickupActor) && TriggerShockSequence();
}

bool ARemainPickupShockController::NotifyInventoryItemAdded(UObject* InventoryItem)
{
	return DoesItemMatch(InventoryItem) && TriggerShockSequence();
}

bool ARemainPickupShockController::HasTriggered() const
{
	return bTriggered;
}

void ARemainPickupShockController::RetryBindInventoryItemAdded()
{
	if (TryBindInventoryItemAdded())
	{
		return;
	}

	++InventoryBindAttemptCount;
	if (MaxInventoryBindAttempts > 0 && InventoryBindAttemptCount >= MaxInventoryBindAttempts)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InventoryBindRetryTimerHandle,
			this,
			&ARemainPickupShockController::RetryBindInventoryItemAdded,
			InventoryBindRetryInterval,
			false);
	}
}

void ARemainPickupShockController::HandleInventoryItemAdded(UObject* InventoryItem)
{
	NotifyInventoryItemAdded(InventoryItem);
}

void ARemainPickupShockController::HandleImpact()
{
	if (GlassBreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, GlassBreakSound, GetActorLocation() + GlassBreakLocationOffset);
	}

	if (BreakableCup)
	{
		BreakableCup->TriggerBreak();
	}

	ExtinguishConfiguredLights();
	StartScreenEffects();

	if (APlayerController* PlayerController = ResolvePlayerController(this))
	{
		if (CameraShakeClass && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraShake(CameraShakeClass, CameraShakeScale);
		}
	}

	OnImpactTriggered.Broadcast();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScreenFlashTimerHandle);
		World->GetTimerManager().SetTimer(ScreenFlashTimerHandle, this, &ARemainPickupShockController::FinishScreenFlash, ScreenFlashDuration, false);
	}
	else
	{
		FinishScreenFlash();
	}
}

void ARemainPickupShockController::FinishScreenFlash()
{
	RestoreScreenEffects();

	if (bRestoreAmbientAfterImpact)
	{
		RestoreAmbientAudio();
	}

	OnSequenceCompleted.Broadcast();

	if (bTriggerNurseEncounterOnSequenceCompleted && IsValid(NurseEncounter))
	{
		if (NurseEncounterDelay <= KINDA_SMALL_NUMBER)
		{
			TriggerLinkedNurseEncounter();
		}
		else if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(NurseEncounterTimerHandle);
			World->GetTimerManager().SetTimer(
				NurseEncounterTimerHandle,
				this,
				&ARemainPickupShockController::TriggerLinkedNurseEncounter,
				NurseEncounterDelay,
				false);
		}
	}
}

void ARemainPickupShockController::TriggerLinkedNurseEncounter()
{
	if (IsValid(NurseEncounter))
	{
		NurseEncounter->ActivateEncounter();
	}
}

bool ARemainPickupShockController::TryBindInventoryItemAdded()
{
	bool bInventoryValid = false;
	UObject* Inventory = URemainInventoryDebugLibrary::GetRuntimeInventory(this, bInventoryValid);
	if (!bInventoryValid || !IsValid(Inventory))
	{
		return false;
	}

	if (BoundInventory == Inventory)
	{
		return true;
	}

	for (TFieldIterator<FMulticastDelegateProperty> It(Inventory->GetClass()); It; ++It)
	{
		FMulticastDelegateProperty* DelegateProperty = *It;
		if (!DelegateProperty)
		{
			continue;
		}

		const FString DelegateName = DelegateProperty->GetName();
		if (!DelegateName.Contains(TEXT("Item")) || !DelegateName.Contains(TEXT("Added")))
		{
			continue;
		}

		if (FMulticastScriptDelegate* MulticastDelegate = DelegateProperty->ContainerPtrToValuePtr<FMulticastScriptDelegate>(Inventory))
		{
			FScriptDelegate ScriptDelegate;
			ScriptDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ARemainPickupShockController, HandleInventoryItemAdded));
			MulticastDelegate->AddUnique(ScriptDelegate);
			BoundInventory = Inventory;
			return true;
		}
	}

	return false;
}

bool ARemainPickupShockController::DoesPickupMatch(AActor* PickupActor) const
{
	if (!IsValid(PickupActor))
	{
		return false;
	}

	if (TriggerPickupActor && PickupActor == TriggerPickupActor)
	{
		return true;
	}

	if (TriggerPartId.IsNone())
	{
		return false;
	}

	for (const FName PartId : ResolvePartIds(PickupActor))
	{
		if (PartId == TriggerPartId)
		{
			return true;
		}
	}

	return false;
}

bool ARemainPickupShockController::DoesItemMatch(UObject* Item) const
{
	if (!IsValid(Item) || TriggerPartId.IsNone())
	{
		return false;
	}

	for (const FName PartId : ResolvePartIds(Item))
	{
		if (PartId == TriggerPartId)
		{
			return true;
		}
	}

	return false;
}

void ARemainPickupShockController::MuteAmbientAudio()
{
	MutedAudioComponents.Reset();
	OriginalAudioVolumes.Reset();

	TArray<UAudioComponent*> AudioComponents;
	for (AActor* AudioActor : AmbientAudioActors)
	{
		CollectAudioComponents(AudioActor, AudioComponents);
	}

	for (UAudioComponent* AudioComponent : AudioComponents)
	{
		MutedAudioComponents.Add(AudioComponent);
		OriginalAudioVolumes.Add(AudioComponent->VolumeMultiplier);
		AudioComponent->SetVolumeMultiplier(MutedAmbientVolume);
	}
}

void ARemainPickupShockController::RestoreAmbientAudio()
{
	for (int32 Index = 0; Index < MutedAudioComponents.Num(); ++Index)
	{
		if (UAudioComponent* AudioComponent = MutedAudioComponents[Index].Get())
		{
			const float OriginalVolume = OriginalAudioVolumes.IsValidIndex(Index) ? OriginalAudioVolumes[Index] : 1.0f;
			AudioComponent->SetVolumeMultiplier(OriginalVolume);
		}
	}
}

void ARemainPickupShockController::ExtinguishConfiguredLights()
{
	TArray<ULightComponent*> LightComponents;
	for (AActor* LightActor : LightsToExtinguish)
	{
		CollectLightComponents(LightActor, LightComponents);
	}

	for (ULightComponent* LightComponent : LightComponents)
	{
		LightComponent->SetIntensity(ExtinguishedLightIntensity);
		if (bHideExtinguishedLights)
		{
			LightComponent->SetVisibility(false);
		}
	}
}

void ARemainPickupShockController::StartScreenEffects()
{
	APlayerController* PlayerController = ResolvePlayerController(this);
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (bUseWhiteCameraFade && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(0.0f, WhiteFadeAlpha, 0.05f, FLinearColor::White, false, true);
	}

	if (!bUseDesaturationFlash)
	{
		return;
	}

	UCameraComponent* CameraComponent = ResolvePlayerCameraComponent(PlayerController);
	if (!CameraComponent)
	{
		return;
	}

	ActiveFlashCamera = CameraComponent;
	bSavedColorSaturationOverride = CameraComponent->PostProcessSettings.bOverride_ColorSaturation;
	SavedColorSaturation = CameraComponent->PostProcessSettings.ColorSaturation;
	SavedPostProcessBlendWeight = CameraComponent->PostProcessBlendWeight;

	CameraComponent->PostProcessSettings.bOverride_ColorSaturation = true;
	CameraComponent->PostProcessSettings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
	CameraComponent->PostProcessBlendWeight = FMath::Max(CameraComponent->PostProcessBlendWeight, FlashPostProcessBlendWeight);
}

void ARemainPickupShockController::RestoreScreenEffects()
{
	if (APlayerController* PlayerController = ResolvePlayerController(this))
	{
		if (bUseWhiteCameraFade && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(WhiteFadeAlpha, 0.0f, 0.15f, FLinearColor::White, false, false);
		}
	}

	if (UCameraComponent* CameraComponent = ActiveFlashCamera.Get())
	{
		CameraComponent->PostProcessSettings.bOverride_ColorSaturation = bSavedColorSaturationOverride;
		CameraComponent->PostProcessSettings.ColorSaturation = SavedColorSaturation;
		CameraComponent->PostProcessBlendWeight = SavedPostProcessBlendWeight;
	}

	ActiveFlashCamera.Reset();
}
