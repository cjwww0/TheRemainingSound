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
	float NurseVisibleDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P12")
	bool bRestoreFlickerLightsAfterEvent = false;

	UPROPERTY(BlueprintReadOnly, Category="Remain|P12")
	bool bTriggered = false;

	UFUNCTION(BlueprintCallable, Category="Remain|P12")
	bool TriggerCorridorEncounter();

	UFUNCTION(BlueprintCallable, Category="Remain|P12")
	void ResetCorridorEncounter();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle SilenceTimerHandle;
	FTimerHandle HideTimerHandle;

	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ShowNurseAndStrongLight();
	void HideNurseAndLights();
	void SetActorLightsEnabled(const TArray<TObjectPtr<AActor>>& Actors, bool bEnabled) const;
	void SetActorVisible(AActor* Actor, bool bVisible) const;
	bool IsPlayerActor(const AActor* Actor) const;
};
