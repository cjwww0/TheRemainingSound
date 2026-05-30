#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "RemainWorkbenchPuzzleComponent.generated.h"

class ULightComponent;
class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USoundBase;

USTRUCT(BlueprintType)
struct FRemainWorkbenchSlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	TSubclassOf<UObject> RequiredItemClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	FComponentReference SlotMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	TObjectPtr<UStaticMesh> PlacedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	FComponentReference WarmLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	TObjectPtr<USoundBase> InsertSound = nullptr;
};

USTRUCT(BlueprintType)
struct FRemainWorkbenchCheckpointState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	TArray<bool> PlacedSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	int32 CurrentUnlockedSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	int32 PlacedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	bool bFaultLightsTriggered = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRemainWorkbenchSlotPlacedSignature, int32, SlotIndex, FName, SlotId, UObject*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemainWorkbenchSimpleSignature);

UCLASS(ClassGroup=(Remain), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainWorkbenchPuzzleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainWorkbenchPuzzleComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench")
	TArray<FRemainWorkbenchSlotConfig> SlotConfigs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench", meta=(ClampMin="0.05"))
	float WarmLightDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench|Interaction")
	bool bForceVisibilityBlockOnSlotMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench|Interaction")
	bool bCreateInteractionTraceProxy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench|Interaction")
	FVector InteractionTracePadding = FVector(18.0f, 18.0f, 24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Workbench|Interaction")
	FVector InteractionTraceMinimumExtent = FVector(24.0f, 24.0f, 24.0f);

	UPROPERTY(BlueprintAssignable, Category="Remain|Workbench")
	FRemainWorkbenchSlotPlacedSignature OnSlotPlaced;

	UPROPERTY(BlueprintAssignable, Category="Remain|Workbench")
	FRemainWorkbenchSimpleSignature OnFirstPanelPlaced;

	UPROPERTY(BlueprintAssignable, Category="Remain|Workbench")
	FRemainWorkbenchSimpleSignature OnAllSlotsPlaced;

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench")
	bool CanUseItem(UObject* Item) const;

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench")
	bool PlaceItem(UObject* Item, UObject* Inventory);

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	int32 GetCurrentUnlockedSlotIndex() const;

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	int32 GetPlacedCount() const;

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	bool HasTriggeredFaultLights() const;

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	bool IsSlotPlaced(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	bool IsComplete() const;

	UFUNCTION(BlueprintPure, Category="Remain|Workbench")
	FText GetCurrentPromptText() const;

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench|Debug")
	FString BuildInventoryAcceptanceDebugString(UObject* Inventory) const;

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench")
	FRemainWorkbenchCheckpointState GetCheckpointState() const;

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench")
	void ApplyCheckpointState(const FRemainWorkbenchCheckpointState& State);

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench")
	void ResetWorkbenchState();

	UFUNCTION(BlueprintCallable, Category="Remain|Workbench|Interaction")
	void RebuildInteractionTraceProxy();

protected:
	UPROPERTY(Transient)
	TArray<bool> PlacedSlots;

	UPROPERTY(Transient)
	int32 CurrentUnlockedSlotIndex = 0;

	UPROPERTY(Transient)
	int32 PlacedCount = 0;

	UPROPERTY(Transient)
	bool bFaultLightsTriggered = false;

private:
	UFUNCTION()
	void HandleWarmLightTimeout();

	void EnsureStateArrays();
	void ConfigureInteractionCollision();
	void RefreshSlotVisuals();
	void SetWarmLightActive(int32 SlotIndex, bool bActive);
	bool RemoveInventoryItem(UObject* Inventory, UObject* Item) const;
	bool BuildInteractionLocalBounds(FBox& OutLocalBounds) const;

	UPROPERTY(Transient)
	int32 ActiveWarmLightSlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> InteractionTraceProxy = nullptr;

	FTimerHandle WarmLightTimerHandle;
};
