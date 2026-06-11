#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemainBreakableSwapComponent.generated.h"

class UStaticMesh;
class AStaticMeshActor;

UCLASS(ClassGroup=(Remain), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainBreakableSwapComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainBreakableSwapComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable")
	TArray<TObjectPtr<AActor>> IntactActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable")
	TArray<TObjectPtr<AActor>> BrokenActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable")
	bool bPrepareBrokenActorsOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable")
	bool bDisableIntactActorsOnBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact")
	bool bSpawnRuntimeIntactOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact", meta=(EditCondition="bSpawnRuntimeIntactOnBeginPlay"))
	TObjectPtr<UStaticMesh> RuntimeIntactMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact", meta=(EditCondition="bSpawnRuntimeIntactOnBeginPlay"))
	FVector RuntimeIntactRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact", meta=(EditCondition="bSpawnRuntimeIntactOnBeginPlay"))
	FRotator RuntimeIntactRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact", meta=(EditCondition="bSpawnRuntimeIntactOnBeginPlay"))
	FVector RuntimeIntactScale = FVector(0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Intact", meta=(EditCondition="bSpawnRuntimeIntactOnBeginPlay"))
	bool bHideConfiguredIntactActorsWhenRuntimeIntact = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Fragments")
	bool bSpawnRuntimeFragmentsOnBreak = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Fragments", meta=(EditCondition="bSpawnRuntimeFragmentsOnBreak"))
	TObjectPtr<UStaticMesh> RuntimeFragmentMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Fragments", meta=(EditCondition="bSpawnRuntimeFragmentsOnBreak", ClampMin="1", ClampMax="24"))
	int32 RuntimeFragmentCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Fragments", meta=(EditCondition="bSpawnRuntimeFragmentsOnBreak", ClampMin="0.0"))
	float RuntimeFragmentSpreadRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Runtime Fragments", meta=(EditCondition="bSpawnRuntimeFragmentsOnBreak"))
	FVector RuntimeFragmentScale = FVector(0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bEnablePhysicsOnBrokenActors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bForceBrokenActorsMovable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bApplyRadialImpulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	TObjectPtr<AActor> ImpulseOriginActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	FVector ImpulseOriginOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics", meta=(ClampMin="0.0"))
	float ImpulseRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	float ImpulseStrength = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bImpulseVelChange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bForceBrokenCollisionBlockAll = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	bool bApplyDirectionalLaunchImpulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics")
	FVector DirectionalLaunchImpulse = FVector(1.0f, 0.0f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Physics", meta=(ClampMin="0.0"))
	float DirectionalLaunchImpulseStrength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bUseChaosGeometryCollections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bUseSingleVisibleChaosActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bHideIntactActorsInSingleChaosMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bUseScriptedChaosLaunchBeforeBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0.0"))
	float ScriptedChaosLaunchDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	float ScriptedChaosLaunchDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	float ScriptedChaosLaunchArcHeight = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	FRotator ScriptedChaosLaunchRotation = FRotator(25.0f, 0.0f, 70.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0.0"))
	float ChaosBreakDelay = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bSetGeometryCollectionsDynamic = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bActivateChaosComponentsOnLaunch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bRecreateChaosPhysicsStateOnLaunch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bWakeChaosRigidBodiesOnLaunch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bRemoveChaosAnchors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bCrumbleActiveChaosClusters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bCrumbleChaosClusterByIndex = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	bool bApplyChaosExternalStrain = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos")
	int32 ChaosStrainItemIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0.0"))
	float ChaosExternalStrain = 5000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0.0"))
	float ChaosStrainRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0"))
	int32 ChaosStrainPropagationDepth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Chaos", meta=(ClampMin="0.0"))
	float ChaosStrainPropagationFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Breakable|Debug")
	bool bDebugBreakable = false;

	UFUNCTION(BlueprintCallable, Category="Remain|Breakable")
	void PrepareBreakableState();

	UFUNCTION(BlueprintCallable, Category="Remain|Breakable")
	bool TriggerBreak();

	UFUNCTION(BlueprintPure, Category="Remain|Breakable")
	bool HasBroken() const;

protected:
	UPROPERTY(Transient)
	bool bBroken = false;

private:
	struct FPendingScriptedChaosLaunch
	{
		TWeakObjectPtr<AActor> Actor;
		FVector StartLocation = FVector::ZeroVector;
		FVector TargetLocation = FVector::ZeroVector;
		FQuat StartRotation = FQuat::Identity;
		FQuat TargetRotation = FQuat::Identity;
		float Elapsed = 0.0f;
	};

	void SetActorBreakableEnabled(AActor* Actor, bool bEnabled) const;
	void SpawnRuntimeIntactActor();
	bool PrepareChaosActorForScriptedLaunch(AActor* Actor) const;
	void StartScriptedChaosLaunch(AActor* Actor);
	void FinishScriptedChaosLaunch(AActor* Actor);
	bool SpawnRuntimeFragments(const FVector& ImpulseOrigin) const;
	void ActivateBrokenActor(AActor* Actor, const FVector& ImpulseOrigin) const;
	bool ActivateChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const;
	bool LaunchChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const;
	bool BreakChaosGeometryCollections(AActor* Actor, const FVector& ImpulseOrigin) const;
	void TriggerSingleVisibleChaosActorBreak(const FVector& ImpulseOrigin);
	UFUNCTION()
	void FinishSingleVisibleChaosActorBreak();
	void DebugBreakableMessage(const FString& Message) const;
	void LogGeometryCollectionState(const TCHAR* Phase, const AActor* Actor, const UGeometryCollectionComponent* GeometryCollectionComponent) const;
	FVector ResolveImpulseOrigin() const;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> PendingChaosBreakActors;

	UPROPERTY(Transient)
	TObjectPtr<AStaticMeshActor> RuntimeIntactActor = nullptr;

	TArray<FPendingScriptedChaosLaunch> PendingScriptedChaosLaunches;

	FTimerHandle ChaosBreakTimerHandle;
};
