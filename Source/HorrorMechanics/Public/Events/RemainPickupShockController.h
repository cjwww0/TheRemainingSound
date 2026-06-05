#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainPickupShockController.generated.h"

class UAudioComponent;
class UCameraComponent;
class UCameraShakeBase;
class ARemainNurseEncounterActor;
class URemainBreakableSwapComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemainPickupShockSimpleSignature);

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainPickupShockController : public AActor
{
	GENERATED_BODY()

public:
	ARemainPickupShockController();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P10", meta=(AllowPrivateAccess="true"))
	TObjectPtr<URemainBreakableSwapComponent> BreakableCup = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Trigger")
	TObjectPtr<AActor> TriggerPickupActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Trigger")
	FName TriggerPartId = TEXT("Part01");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Trigger")
	bool bAutoBindInventoryItemAdded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Trigger", meta=(ClampMin="0.05"))
	float InventoryBindRetryInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Trigger", meta=(ClampMin="0"))
	int32 MaxInventoryBindAttempts = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Audio")
	TArray<TObjectPtr<AActor>> AmbientAudioActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MutedAmbientVolume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Audio")
	bool bRestoreAmbientAfterImpact = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Audio")
	TObjectPtr<USoundBase> GlassBreakSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Audio")
	FVector GlassBreakLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Lighting")
	TArray<TObjectPtr<AActor>> LightsToExtinguish;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Lighting")
	bool bHideExtinguishedLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Lighting", meta=(ClampMin="0.0"))
	float ExtinguishedLightIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Timing", meta=(ClampMin="0.0"))
	float SilenceDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen", meta=(ClampMin="0.0"))
	float ScreenFlashDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen")
	bool bUseDesaturationFlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FlashPostProcessBlendWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen")
	bool bUseWhiteCameraFade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WhiteFadeAlpha = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P10|Screen", meta=(ClampMin="0.0"))
	float CameraShakeScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11")
	TObjectPtr<ARemainNurseEncounterActor> NurseEncounter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11")
	bool bTriggerNurseEncounterOnSequenceCompleted = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11", meta=(ClampMin="0.0"))
	float NurseEncounterDelay = 0.0f;

	UPROPERTY(BlueprintAssignable, Category="Remain|P10")
	FRemainPickupShockSimpleSignature OnImpactTriggered;

	UPROPERTY(BlueprintAssignable, Category="Remain|P10")
	FRemainPickupShockSimpleSignature OnSequenceCompleted;

	UFUNCTION(BlueprintCallable, Category="Remain|P10")
	bool TriggerShockSequence();

	UFUNCTION(BlueprintCallable, Category="Remain|P10")
	bool NotifyWorkbenchPickupCollected(AActor* PickupActor);

	UFUNCTION(BlueprintCallable, Category="Remain|P10")
	bool NotifyInventoryItemAdded(UObject* InventoryItem);

	UFUNCTION(BlueprintPure, Category="Remain|P10")
	bool HasTriggered() const;

protected:
	UPROPERTY(Transient)
	bool bTriggered = false;

private:
	UFUNCTION()
	void RetryBindInventoryItemAdded();

	UFUNCTION()
	void HandleInventoryItemAdded(UObject* InventoryItem);

	UFUNCTION()
	void HandleImpact();

	UFUNCTION()
	void FinishScreenFlash();

	UFUNCTION()
	void TriggerLinkedNurseEncounter();

	bool TryBindInventoryItemAdded();
	bool DoesPickupMatch(AActor* PickupActor) const;
	bool DoesItemMatch(UObject* Item) const;
	void MuteAmbientAudio();
	void RestoreAmbientAudio();
	void ExtinguishConfiguredLights();
	void StartScreenEffects();
	void RestoreScreenEffects();

	UPROPERTY(Transient)
	TObjectPtr<UObject> BoundInventory = nullptr;

	TArray<TWeakObjectPtr<UAudioComponent>> MutedAudioComponents;
	TArray<float> OriginalAudioVolumes;

	TWeakObjectPtr<UCameraComponent> ActiveFlashCamera;
	bool bSavedColorSaturationOverride = false;
	FVector4 SavedColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	float SavedPostProcessBlendWeight = 0.0f;

	int32 InventoryBindAttemptCount = 0;

	FTimerHandle InventoryBindRetryTimerHandle;
	FTimerHandle SilenceTimerHandle;
	FTimerHandle ScreenFlashTimerHandle;
	FTimerHandle NurseEncounterTimerHandle;
};
