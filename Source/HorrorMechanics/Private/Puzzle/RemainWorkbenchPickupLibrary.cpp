#include "Puzzle/RemainWorkbenchPickupLibrary.h"

#include "Components/ActorComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace
{
	static bool IsEditorOnlyVisualComponent(const UActorComponent* Component)
	{
		if (!Component)
		{
			return false;
		}

		const FString Name = Component->GetName();
		return Component->IsA<UBillboardComponent>()
			|| Component->IsA<UArrowComponent>()
			|| Name.Contains(TEXT("Editor"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Icon"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Sprite"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Billboard"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Arrow"), ESearchCase::IgnoreCase);
	}

	static bool ShouldForceGameVisible(const UPrimitiveComponent* Primitive)
	{
		return Primitive
			&& !IsEditorOnlyVisualComponent(Primitive)
			&& (Primitive->IsA<UStaticMeshComponent>() || Primitive->IsA<USkeletalMeshComponent>());
	}

	static bool IsLikelyInteractComponent(const UActorComponent* Component)
	{
		if (!Component)
		{
			return false;
		}

		const FString Name = Component->GetName();
		return Name.Contains(TEXT("Interact"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Interaction"), ESearchCase::IgnoreCase);
	}

	static void SetBoolPropertyIfPresent(UObject* Object, FName PropertyName, bool bValue)
	{
		if (!IsValid(Object))
		{
			return;
		}

		if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
		{
			BoolProperty->SetPropertyValue_InContainer(Object, bValue);
		}
	}

	static void SetInteractionStateProperties(UObject* Object, bool bEnabled)
	{
		SetBoolPropertyIfPresent(Object, TEXT("DisableInteraction"), !bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("bDisableInteraction"), !bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("InteractionDisabled"), !bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("bInteractionDisabled"), !bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("Interactive"), bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("bInteractive"), bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("CanInteract"), bEnabled);
		SetBoolPropertyIfPresent(Object, TEXT("bCanInteract"), bEnabled);
	}

	static void ApplyPrimitiveState(UPrimitiveComponent* Primitive, bool bEnabled)
	{
		if (!IsValid(Primitive))
		{
			return;
		}

		if (!bEnabled)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Primitive->SetCollisionResponseToAllChannels(ECR_Ignore);
			Primitive->SetGenerateOverlapEvents(false);
			Primitive->SetHiddenInGame(true, true);
			Primitive->SetVisibility(false, true);
			return;
		}

		if (IsEditorOnlyVisualComponent(Primitive))
		{
			Primitive->SetHiddenInGame(true, true);
			Primitive->SetVisibility(false, true);
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Primitive->SetCollisionResponseToAllChannels(ECR_Ignore);
			Primitive->SetGenerateOverlapEvents(false);
			return;
		}

		if (ShouldForceGameVisible(Primitive))
		{
			Primitive->SetHiddenInGame(false, true);
			Primitive->SetVisibility(true, true);
		}

		Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Primitive->SetCollisionResponseToAllChannels(ECR_Ignore);
		Primitive->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Primitive->SetGenerateOverlapEvents(IsLikelyInteractComponent(Primitive));
	}

	static void SetWorkbenchPickupEnabledRecursive(AActor* PickupActor, bool bEnabled, TSet<AActor*>& VisitedActors)
	{
		if (!IsValid(PickupActor) || VisitedActors.Contains(PickupActor))
		{
			return;
		}

		VisitedActors.Add(PickupActor);

		PickupActor->SetActorHiddenInGame(!bEnabled);
		PickupActor->SetActorEnableCollision(bEnabled);
		PickupActor->SetActorTickEnabled(bEnabled);
		SetInteractionStateProperties(PickupActor, bEnabled);

		TArray<UActorComponent*> Components;
		PickupActor->GetComponents(Components);

		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component))
			{
				continue;
			}

			Component->SetComponentTickEnabled(bEnabled);
			Component->SetActive(bEnabled, true);
			SetInteractionStateProperties(Component, bEnabled);

			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
			{
				ApplyPrimitiveState(Primitive, bEnabled);
			}

			if (UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(Component))
			{
				ChildActorComponent->SetHiddenInGame(!bEnabled, true);
				ChildActorComponent->SetVisibility(bEnabled, true);

				if (AActor* ChildActor = ChildActorComponent->GetChildActor())
				{
					SetWorkbenchPickupEnabledRecursive(ChildActor, bEnabled, VisitedActors);
				}
			}
		}
	}
}

void URemainWorkbenchPickupLibrary::SetWorkbenchPickupEnabled(AActor* PickupActor, bool bEnabled)
{
	TSet<AActor*> VisitedActors;
	SetWorkbenchPickupEnabledRecursive(PickupActor, bEnabled, VisitedActors);
}
