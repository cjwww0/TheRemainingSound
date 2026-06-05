#include "Components/RemainBreakableSwapComponent.h"

#include "Chaos/Particle/ObjectState.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "TimerManager.h"

namespace
{
const TCHAR* ToCollisionEnabledString(const ECollisionEnabled::Type CollisionEnabled)
{
	switch (CollisionEnabled)
	{
	case ECollisionEnabled::NoCollision:
		return TEXT("NoCollision");
	case ECollisionEnabled::QueryOnly:
		return TEXT("QueryOnly");
	case ECollisionEnabled::PhysicsOnly:
		return TEXT("PhysicsOnly");
	case ECollisionEnabled::QueryAndPhysics:
		return TEXT("QueryAndPhysics");
	case ECollisionEnabled::ProbeOnly:
		return TEXT("ProbeOnly");
	case ECollisionEnabled::QueryAndProbe:
		return TEXT("QueryAndProbe");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* ToMobilityString(const EComponentMobility::Type Mobility)
{
	switch (Mobility)
	{
	case EComponentMobility::Static:
		return TEXT("Static");
	case EComponentMobility::Stationary:
		return TEXT("Stationary");
	case EComponentMobility::Movable:
		return TEXT("Movable");
	default:
		return TEXT("Unknown");
	}
}
}

URemainBreakableSwapComponent::URemainBreakableSwapComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URemainBreakableSwapComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bPrepareBrokenActorsOnBeginPlay)
	{
		PrepareBreakableState();
	}
}

void URemainBreakableSwapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 Index = PendingScriptedChaosLaunches.Num() - 1; Index >= 0; --Index)
	{
		FPendingScriptedChaosLaunch& PendingLaunch = PendingScriptedChaosLaunches[Index];
		AActor* Actor = PendingLaunch.Actor.Get();
		if (!IsValid(Actor))
		{
			PendingScriptedChaosLaunches.RemoveAtSwap(Index);
			continue;
		}

		PendingLaunch.Elapsed += DeltaTime;
		const float Duration = FMath::Max(ScriptedChaosLaunchDuration, KINDA_SMALL_NUMBER);
		const float Alpha = FMath::Clamp(PendingLaunch.Elapsed / Duration, 0.0f, 1.0f);
		const float SmoothAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
		FVector NewLocation = FMath::Lerp(PendingLaunch.StartLocation, PendingLaunch.TargetLocation, SmoothAlpha);
		NewLocation.Z += FMath::Sin(Alpha * PI) * ScriptedChaosLaunchArcHeight;
		const FQuat NewRotation = FQuat::Slerp(PendingLaunch.StartRotation, PendingLaunch.TargetRotation, SmoothAlpha);

		Actor->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

		if (Alpha >= 1.0f)
		{
			FinishScriptedChaosLaunch(Actor);
			PendingScriptedChaosLaunches.RemoveAtSwap(Index);
		}
	}

	if (PendingScriptedChaosLaunches.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void URemainBreakableSwapComponent::PrepareBreakableState()
{
	bBroken = false;
	PendingChaosBreakActors.Reset();
	PendingScriptedChaosLaunches.Reset();
	SetComponentTickEnabled(false);

	const bool bSingleChaosMode = bUseChaosGeometryCollections && bUseSingleVisibleChaosActor;
	for (AActor* IntactActor : IntactActors)
	{
		SetActorBreakableEnabled(IntactActor, !(bSingleChaosMode && bHideIntactActorsInSingleChaosMode));
	}

	for (AActor* BrokenActor : BrokenActors)
	{
		SetActorBreakableEnabled(BrokenActor, bSingleChaosMode);

		if (bSingleChaosMode && IsValid(BrokenActor))
		{
			TInlineComponentArray<UGeometryCollectionComponent*> GeometryCollectionComponents;
			BrokenActor->GetComponents(GeometryCollectionComponents);
			for (UGeometryCollectionComponent* GeometryCollectionComponent : GeometryCollectionComponents)
			{
				if (!IsValid(GeometryCollectionComponent))
				{
					continue;
				}

				if (bForceBrokenActorsMovable && GeometryCollectionComponent->Mobility != EComponentMobility::Movable)
				{
					GeometryCollectionComponent->SetMobility(EComponentMobility::Movable);
				}

				GeometryCollectionComponent->SetSimulatePhysics(false);
				GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				if (bForceBrokenCollisionBlockAll)
				{
					GeometryCollectionComponent->SetCollisionObjectType(ECC_PhysicsBody);
					GeometryCollectionComponent->SetCollisionResponseToAllChannels(ECR_Block);
				}
				GeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Kinematic);
			}
		}
	}
}

bool URemainBreakableSwapComponent::TriggerBreak()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RemainBreakable] TriggerBreak called | Owner=%s | Intact=%d | Broken=%d | SingleChaos=%s | UseChaos=%s | Debug=%s"),
		*GetNameSafe(GetOwner()),
		IntactActors.Num(),
		BrokenActors.Num(),
		bUseSingleVisibleChaosActor ? TEXT("true") : TEXT("false"),
		bUseChaosGeometryCollections ? TEXT("true") : TEXT("false"),
		bDebugBreakable ? TEXT("true") : TEXT("false"));

	DebugBreakableMessage(FString::Printf(
		TEXT("TriggerBreak | Intact=%d | Broken=%d | SingleChaos=%s"),
		IntactActors.Num(),
		BrokenActors.Num(),
		bUseSingleVisibleChaosActor ? TEXT("true") : TEXT("false")));

	if (bBroken)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] TriggerBreak ignored because component is already broken | Owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	bBroken = true;

	const FVector ImpulseOrigin = ResolveImpulseOrigin();
	if (bUseChaosGeometryCollections && bUseSingleVisibleChaosActor)
	{
		TriggerSingleVisibleChaosActorBreak(ImpulseOrigin);
		return true;
	}

	if (bDisableIntactActorsOnBreak)
	{
		for (AActor* IntactActor : IntactActors)
		{
			SetActorBreakableEnabled(IntactActor, false);
		}
	}

	for (AActor* BrokenActor : BrokenActors)
	{
		ActivateBrokenActor(BrokenActor, ImpulseOrigin);
	}

	return true;
}

bool URemainBreakableSwapComponent::HasBroken() const
{
	return bBroken;
}

void URemainBreakableSwapComponent::SetActorBreakableEnabled(AActor* Actor, bool bEnabled) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorHiddenInGame(!bEnabled);
	Actor->SetActorEnableCollision(bEnabled);
	Actor->SetActorTickEnabled(bEnabled);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		PrimitiveComponent->SetVisibility(bEnabled, true);
		PrimitiveComponent->SetHiddenInGame(!bEnabled, true);
		PrimitiveComponent->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

bool URemainBreakableSwapComponent::PrepareChaosActorForScriptedLaunch(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	SetActorBreakableEnabled(Actor, true);

	TInlineComponentArray<UGeometryCollectionComponent*> GeometryCollectionComponents;
	Actor->GetComponents(GeometryCollectionComponents);
	if (GeometryCollectionComponents.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] No GeometryCollectionComponent found for scripted launch on %s"), *GetNameSafe(Actor));
		DebugBreakableMessage(FString::Printf(TEXT("No GC for scripted launch on %s"), *GetNameSafe(Actor)));
		return false;
	}

	for (UGeometryCollectionComponent* GeometryCollectionComponent : GeometryCollectionComponents)
	{
		if (!IsValid(GeometryCollectionComponent))
		{
			continue;
		}

		if (bForceBrokenActorsMovable && GeometryCollectionComponent->Mobility != EComponentMobility::Movable)
		{
			GeometryCollectionComponent->SetMobility(EComponentMobility::Movable);
		}

		GeometryCollectionComponent->SetVisibility(true, true);
		GeometryCollectionComponent->SetHiddenInGame(false, true);
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (bForceBrokenCollisionBlockAll)
		{
			GeometryCollectionComponent->SetCollisionObjectType(ECC_PhysicsBody);
			GeometryCollectionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		}

		GeometryCollectionComponent->SetSimulatePhysics(false);
		GeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Kinematic);
		LogGeometryCollectionState(TEXT("PreparedScriptedLaunch"), Actor, GeometryCollectionComponent);
	}

	return true;
}

void URemainBreakableSwapComponent::StartScriptedChaosLaunch(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	FVector LaunchDirection = DirectionalLaunchImpulse.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = Actor->GetActorForwardVector();
	}

	FPendingScriptedChaosLaunch PendingLaunch;
	PendingLaunch.Actor = Actor;
	PendingLaunch.StartLocation = Actor->GetActorLocation();
	PendingLaunch.TargetLocation = PendingLaunch.StartLocation + (LaunchDirection * ScriptedChaosLaunchDistance);
	PendingLaunch.StartRotation = Actor->GetActorQuat();
	PendingLaunch.TargetRotation = (Actor->GetActorRotation() + ScriptedChaosLaunchRotation).Quaternion();
	PendingLaunch.Elapsed = 0.0f;
	PendingScriptedChaosLaunches.Add(PendingLaunch);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RemainBreakable] Scripted Chaos launch started | Actor=%s | Start=%s | Target=%s | Duration=%.2f"),
		*GetNameSafe(Actor),
		*PendingLaunch.StartLocation.ToCompactString(),
		*PendingLaunch.TargetLocation.ToCompactString(),
		ScriptedChaosLaunchDuration);

	DebugBreakableMessage(FString::Printf(
		TEXT("Scripted launch %s | %.0fcm"),
		*GetNameSafe(Actor),
		ScriptedChaosLaunchDistance));

	SetComponentTickEnabled(true);
}

void URemainBreakableSwapComponent::FinishScriptedChaosLaunch(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] Scripted Chaos launch finished | Actor=%s"), *GetNameSafe(Actor));
	DebugBreakableMessage(FString::Printf(TEXT("Scripted launch finished %s"), *GetNameSafe(Actor)));

	const FVector ImpulseOrigin = ResolveImpulseOrigin();
	LaunchChaosGeometryCollections(Actor, ImpulseOrigin);
	BreakChaosGeometryCollections(Actor, ImpulseOrigin);
}

void URemainBreakableSwapComponent::ActivateBrokenActor(AActor* Actor, const FVector& ImpulseOrigin) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	SetActorBreakableEnabled(Actor, true);

	const bool bActivatedChaos = bUseChaosGeometryCollections && ActivateChaosGeometryCollections(Actor, ImpulseOrigin);
	if (bActivatedChaos)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		if (bEnablePhysicsOnBrokenActors)
		{
			if (bForceBrokenActorsMovable && PrimitiveComponent->Mobility != EComponentMobility::Movable)
			{
				PrimitiveComponent->SetMobility(EComponentMobility::Movable);
			}

			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			if (bForceBrokenCollisionBlockAll)
			{
				PrimitiveComponent->SetCollisionObjectType(ECC_PhysicsBody);
				PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Block);
			}
			PrimitiveComponent->SetSimulatePhysics(true);
		}

		if (bApplyRadialImpulse && FMath::Abs(ImpulseStrength) > KINDA_SMALL_NUMBER && ImpulseRadius > KINDA_SMALL_NUMBER)
		{
			PrimitiveComponent->AddRadialImpulse(ImpulseOrigin, ImpulseRadius, ImpulseStrength, RIF_Linear, bImpulseVelChange);
		}
	}
}

bool URemainBreakableSwapComponent::ActivateChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const
{
	const bool bLaunched = LaunchChaosGeometryCollections(Actor, ImpulseOrigin);
	const bool bBrokenChaos = BreakChaosGeometryCollections(Actor, ImpulseOrigin);
	return bLaunched || bBrokenChaos;
}

bool URemainBreakableSwapComponent::LaunchChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	TInlineComponentArray<UGeometryCollectionComponent*> GeometryCollectionComponents;
	Actor->GetComponents(GeometryCollectionComponents);
	if (GeometryCollectionComponents.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] No GeometryCollectionComponent found on %s"), *GetNameSafe(Actor));
		DebugBreakableMessage(FString::Printf(TEXT("No GeometryCollectionComponent found on %s"), *GetNameSafe(Actor)));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] Launching Chaos actor %s with %d GeometryCollectionComponent(s)"), *GetNameSafe(Actor), GeometryCollectionComponents.Num());
	DebugBreakableMessage(FString::Printf(TEXT("Launching Chaos actor %s with %d GC components"), *GetNameSafe(Actor), GeometryCollectionComponents.Num()));

	for (UGeometryCollectionComponent* GeometryCollectionComponent : GeometryCollectionComponents)
	{
		if (!IsValid(GeometryCollectionComponent))
		{
			continue;
		}

		LogGeometryCollectionState(TEXT("BeforeLaunch"), Actor, GeometryCollectionComponent);

		if (bForceBrokenActorsMovable && GeometryCollectionComponent->Mobility != EComponentMobility::Movable)
		{
			GeometryCollectionComponent->SetMobility(EComponentMobility::Movable);
		}

		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);
		Actor->SetActorTickEnabled(true);

		if (!GeometryCollectionComponent->IsRegistered())
		{
			GeometryCollectionComponent->RegisterComponent();
		}

		GeometryCollectionComponent->SetVisibility(true, true);
		GeometryCollectionComponent->SetHiddenInGame(false, true);
		GeometryCollectionComponent->SetComponentTickEnabled(true);
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (bForceBrokenCollisionBlockAll)
		{
			GeometryCollectionComponent->SetCollisionObjectType(ECC_PhysicsBody);
			GeometryCollectionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		}

		if (bActivateChaosComponentsOnLaunch)
		{
			GeometryCollectionComponent->Activate(true);
		}

		if (bRecreateChaosPhysicsStateOnLaunch)
		{
			GeometryCollectionComponent->RecreatePhysicsState();
		}

		GeometryCollectionComponent->SetSimulatePhysics(true);
		GeometryCollectionComponent->SetEnableGravity(true);

		if (bRemoveChaosAnchors)
		{
			GeometryCollectionComponent->RemoveAllAnchors();
		}

		if (bSetGeometryCollectionsDynamic)
		{
			GeometryCollectionComponent->SetDynamicState(Chaos::EObjectStateType::Dynamic);
		}

		if (bWakeChaosRigidBodiesOnLaunch)
		{
			GeometryCollectionComponent->WakeAllRigidBodies();
		}

		LogGeometryCollectionState(TEXT("AfterLaunchSetup"), Actor, GeometryCollectionComponent);

		if (bApplyDirectionalLaunchImpulse && DirectionalLaunchImpulseStrength > KINDA_SMALL_NUMBER)
		{
			const FVector LaunchDirection = DirectionalLaunchImpulse.GetSafeNormal();
			if (!LaunchDirection.IsNearlyZero())
			{
				GeometryCollectionComponent->AddImpulse(LaunchDirection * DirectionalLaunchImpulseStrength, NAME_None, bImpulseVelChange);
				GeometryCollectionComponent->AddImpulseAtLocation(
					LaunchDirection * DirectionalLaunchImpulseStrength,
					GeometryCollectionComponent->GetComponentLocation(),
					NAME_None);
			}
		}

		if (bApplyRadialImpulse && FMath::Abs(ImpulseStrength) > KINDA_SMALL_NUMBER && ImpulseRadius > KINDA_SMALL_NUMBER)
		{
			GeometryCollectionComponent->AddRadialImpulse(ImpulseOrigin, ImpulseRadius, ImpulseStrength, RIF_Linear, bImpulseVelChange);
		}

		if (bWakeChaosRigidBodiesOnLaunch)
		{
			GeometryCollectionComponent->WakeAllRigidBodies();
		}

		LogGeometryCollectionState(TEXT("AfterImpulse"), Actor, GeometryCollectionComponent);
	}

	return true;
}

bool URemainBreakableSwapComponent::BreakChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	TInlineComponentArray<UGeometryCollectionComponent*> GeometryCollectionComponents;
	Actor->GetComponents(GeometryCollectionComponents);
	if (GeometryCollectionComponents.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemainBreakable] No GeometryCollectionComponent to break on %s"), *GetNameSafe(Actor));
		DebugBreakableMessage(FString::Printf(TEXT("No GeometryCollectionComponent to break on %s"), *GetNameSafe(Actor)));
		return false;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RemainBreakable] Breaking Chaos actor %s | ItemIndex=%d | Strain=%.1f | Radius=%.1f"),
		*GetNameSafe(Actor),
		ChaosStrainItemIndex,
		ChaosExternalStrain,
		ChaosStrainRadius);

	DebugBreakableMessage(FString::Printf(
		TEXT("Breaking Chaos actor %s | ItemIndex=%d | Strain=%.1f | Radius=%.1f"),
		*GetNameSafe(Actor),
		ChaosStrainItemIndex,
		ChaosExternalStrain,
		ChaosStrainRadius));

	for (UGeometryCollectionComponent* GeometryCollectionComponent : GeometryCollectionComponents)
	{
		if (!IsValid(GeometryCollectionComponent))
		{
			continue;
		}

		LogGeometryCollectionState(TEXT("BeforeBreak"), Actor, GeometryCollectionComponent);

		if (bApplyChaosExternalStrain && ChaosExternalStrain > KINDA_SMALL_NUMBER)
		{
			const float Radius = ChaosStrainRadius > KINDA_SMALL_NUMBER ? ChaosStrainRadius : ImpulseRadius;
			GeometryCollectionComponent->ApplyExternalStrain(
				ChaosStrainItemIndex,
				ImpulseOrigin,
				Radius,
				ChaosStrainPropagationDepth,
				ChaosStrainPropagationFactor,
				ChaosExternalStrain);
		}

		if (bCrumbleActiveChaosClusters)
		{
			GeometryCollectionComponent->CrumbleActiveClusters();
		}

		if (bCrumbleChaosClusterByIndex)
		{
			GeometryCollectionComponent->CrumbleCluster(ChaosStrainItemIndex);
		}

		if (bWakeChaosRigidBodiesOnLaunch)
		{
			GeometryCollectionComponent->WakeAllRigidBodies();
		}

		LogGeometryCollectionState(TEXT("AfterBreak"), Actor, GeometryCollectionComponent);
	}

	return true;
}

void URemainBreakableSwapComponent::TriggerSingleVisibleChaosActorBreak(const FVector& ImpulseOrigin)
{
	PendingChaosBreakActors.Reset();

	for (AActor* IntactActor : IntactActors)
	{
		if (bHideIntactActorsInSingleChaosMode)
		{
			SetActorBreakableEnabled(IntactActor, false);
		}
	}

	for (AActor* BrokenActor : BrokenActors)
	{
		if (!IsValid(BrokenActor))
		{
			continue;
		}

		if (bUseScriptedChaosLaunchBeforeBreak && ScriptedChaosLaunchDuration > KINDA_SMALL_NUMBER)
		{
			if (PrepareChaosActorForScriptedLaunch(BrokenActor))
			{
				StartScriptedChaosLaunch(BrokenActor);
				continue;
			}
		}

		SetActorBreakableEnabled(BrokenActor, true);
		if (LaunchChaosGeometryCollections(BrokenActor, ImpulseOrigin))
		{
			PendingChaosBreakActors.Add(BrokenActor);
		}
	}

	if (ChaosBreakDelay <= KINDA_SMALL_NUMBER)
	{
		FinishSingleVisibleChaosActorBreak();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChaosBreakTimerHandle);
		World->GetTimerManager().SetTimer(
			ChaosBreakTimerHandle,
			this,
			&URemainBreakableSwapComponent::FinishSingleVisibleChaosActorBreak,
			ChaosBreakDelay,
			false);
	}
}

void URemainBreakableSwapComponent::FinishSingleVisibleChaosActorBreak()
{
	const FVector ImpulseOrigin = ResolveImpulseOrigin();
	for (const TWeakObjectPtr<AActor>& ActorPtr : PendingChaosBreakActors)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			BreakChaosGeometryCollections(Actor, ImpulseOrigin);
		}
	}

	PendingChaosBreakActors.Reset();
}

void URemainBreakableSwapComponent::DebugBreakableMessage(const FString& Message) const
{
	if (!bDebugBreakable)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[RemainBreakable] %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, FString::Printf(TEXT("RemainBreakable: %s"), *Message));
	}
}

void URemainBreakableSwapComponent::LogGeometryCollectionState(
	const TCHAR* Phase,
	const AActor* Actor,
	const UGeometryCollectionComponent* GeometryCollectionComponent) const
{
	if (!bDebugBreakable || !IsValid(GeometryCollectionComponent))
	{
		return;
	}

	const FVector ActorLocation = IsValid(Actor) ? Actor->GetActorLocation() : FVector::ZeroVector;
	const FVector ComponentLocation = GeometryCollectionComponent->GetComponentLocation();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[RemainBreakable] GCState %s | Actor=%s Hidden=%s Collision=%s | Comp=%s Registered=%s Active=%s Visible=%s Sim=%s Collision=%s Mobility=%s Loc=%s CompLoc=%s"),
		Phase,
		*GetNameSafe(Actor),
		IsValid(Actor) && Actor->IsHidden() ? TEXT("true") : TEXT("false"),
		IsValid(Actor) && Actor->GetActorEnableCollision() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GeometryCollectionComponent),
		GeometryCollectionComponent->IsRegistered() ? TEXT("true") : TEXT("false"),
		GeometryCollectionComponent->IsActive() ? TEXT("true") : TEXT("false"),
		GeometryCollectionComponent->IsVisible() ? TEXT("true") : TEXT("false"),
		GeometryCollectionComponent->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"),
		ToCollisionEnabledString(GeometryCollectionComponent->GetCollisionEnabled()),
		ToMobilityString(GeometryCollectionComponent->Mobility),
		*ActorLocation.ToCompactString(),
		*ComponentLocation.ToCompactString());
}

FVector URemainBreakableSwapComponent::ResolveImpulseOrigin() const
{
	if (IsValid(ImpulseOriginActor))
	{
		return ImpulseOriginActor->GetActorLocation() + ImpulseOriginOffset;
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation() + ImpulseOriginOffset;
	}

	return ImpulseOriginOffset;
}
