#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainWorkshopLightFaultController.generated.h"

class ULightComponent;
class USoundBase;

USTRUCT(BlueprintType)
struct FRemainFaultLightConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting")
	TObjectPtr<AActor> LightActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting")
	float FaultIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting")
	bool bToggleVisibility = true;
};

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainWorkshopLightFaultController : public AActor
{
	GENERATED_BODY()

public:
	ARemainWorkshopLightFaultController();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting")
	TArray<FRemainFaultLightConfig> ControlledLights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting", meta=(ClampMin="0.1"))
	float FlashDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting", meta=(ClampMin="0.0"))
	float FaultPulseDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting", meta=(ClampMin="1.0"))
	float FaultPulseIntensityMultiplier = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting", meta=(ClampMin="0.0"))
	float MinimumFaultPulseIntensity = 25000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Lighting")
	TObjectPtr<USoundBase> ElectricBuzzSound = nullptr;

	UFUNCTION(BlueprintCallable, Category="Remain|Lighting")
	bool TriggerFaultState();

	UFUNCTION(BlueprintCallable, Category="Remain|Lighting")
	void ApplyFaultState();

	UFUNCTION(BlueprintPure, Category="Remain|Lighting")
	bool HasTriggeredFaultState() const;

protected:
	UPROPERTY(Transient)
	bool bTriggeredFaultState = false;

private:
	UFUNCTION()
	void FinishFaultPulse();

	UFUNCTION()
	void FinishFaultFlash();

	void ApplyFaultPulseState();
	void SetLightsEnabled(bool bEnabled);
	void SnapshotLightDefaults();
	ULightComponent* ResolveLightComponent(const FRemainFaultLightConfig& Config) const;

	UPROPERTY(Transient)
	TArray<float> DefaultIntensities;

	UPROPERTY(Transient)
	TArray<bool> DefaultVisibility;

	FTimerHandle FaultPulseTimerHandle;
	FTimerHandle FaultFlashTimerHandle;
};
