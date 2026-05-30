#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemainScreenEffectsBridgeComponent.generated.h"

UCLASS(ClassGroup=(Remain), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainScreenEffectsBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainScreenEffectsBridgeComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Optional auto-start used for the current P02 opening blur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur")
	bool bRunOpeningBlurOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur", meta=(ClampMin="0.0"))
	float OpeningBlurDelay = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur", meta=(ClampMin="0.05"))
	float OpeningBlurDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur")
	float OpeningBlurFocalDistance = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur")
	float OpeningBlurStartAlpha = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening Blur", meta=(ClampMin="0.0"))
	float OpeningBlurFstop = 1.2f;

	UFUNCTION(BlueprintCallable, Category="Remain|Opening Blur")
	void StartOpeningBlur();

protected:
	UPROPERTY(Transient)
	bool bOpeningBlurActive = false;

	UPROPERTY(Transient)
	float OpeningBlurElapsed = 0.0f;

	UPROPERTY(Transient)
	bool bPostProcessOverridden = false;

	UPROPERTY(Transient)
	float OriginalDepthOfFieldFstop = 0.0f;

	UPROPERTY(Transient)
	float OriginalDepthOfFieldFocalDistance = 0.0f;

	UPROPERTY(Transient)
	float OriginalPostProcessBlendWeight = 0.0f;
};
