#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainNurseEncounterActor.generated.h"

class UAudioComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemainNurseEncounterSimpleSignature);

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainNurseEncounterActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainNurseEncounterActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P11", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P11", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> NurseMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P11", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> FaceFogMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|P11", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> ProximitySphere = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11")
	bool bAutoHideOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11", meta=(ClampMin="0.0"))
	float ActiveDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11", meta=(ClampMin="1.0"))
	float ProximityRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Screen", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BlackFadeAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Screen", meta=(ClampMin="0.0"))
	float BlackFadeInDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Screen", meta=(ClampMin="0.0"))
	float BlackFadeOutDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Audio")
	TObjectPtr<USoundBase> AppearSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Audio")
	TObjectPtr<USoundBase> DisappearSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Audio")
	TObjectPtr<USoundBase> TinnitusSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Audio")
	TObjectPtr<USoundBase> BreathingSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|P11|Audio")
	bool bLoopProximitySounds = true;

	UPROPERTY(BlueprintAssignable, Category="Remain|P11")
	FRemainNurseEncounterSimpleSignature OnEncounterActivated;

	UPROPERTY(BlueprintAssignable, Category="Remain|P11")
	FRemainNurseEncounterSimpleSignature OnEncounterEnded;

	UPROPERTY(BlueprintAssignable, Category="Remain|P11")
	FRemainNurseEncounterSimpleSignature OnProximityFeedbackStarted;

	UPROPERTY(BlueprintAssignable, Category="Remain|P11")
	FRemainNurseEncounterSimpleSignature OnProximityFeedbackStopped;

	UFUNCTION(BlueprintCallable, Category="Remain|P11")
	bool ActivateEncounter();

	UFUNCTION(BlueprintCallable, Category="Remain|P11")
	void EndEncounter();

	UFUNCTION(BlueprintPure, Category="Remain|P11")
	bool IsEncounterActive() const;

protected:
	UPROPERTY(Transient)
	bool bEncounterActive = false;

	UPROPERTY(Transient)
	bool bProximityFeedbackActive = false;

private:
	UFUNCTION()
	void HandleProximityBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProximityEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void SetEncounterVisible(bool bVisible);
	void StartProximityFeedback();
	void StopProximityFeedback();
	bool IsPlayerActor(const AActor* Actor) const;
	APlayerController* ResolvePlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> TinnitusAudioComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BreathingAudioComponent = nullptr;

	FTimerHandle ActiveTimerHandle;
};
