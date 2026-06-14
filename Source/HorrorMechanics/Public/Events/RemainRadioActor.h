#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainRadioActor.generated.h"

class UAudioComponent;
class UBoxComponent;
class USoundBase;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainRadioActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainRadioActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Radio")
	TObjectPtr<UStaticMeshComponent> RadioMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Radio")
	TObjectPtr<UBoxComponent> InteractionBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Radio")
	TObjectPtr<UAudioComponent> RadioAudio = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio")
	TObjectPtr<USoundBase> BroadcastSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AmbientVolume = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio", meta=(ClampMin="0.0", ClampMax="2.0"))
	float ActiveVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio")
	bool bStartAmbientOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio")
	bool bLockMovementWhileListening = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio|Direct Input")
	bool bEnableDirectInteractInputFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio|Direct Input")
	FName DirectInteractActionName = TEXT("Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio|Direct Input")
	FName DirectBackActionName = TEXT("Back");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Radio|Direct Input", meta=(ClampMin="50.0"))
	float DirectInteractTraceDistance = 450.0f;

	UFUNCTION(BlueprintCallable, Category="Remain|Radio")
	void ToggleRadio();

	UFUNCTION(BlueprintCallable, Category="Remain|Radio")
	void HandleRadioInteraction(APlayerController* PlayerController, UPrimitiveComponent* Component);

	UFUNCTION(BlueprintPure, Category="Remain|Radio")
	bool IsRadioInteractionAvailable(UPrimitiveComponent* Component) const;

	UFUNCTION(BlueprintPure, Category="Remain|Radio")
	bool IsRadioPlayingActiveBroadcast() const;

	UFUNCTION(BlueprintPure, Category="Remain|Radio")
	bool IsRadioListening() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> DirectInputPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ListeningPlayerController = nullptr;

	bool bListeningBroadcast = false;
	bool bDirectInputEnabled = false;
	bool bDirectInputBound = false;
	bool bDirectBackInputBound = false;
	bool bMoveInputLockedByRadio = false;

	void StartAmbientBroadcast();
	void PlayActiveBroadcast();
	void PauseBroadcast();
	void StartListening(APlayerController* PlayerController);
	void StopListening();
	void UpdateDirectInteractInputFallback();
	bool IsPlayerLookingAtRadio(APlayerController* PlayerController) const;
	void EnableDirectInteractInput(APlayerController* PlayerController);
	void DisableDirectInteractInput();
	void HandleDirectInteractInput();
	void HandleDirectBackInput();
	void LockPlayerForListening(APlayerController* PlayerController);
	void RestorePlayerAfterListening();
};
