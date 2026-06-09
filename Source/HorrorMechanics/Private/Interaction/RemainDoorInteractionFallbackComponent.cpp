#include "Interaction/RemainDoorInteractionFallbackComponent.h"

#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	struct FDoorFunctionParams
	{
		APlayerController* PlayerController = nullptr;
	};
}

URemainDoorInteractionFallbackComponent::URemainDoorInteractionFallbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URemainDoorInteractionFallbackComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyUnlockedState();
	CacheVisualClosedRotation();
	RebindInput();
}

void URemainDoorInteractionFallbackComponent::RebindInput()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!bEnableFallback || !Owner || !World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	Owner->EnableInput(PlayerController);
	if (Owner->InputComponent)
	{
		Owner->InputComponent->Priority = 1000;
		Owner->InputComponent->bBlockInput = false;
		FInputActionBinding& Binding = Owner->InputComponent->BindAction(InteractActionName, IE_Pressed, this, &URemainDoorInteractionFallbackComponent::HandleInteractPressed);
		Binding.bConsumeInput = false;
	}
}

void URemainDoorInteractionFallbackComponent::HandleInteractPressed()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!bEnableFallback || !PlayerController || !IsPlayerLookingAtOwner(PlayerController))
	{
		return;
	}

	bool bIsClosed = true;
	if (!GetDoorClosedState(bIsClosed))
	{
		DebugMessage(TEXT("Skipped: IsClosed property not found"));
		return;
	}

	bPendingFallback = true;
	bStateBeforeInteraction = bIsClosed;
	PendingPlayerController = PlayerController;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackTimerHandle);
		World->GetTimerManager().SetTimer(FallbackTimerHandle, this, &URemainDoorInteractionFallbackComponent::EvaluateFallback, FallbackDelay, false);
	}
}

void URemainDoorInteractionFallbackComponent::EvaluateFallback()
{
	if (!bPendingFallback)
	{
		return;
	}

	bPendingFallback = false;
	ApplyUnlockedState();

	bool bCurrentIsClosed = true;
	if (!GetDoorClosedState(bCurrentIsClosed))
	{
		return;
	}

	// If the original blueprint interaction already changed the state, do not double-toggle.
	if (bCurrentIsClosed != bStateBeforeInteraction)
	{
		ApplyDoorVisualState(bCurrentIsClosed);
		DebugMessage(TEXT("Original door interaction handled the toggle; visual applied"));
		return;
	}

	APlayerController* PlayerController = PendingPlayerController.Get();
	if (!PlayerController)
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	const bool bTargetClosed = !bCurrentIsClosed;
	InvokeDoorFunction(bCurrentIsClosed ? TEXT("Open") : TEXT("Close"), PlayerController);

	bool bStateAfterFunction = bTargetClosed;
	GetDoorClosedState(bStateAfterFunction);
	ApplyDoorVisualState(bStateAfterFunction);

	DebugMessage(bCurrentIsClosed ? TEXT("Fallback called Open; visual applied") : TEXT("Fallback called Close; visual applied"));
}

bool URemainDoorInteractionFallbackComponent::IsPlayerLookingAtOwner(APlayerController* PlayerController) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !PlayerController)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RemainDoorInteractionFallbackTrace), false);
	Params.AddIgnoredActor(PlayerController->GetPawn());

	const UWorld* World = GetWorld();
	if (!World || !World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		return false;
	}

	if (Hit.GetActor() == Owner)
	{
		return true;
	}

	const UActorComponent* HitComponent = Hit.GetComponent();
	return HitComponent && HitComponent->GetOwner() == Owner;
}

bool URemainDoorInteractionFallbackComponent::GetDoorClosedState(bool& bOutIsClosed) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	if (const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Owner->GetClass(), TEXT("IsClosed")))
	{
		bOutIsClosed = BoolProperty->GetPropertyValue_InContainer(Owner);
		return true;
	}

	return false;
}

void URemainDoorInteractionFallbackComponent::SetDoorBoolProperty(FName PropertyName, bool bValue) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Owner->GetClass(), PropertyName))
	{
		BoolProperty->SetPropertyValue_InContainer(Owner, bValue);
	}
}

void URemainDoorInteractionFallbackComponent::InvokeDoorFunction(FName FunctionName, APlayerController* PlayerController) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);
	if (!Function)
	{
		DebugMessage(FString::Printf(TEXT("Function not found: %s"), *FunctionName.ToString()));
		return;
	}

	FDoorFunctionParams Params;
	Params.PlayerController = PlayerController;
	Owner->ProcessEvent(Function, &Params);
}

void URemainDoorInteractionFallbackComponent::ApplyUnlockedState() const
{
	if (!bForceUnlocked)
	{
		return;
	}

	SetDoorBoolProperty(TEXT("Locked"), false);
	SetDoorBoolProperty(TEXT("Jammed"), false);
}

void URemainDoorInteractionFallbackComponent::CacheVisualClosedRotation()
{
	if (bCachedClosedRelativeRotation)
	{
		return;
	}

	if (const USceneComponent* DoorVisual = ResolveDoorVisualComponent())
	{
		CachedClosedRelativeRotation = DoorVisual->GetRelativeRotation();
		bCachedClosedRelativeRotation = true;
		DebugMessage(FString::Printf(TEXT("Cached visual component %s"), *DoorVisual->GetName()));
	}
	else
	{
		DebugMessage(TEXT("Door visual component not found"));
	}
}

void URemainDoorInteractionFallbackComponent::CacheDoorVisualCollision(UPrimitiveComponent* DoorPrimitive)
{
	if (bCachedDoorVisualCollision || !DoorPrimitive)
	{
		return;
	}

	CachedDoorVisualCollision = DoorPrimitive->GetCollisionEnabled();
	bCachedDoorVisualCollision = true;
	DebugMessage(FString::Printf(TEXT("Cached visual collision %s"), *StaticEnum<ECollisionEnabled::Type>()->GetNameStringByValue(static_cast<int64>(CachedDoorVisualCollision))));
}

void URemainDoorInteractionFallbackComponent::ApplyDoorVisualState(bool bIsClosed)
{
	if (!bDriveDoorVisual)
	{
		return;
	}

	CacheVisualClosedRotation();

	USceneComponent* DoorVisual = ResolveDoorVisualComponent();
	if (!DoorVisual || !bCachedClosedRelativeRotation)
	{
		DebugMessage(TEXT("Cannot apply visual state: visual component missing"));
		return;
	}

	UPrimitiveComponent* DoorPrimitive = Cast<UPrimitiveComponent>(DoorVisual);
	CacheDoorVisualCollision(DoorPrimitive);

	const FRotator TargetRotation = bIsClosed
		? CachedClosedRelativeRotation
		: CachedClosedRelativeRotation + OpenRelativeRotationOffset;

	DoorVisual->SetRelativeRotation(TargetRotation);
	ApplyDoorVisualCollision(bIsClosed, DoorPrimitive);
	DebugMessage(FString::Printf(TEXT("Set visual %s to %s"), *DoorVisual->GetName(), bIsClosed ? TEXT("Closed") : TEXT("Open")));
}

void URemainDoorInteractionFallbackComponent::ApplyDoorVisualCollision(bool bIsClosed, UPrimitiveComponent* DoorPrimitive)
{
	if (!bDisableDoorVisualCollisionWhenOpen || !DoorPrimitive)
	{
		return;
	}

	if (bIsClosed)
	{
		if (bCachedDoorVisualCollision)
		{
			DoorPrimitive->SetCollisionEnabled(CachedDoorVisualCollision);
		}
	}
	else
	{
		DoorPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	DebugMessage(FString::Printf(TEXT("Set visual collision %s to %s"),
		*DoorPrimitive->GetName(),
		bIsClosed ? TEXT("Restored") : TEXT("NoCollision")));
}

USceneComponent* URemainDoorInteractionFallbackComponent::ResolveDoorVisualComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents;
	Owner->GetComponents(SceneComponents);

	for (USceneComponent* Component : SceneComponents)
	{
		if (Component && Component->GetFName() == DoorVisualComponentName)
		{
			return Component;
		}
	}

	for (USceneComponent* Component : SceneComponents)
	{
		if (!Component)
		{
			continue;
		}

		const FString Name = Component->GetName();
		if (Name.Contains(TEXT("Door"), ESearchCase::IgnoreCase)
			&& !Name.Contains(TEXT("Area"), ESearchCase::IgnoreCase)
			&& !Name.Contains(TEXT("Proxy"), ESearchCase::IgnoreCase)
			&& !Name.Contains(TEXT("Handle"), ESearchCase::IgnoreCase))
		{
			return Component;
		}
	}

	for (USceneComponent* Component : SceneComponents)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
		if (!Primitive)
		{
			continue;
		}

		const FString Name = Component->GetName();
		if (Name.Contains(TEXT("Area"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Proxy"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Handle"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Interact"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Collision"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		return Component;
	}

	return nullptr;
}

void URemainDoorInteractionFallbackComponent::DebugMessage(const FString& Message) const
{
	if (!bDebugDoorFallback || !GEngine)
	{
		return;
	}

	const FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("None");
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("DoorFallback %s: %s"), *OwnerName, *Message));
}
