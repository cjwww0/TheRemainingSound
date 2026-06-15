#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainNurseCorridorEncounterActor.generated.h"

class ARemainNurseEncounterActor;
class UBoxComponent;
class USceneComponent;
class USoundBase;

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainNurseCorridorEncounterActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainNurseCorridorEncounterActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P12")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P12")
	TObjectPtr<UBoxComponent> TriggerBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	TObjectPtr<ARemainNurseEncounterActor> NurseEncounter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	TObjectPtr<AActor> NurseActorFallback = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	TArray<TObjectPtr<AActor>> FlickerLightActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	TArray<TObjectPtr<AActor>> StrongLightActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	TObjectPtr<USoundBase> StrongLightSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12", meta=(ClampMin="0.0"))
	float SilenceDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12", meta=(ClampMin="0.0"))
	float NurseVisibleDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Reveal", meta=(ClampMin="0"))
	int32 StrongLightFlashCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Reveal", meta=(ClampMin="0.02"))
	float StrongLightFlashInterval = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Reveal")
	bool bHoldStrongLightAfterFlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	bool bRestoreFlickerLightsAfterEvent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Trigger")
	bool bRequirePickupBeforeTrigger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Trigger")
	FName RequiredPickupPartId = TEXT("Part02");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12|Debug")
	bool bDebugCorridorEncounter = true;

	UPROPERTY(BlueprintReadOnly, Category="Remain|P12")
	bool bTriggered = false;

	UPROPERTY(BlueprintReadOnly, Category="Remain|P12|Trigger")
	mutable bool bRequiredPickupCollected = false;

	UFUNCTION(BlueprintCallable, Category="Remain|P12")
	bool TriggerCorridorEncounter();

	UFUNCTION(BlueprintCallable, Category="Remain|P12")
	void ResetCorridorEncounter();

	UFUNCTION(BlueprintCallable, Category="Remain|P12|Trigger")
	bool NotifyWorkbenchPickupCollected(AActor* PickupActor);

	UFUNCTION(BlueprintCallable, Category="Remain|P12|Trigger")
	bool NotifyCollectedObject(UObject* CollectedObject);

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle SilenceTimerHandle;
	FTimerHandle HideTimerHandle;
	FTimerHandle StrongLightFlashTimerHandle;
	int32 StrongLightFlashStep = 0;
	bool bStrongLightFlashVisible = false;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ShowNurseAndStrongLight();
	void StepStrongLightFlash();
	void HideNurseAndLights();
	void ConfigureTriggerBox();
	void SetActorLightsEnabled(const TArray<TObjectPtr<AActor>>& Actors, bool bEnabled) const;
	void SetActorVisible(AActor* Actor, bool bVisible) const;
	void DebugMessage(const FString& Message) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool HasRequiredPickup() const;
	bool DoesObjectMatchRequiredPart(const UObject* CollectedObject) const;
	bool RuntimeInventoryHasRequiredPickup() const;
};
