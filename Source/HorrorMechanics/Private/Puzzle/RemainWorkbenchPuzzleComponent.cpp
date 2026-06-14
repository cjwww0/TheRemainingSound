#include "Puzzle/RemainWorkbenchPuzzleComponent.h"

#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Puzzle/RemainWorkbenchPartIdProvider.h"
#include "Puzzle/RemainWorkbenchPickupLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

URemainWorkbenchPuzzleComponent::URemainWorkbenchPuzzleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URemainWorkbenchPuzzleComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureStateArrays();
	ConfigureInteractionCollision();
	RefreshSlotVisuals();
	ApplySequentialPickupVisibility();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &URemainWorkbenchPuzzleComponent::ApplySequentialPickupVisibility));
	}
}

namespace
{
	static UStaticMeshComponent* ResolveSlotMeshComponent(AActor* Owner, const FRemainWorkbenchSlotConfig& SlotConfig)
	{
		return Owner ? Cast<UStaticMeshComponent>(SlotConfig.SlotMeshComponent.GetComponent(Owner)) : nullptr;
	}

	static ULightComponent* ResolveWarmLightComponent(AActor* Owner, const FRemainWorkbenchSlotConfig& SlotConfig)
	{
		return Owner ? Cast<ULightComponent>(SlotConfig.WarmLight.GetComponent(Owner)) : nullptr;
	}

	static void CollectWorkbenchItemClasses(const UObject* Item, TArray<const UClass*>& OutClasses)
	{
		if (!IsValid(Item))
		{
			return;
		}

		if (const UClass* DirectClass = Item->GetClass())
		{
			OutClasses.AddUnique(DirectClass);
			if (!DirectClass->GetName().Contains(TEXT("InventoryItem")))
			{
				return;
			}
		}

		const UClass* ItemClass = Item->GetClass();
		if (!ItemClass)
		{
			return;
		}

		static const FName ObjectPropertyCandidates[] =
		{
			TEXT("ItemInstanceRef"),
			TEXT("InventoryItemRef"),
			TEXT("ExaminableItem")
		};

		for (const FName PropertyName : ObjectPropertyCandidates)
		{
			if (const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(ItemClass, PropertyName))
			{
				if (UObject* ReferencedObject = ObjectProperty->GetObjectPropertyValue_InContainer(Item))
				{
					OutClasses.AddUnique(ReferencedObject->GetClass());
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
			if (const FClassProperty* ClassProperty = FindFProperty<FClassProperty>(ItemClass, PropertyName))
			{
				if (UObject* ReferencedClassObject = ClassProperty->GetObjectPropertyValue_InContainer(Item))
				{
					if (const UClass* ReferencedClass = Cast<UClass>(ReferencedClassObject))
					{
						OutClasses.AddUnique(ReferencedClass);
					}
				}
			}
		}
	}

	static void CollectWorkbenchPartIds(const UObject* Item, TArray<FName>& OutPartIds, TSet<const UObject*>& VisitedObjects, TSet<const UClass*>& VisitedClasses)
	{
		if (!IsValid(Item) || VisitedObjects.Contains(Item))
		{
			return;
		}

		VisitedObjects.Add(Item);

		if (const UClass* ItemClass = Item->GetClass())
		{
			if (ItemClass->ImplementsInterface(URemainWorkbenchPartIdProvider::StaticClass()))
			{
				const FName PartId = IRemainWorkbenchPartIdProvider::Execute_GetWorkbenchPartId(const_cast<UObject*>(Item));
				if (!PartId.IsNone())
				{
					OutPartIds.AddUnique(PartId);
				}
			}

			if (const FNameProperty* PartIdProperty = FindFProperty<FNameProperty>(ItemClass, TEXT("PartId")))
			{
				const FName PartId = PartIdProperty->GetPropertyValue_InContainer(Item);
				if (!PartId.IsNone())
				{
					OutPartIds.AddUnique(PartId);
				}
			}

			if (!VisitedClasses.Contains(ItemClass))
			{
				VisitedClasses.Add(ItemClass);
				if (const UObject* DefaultObject = ItemClass->GetDefaultObject())
				{
					if (ItemClass->ImplementsInterface(URemainWorkbenchPartIdProvider::StaticClass()))
					{
						const FName PartId = IRemainWorkbenchPartIdProvider::Execute_GetWorkbenchPartId(const_cast<UObject*>(DefaultObject));
						if (!PartId.IsNone())
						{
							OutPartIds.AddUnique(PartId);
						}
					}

					if (const FNameProperty* DefaultPartIdProperty = FindFProperty<FNameProperty>(ItemClass, TEXT("PartId")))
					{
						const FName PartId = DefaultPartIdProperty->GetPropertyValue_InContainer(DefaultObject);
						if (!PartId.IsNone())
						{
							OutPartIds.AddUnique(PartId);
						}
					}
				}
			}
		}

		static const FName ObjectPropertyCandidates[] =
		{
			TEXT("ItemInstanceRef"),
			TEXT("InventoryItemRef"),
			TEXT("ExaminableItem")
		};

		for (const FName PropertyName : ObjectPropertyCandidates)
		{
			if (const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Item->GetClass(), PropertyName))
			{
				if (UObject* ReferencedObject = ObjectProperty->GetObjectPropertyValue_InContainer(Item))
				{
					CollectWorkbenchPartIds(ReferencedObject, OutPartIds, VisitedObjects, VisitedClasses);
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
			if (const FClassProperty* ClassProperty = FindFProperty<FClassProperty>(Item->GetClass(), PropertyName))
			{
				if (UObject* ReferencedClassObject = ClassProperty->GetObjectPropertyValue_InContainer(Item))
				{
					if (const UClass* ReferencedClass = Cast<UClass>(ReferencedClassObject))
					{
						if (!VisitedClasses.Contains(ReferencedClass))
						{
							VisitedClasses.Add(ReferencedClass);
							if (const UObject* DefaultObject = ReferencedClass->GetDefaultObject())
							{
								if (ReferencedClass->ImplementsInterface(URemainWorkbenchPartIdProvider::StaticClass()))
								{
									const FName PartId = IRemainWorkbenchPartIdProvider::Execute_GetWorkbenchPartId(const_cast<UObject*>(DefaultObject));
									if (!PartId.IsNone())
									{
										OutPartIds.AddUnique(PartId);
									}
								}

								if (const FNameProperty* DefaultPartIdProperty = FindFProperty<FNameProperty>(ReferencedClass, TEXT("PartId")))
								{
									const FName PartId = DefaultPartIdProperty->GetPropertyValue_InContainer(DefaultObject);
									if (!PartId.IsNone())
									{
										OutPartIds.AddUnique(PartId);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	static TArray<FName> ResolveWorkbenchPartIds(const UObject* Item)
	{
		TArray<FName> Result;
		TSet<const UObject*> VisitedObjects;
		TSet<const UClass*> VisitedClasses;
		CollectWorkbenchPartIds(Item, Result, VisitedObjects, VisitedClasses);
		return Result;
	}

	static TArray<UObject*> GetInventoryArrayObjects(UObject* Inventory)
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

	static bool ReadActorArrayProperty(UObject* Object, const FName PropertyName, TArray<AActor*>& OutActors)
	{
		OutActors.Reset();
		if (!IsValid(Object))
		{
			return false;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		if (!ArrayProperty || !ArrayProperty->Inner)
		{
			return false;
		}

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
		if (!ObjectProperty || !ObjectProperty->PropertyClass || !ObjectProperty->PropertyClass->IsChildOf(AActor::StaticClass()))
		{
			return false;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			if (AActor* Actor = Cast<AActor>(ObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index))))
			{
				OutActors.Add(Actor);
			}
			else
			{
				OutActors.Add(nullptr);
			}
		}

		return OutActors.Num() > 0;
	}
}

bool URemainWorkbenchPuzzleComponent::CanUseItem(UObject* Item) const
{
	if (!IsValid(Item) || IsComplete())
	{
		return false;
	}

	if (!SlotConfigs.IsValidIndex(CurrentUnlockedSlotIndex) || !PlacedSlots.IsValidIndex(CurrentUnlockedSlotIndex))
	{
		return false;
	}

	if (PlacedSlots[CurrentUnlockedSlotIndex])
	{
		return false;
	}

	const FRemainWorkbenchSlotConfig& SlotConfig = SlotConfigs[CurrentUnlockedSlotIndex];
	if (!SlotConfig.RequiredPartId.IsNone())
	{
		for (const FName PartId : ResolveWorkbenchPartIds(Item))
		{
			if (PartId == SlotConfig.RequiredPartId)
			{
				return true;
			}
		}
	}

	if (!SlotConfig.RequiredItemClass)
	{
		return false;
	}

	if (Item->IsA(SlotConfig.RequiredItemClass))
	{
		return true;
	}

	TArray<const UClass*> CandidateClasses;
	CollectWorkbenchItemClasses(Item, CandidateClasses);
	for (const UClass* CandidateClass : CandidateClasses)
	{
		if (CandidateClass && CandidateClass->IsChildOf(SlotConfig.RequiredItemClass))
		{
			return true;
		}
	}

	return false;
}

bool URemainWorkbenchPuzzleComponent::PlaceItem(UObject* Item, UObject* Inventory)
{
	if (!CanUseItem(Item) || !SlotConfigs.IsValidIndex(CurrentUnlockedSlotIndex) || !PlacedSlots.IsValidIndex(CurrentUnlockedSlotIndex))
	{
		return false;
	}

	if (!RemoveInventoryItem(Inventory, Item))
	{
		return false;
	}

	const int32 SlotIndex = CurrentUnlockedSlotIndex;
	const FRemainWorkbenchSlotConfig& SlotConfig = SlotConfigs[SlotIndex];

	PlacedSlots[SlotIndex] = true;
	PlacedCount = FMath::Clamp(PlacedCount + 1, 0, SlotConfigs.Num());

	if (UStaticMeshComponent* SlotMeshComponent = ResolveSlotMeshComponent(GetOwner(), SlotConfig); SlotMeshComponent && SlotConfig.PlacedMesh)
	{
		SlotMeshComponent->SetStaticMesh(SlotConfig.PlacedMesh);
		SlotMeshComponent->SetVisibility(true, true);
	}

	if (SlotConfig.InsertSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SlotConfig.InsertSound, GetOwner()->GetActorLocation());
	}

	SetWarmLightActive(SlotIndex, true);

	if (WarmLightDuration > 0.0f)
	{
		ActiveWarmLightSlotIndex = SlotIndex;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(WarmLightTimerHandle);
			World->GetTimerManager().SetTimer(WarmLightTimerHandle, this, &URemainWorkbenchPuzzleComponent::HandleWarmLightTimeout, WarmLightDuration, false);
		}
	}

	const bool bWasFirstPlacement = !bFaultLightsTriggered;
	if (CurrentUnlockedSlotIndex < SlotConfigs.Num() - 1)
	{
		++CurrentUnlockedSlotIndex;
	}

	OnSlotPlaced.Broadcast(SlotIndex, SlotConfig.SlotId, Item);
	ApplySequentialPickupVisibility();

	if (bWasFirstPlacement)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[P15Fault] Workbench first panel placed | Owner=%s | SlotIndex=%d | SlotId=%s | Item=%s"),
			*GetNameSafe(GetOwner()),
			SlotIndex,
			*SlotConfig.SlotId.ToString(),
			*GetNameSafe(Item));

		bFaultLightsTriggered = true;
		OnFirstPanelPlaced.Broadcast();
	}

	if (IsComplete())
	{
		OnAllSlotsPlaced.Broadcast();
	}

	return true;
}

int32 URemainWorkbenchPuzzleComponent::GetCurrentUnlockedSlotIndex() const
{
	return CurrentUnlockedSlotIndex;
}

int32 URemainWorkbenchPuzzleComponent::GetPlacedCount() const
{
	return PlacedCount;
}

bool URemainWorkbenchPuzzleComponent::HasTriggeredFaultLights() const
{
	return bFaultLightsTriggered;
}

bool URemainWorkbenchPuzzleComponent::IsSlotPlaced(int32 SlotIndex) const
{
	return PlacedSlots.IsValidIndex(SlotIndex) ? PlacedSlots[SlotIndex] : false;
}

bool URemainWorkbenchPuzzleComponent::IsComplete() const
{
	return SlotConfigs.Num() > 0 && PlacedCount >= SlotConfigs.Num();
}

FText URemainWorkbenchPuzzleComponent::GetCurrentPromptText() const
{
	if (IsComplete())
	{
		return FText::FromString(TEXT("已完成"));
	}

	const int32 DisplayIndex = FMath::Clamp(CurrentUnlockedSlotIndex + 1, 1, FMath::Max(SlotConfigs.Num(), 1));
	return FText::Format(
		NSLOCTEXT("RemainWorkbench", "PlacePanelPrompt", "[左键] 放置面板 {0}/{1}"),
		FText::AsNumber(DisplayIndex),
		FText::AsNumber(FMath::Max(SlotConfigs.Num(), 1)));
}

FName URemainWorkbenchPuzzleComponent::GetCurrentRequiredPartId() const
{
	if (!SlotConfigs.IsValidIndex(CurrentUnlockedSlotIndex))
	{
		return NAME_None;
	}

	return SlotConfigs[CurrentUnlockedSlotIndex].RequiredPartId;
}

FString URemainWorkbenchPuzzleComponent::BuildInventoryAcceptanceDebugString(UObject* Inventory) const
{
	TArray<FString> Parts;

	const UClass* RequiredClass = nullptr;
	FName RequiredPartId = NAME_None;
	if (SlotConfigs.IsValidIndex(CurrentUnlockedSlotIndex))
	{
		RequiredClass = SlotConfigs[CurrentUnlockedSlotIndex].RequiredItemClass;
		RequiredPartId = SlotConfigs[CurrentUnlockedSlotIndex].RequiredPartId;
	}

	Parts.Add(RequiredClass ? FString::Printf(TEXT("Required=%s"), *RequiredClass->GetName()) : TEXT("Required=None"));
	Parts.Add(RequiredPartId.IsNone() ? TEXT("RequiredPartId=None") : FString::Printf(TEXT("RequiredPartId=%s"), *RequiredPartId.ToString()));
	Parts.Add(FString::Printf(TEXT("SlotIndex=%d"), CurrentUnlockedSlotIndex));

	const TArray<UObject*> Items = GetInventoryArrayObjects(Inventory);
	Parts.Add(FString::Printf(TEXT("InventoryCount=%d"), Items.Num()));

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		UObject* Item = Items[Index];
		if (!IsValid(Item))
		{
			Parts.Add(FString::Printf(TEXT("[%d] Item=None"), Index));
			continue;
		}

		TArray<const UClass*> CandidateClasses;
		CollectWorkbenchItemClasses(Item, CandidateClasses);
		const TArray<FName> CandidatePartIds = ResolveWorkbenchPartIds(Item);

		TArray<FString> CandidateNames;
		TArray<FString> CandidatePartIdNames;
		bool bAccepted = false;
		for (const UClass* CandidateClass : CandidateClasses)
		{
			if (CandidateClass)
			{
				CandidateNames.Add(CandidateClass->GetName());
				if (RequiredClass && CandidateClass->IsChildOf(RequiredClass))
				{
					bAccepted = true;
				}
			}
		}

		for (const FName CandidatePartId : CandidatePartIds)
		{
			CandidatePartIdNames.Add(CandidatePartId.ToString());
			if (!RequiredPartId.IsNone() && CandidatePartId == RequiredPartId)
			{
				bAccepted = true;
			}
		}

		Parts.Add(FString::Printf(
			TEXT("[%d] ItemClass=%s Accepted=%s Candidates=%s PartIds=%s"),
			Index,
			*Item->GetClass()->GetName(),
			bAccepted ? TEXT("true") : TEXT("false"),
			CandidateNames.Num() > 0 ? *FString::Join(CandidateNames, TEXT(",")) : TEXT("None"),
			CandidatePartIdNames.Num() > 0 ? *FString::Join(CandidatePartIdNames, TEXT(",")) : TEXT("None")));
	}

	return FString::Join(Parts, TEXT(" | "));
}

FRemainWorkbenchCheckpointState URemainWorkbenchPuzzleComponent::GetCheckpointState() const
{
	FRemainWorkbenchCheckpointState State;
	State.PlacedSlots = PlacedSlots;
	State.CurrentUnlockedSlotIndex = CurrentUnlockedSlotIndex;
	State.PlacedCount = PlacedCount;
	State.bFaultLightsTriggered = bFaultLightsTriggered;
	return State;
}

void URemainWorkbenchPuzzleComponent::ApplyCheckpointState(const FRemainWorkbenchCheckpointState& State)
{
	EnsureStateArrays();

	for (int32 Index = 0; Index < PlacedSlots.Num(); ++Index)
	{
		PlacedSlots[Index] = State.PlacedSlots.IsValidIndex(Index) ? State.PlacedSlots[Index] : false;
	}

	PlacedCount = FMath::Clamp(State.PlacedCount, 0, SlotConfigs.Num());
	CurrentUnlockedSlotIndex = FMath::Clamp(State.CurrentUnlockedSlotIndex, 0, FMath::Max(SlotConfigs.Num() - 1, 0));
	bFaultLightsTriggered = State.bFaultLightsTriggered;

	RefreshSlotVisuals();
	ApplySequentialPickupVisibility();
}

void URemainWorkbenchPuzzleComponent::ResetWorkbenchState()
{
	EnsureStateArrays();
	for (bool& bPlaced : PlacedSlots)
	{
		bPlaced = false;
	}

	PlacedCount = 0;
	CurrentUnlockedSlotIndex = 0;
	bFaultLightsTriggered = false;
	ActiveWarmLightSlotIndex = INDEX_NONE;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarmLightTimerHandle);
	}

	RefreshSlotVisuals();
	ApplySequentialPickupVisibility();
}

void URemainWorkbenchPuzzleComponent::RebuildInteractionTraceProxy()
{
	ConfigureInteractionCollision();
}

void URemainWorkbenchPuzzleComponent::HandleWarmLightTimeout()
{
	if (ActiveWarmLightSlotIndex != INDEX_NONE)
	{
		SetWarmLightActive(ActiveWarmLightSlotIndex, false);
		ActiveWarmLightSlotIndex = INDEX_NONE;
	}
}

void URemainWorkbenchPuzzleComponent::EnsureStateArrays()
{
	if (PlacedSlots.Num() != SlotConfigs.Num())
	{
		PlacedSlots.SetNumZeroed(SlotConfigs.Num());
	}
}

void URemainWorkbenchPuzzleComponent::ConfigureInteractionCollision()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	for (const FRemainWorkbenchSlotConfig& SlotConfig : SlotConfigs)
	{
		if (UStaticMeshComponent* SlotMeshComponent = ResolveSlotMeshComponent(Owner, SlotConfig))
		{
			if (bForceVisibilityBlockOnSlotMeshes)
			{
				if (SlotMeshComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
				{
					SlotMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				}

				SlotMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				SlotMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
		}
	}

	if (!bCreateInteractionTraceProxy)
	{
		return;
	}

	FBox LocalBounds(EForceInit::ForceInit);
	if (!BuildInteractionLocalBounds(LocalBounds))
	{
		return;
	}

	if (!InteractionTraceProxy)
	{
		InteractionTraceProxy = NewObject<UBoxComponent>(Owner, TEXT("RemainWorkbenchTraceProxy"));
		if (!InteractionTraceProxy)
		{
			return;
		}

		InteractionTraceProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionTraceProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
		InteractionTraceProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		InteractionTraceProxy->SetGenerateOverlapEvents(false);
		InteractionTraceProxy->SetHiddenInGame(true);
		InteractionTraceProxy->SetVisibility(false);
		InteractionTraceProxy->SetCanEverAffectNavigation(false);

		if (USceneComponent* RootComponent = Owner->GetRootComponent())
		{
			InteractionTraceProxy->SetupAttachment(RootComponent);
		}

		Owner->AddInstanceComponent(InteractionTraceProxy);
		InteractionTraceProxy->RegisterComponent();
	}

	const FVector BoxCenter = LocalBounds.GetCenter();
	const FVector BoxExtent = LocalBounds.GetExtent().ComponentMax(InteractionTraceMinimumExtent) + InteractionTracePadding;
	InteractionTraceProxy->SetRelativeLocation(BoxCenter);
	InteractionTraceProxy->SetBoxExtent(BoxExtent, true);
}

void URemainWorkbenchPuzzleComponent::RefreshSlotVisuals()
{
	for (int32 Index = 0; Index < SlotConfigs.Num(); ++Index)
	{
		const FRemainWorkbenchSlotConfig& SlotConfig = SlotConfigs[Index];
		if (UStaticMeshComponent* SlotMeshComponent = ResolveSlotMeshComponent(GetOwner(), SlotConfig))
		{
			if (PlacedSlots.IsValidIndex(Index) && PlacedSlots[Index] && SlotConfig.PlacedMesh)
			{
				SlotMeshComponent->SetStaticMesh(SlotConfig.PlacedMesh);
				SlotMeshComponent->SetVisibility(true, true);
			}
			else if (SlotConfig.EmptyGrooveMesh)
			{
				SlotMeshComponent->SetStaticMesh(SlotConfig.EmptyGrooveMesh);
				SlotMeshComponent->SetVisibility(true, true);
			}
		}

		SetWarmLightActive(Index, false);
	}
}

void URemainWorkbenchPuzzleComponent::SetWarmLightActive(int32 SlotIndex, bool bActive)
{
	if (!SlotConfigs.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (ULightComponent* WarmLight = ResolveWarmLightComponent(GetOwner(), SlotConfigs[SlotIndex]))
	{
		WarmLight->SetVisibility(bActive);
	}
}

void URemainWorkbenchPuzzleComponent::ApplySequentialPickupVisibility()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<AActor*> WorkbenchPickups;
	if (!ReadActorArrayProperty(Owner, TEXT("WorkbenchPickups"), WorkbenchPickups))
	{
		return;
	}

	const int32 EnabledIndex = IsComplete() ? INDEX_NONE : CurrentUnlockedSlotIndex;
	for (int32 Index = 0; Index < WorkbenchPickups.Num(); ++Index)
	{
		URemainWorkbenchPickupLibrary::SetWorkbenchPickupEnabled(WorkbenchPickups[Index], Index == EnabledIndex);
	}
}

bool URemainWorkbenchPuzzleComponent::RemoveInventoryItem(UObject* Inventory, UObject* Item) const
{
	if (!IsValid(Inventory) || !IsValid(Item))
	{
		return false;
	}

	UFunction* RemoveInventoryItemFunction = Inventory->FindFunction(TEXT("RemoveInventoryItem"));
	if (!RemoveInventoryItemFunction)
	{
		return false;
	}

	struct FRemoveInventoryItemParams
	{
		UObject* InventoryItem = nullptr;
		bool Removed = false;
	};

	FRemoveInventoryItemParams Params;
	Params.InventoryItem = Item;
	Inventory->ProcessEvent(RemoveInventoryItemFunction, &Params);
	return Params.Removed;
}

bool URemainWorkbenchPuzzleComponent::BuildInteractionLocalBounds(FBox& OutLocalBounds) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FTransform WorldToActor = Owner->GetActorTransform().Inverse();
	FBox LocalBounds(EForceInit::ForceInit);
	bool bHasBounds = false;

	for (const FRemainWorkbenchSlotConfig& SlotConfig : SlotConfigs)
	{
		const UStaticMeshComponent* SlotMeshComponent = ResolveSlotMeshComponent(Owner, SlotConfig);
		if (!SlotMeshComponent)
		{
			continue;
		}

		const FBox WorldBox = SlotMeshComponent->Bounds.GetBox();
		const FVector Min = WorldBox.Min;
		const FVector Max = WorldBox.Max;

		const FVector Corners[8] =
		{
			FVector(Min.X, Min.Y, Min.Z),
			FVector(Min.X, Min.Y, Max.Z),
			FVector(Min.X, Max.Y, Min.Z),
			FVector(Min.X, Max.Y, Max.Z),
			FVector(Max.X, Min.Y, Min.Z),
			FVector(Max.X, Min.Y, Max.Z),
			FVector(Max.X, Max.Y, Min.Z),
			FVector(Max.X, Max.Y, Max.Z)
		};

		for (const FVector& Corner : Corners)
		{
			LocalBounds += WorldToActor.TransformPosition(Corner);
			bHasBounds = true;
		}
	}

	if (!bHasBounds)
	{
		return false;
	}

	OutLocalBounds = LocalBounds;
	return true;
}
