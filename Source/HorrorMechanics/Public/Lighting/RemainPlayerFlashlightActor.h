#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainPlayerFlashlightActor.generated.h"

class USceneComponent;
class USpotLightComponent;
class UCameraComponent;

UCLASS(Blueprintable)
class HORRORMECHANICS_API ARemainPlayerFlashlightActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainPlayerFlashlightActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Flashlight")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Remain|Flashlight")
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	bool bStartEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	bool bBindToggleInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	FName ToggleActionName = TEXT("FlashlightToggle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="0.0"))
	float Intensity = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="1.0"))
	float AttenuationRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="0.0", ClampMax="89.0"))
	float InnerConeAngle = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="0.1", ClampMax="89.0"))
	float OuterConeAngle = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	FLinearColor LightColor = FLinearColor(1.0f, 0.88f, 0.68f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	bool bCastFlashlightShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	FVector CameraRelativeLocation = FVector(10.0f, 0.0f, -4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight")
	FRotator CameraRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="0.05"))
	float AttachRetryDelay = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Flashlight", meta=(ClampMin="0"))
	int32 MaxAttachRetries = 20;

	UFUNCTION(BlueprintCallable, Category="Remain|Flashlight")
	void SetFlashlightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Remain|Flashlight")
	void ToggleFlashlight();

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle AttachRetryTimerHandle;
	int32 AttachRetryCount = 0;
	bool bAttachedToCamera = false;

	void ConfigureFlashlight();
	void TryAttachToPlayerCamera();
	void BindToggleInput();
	UCameraComponent* ResolvePlayerCamera() const;
};
