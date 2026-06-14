#include "Components/RemainScreenEffectsBridgeComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	static APlayerController* ResolvePlayerControllerFromOwner(AActor* Owner)
	{
		if (!IsValid(Owner))
		{
			return nullptr;
		}

		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			return Cast<APlayerController>(Pawn->GetController());
		}

		return Cast<APlayerController>(Owner->GetInstigatorController());
	}

	static UCameraComponent* ResolveFirstPersonCameraFromOwner(AActor* Owner)
	{
		if (!IsValid(Owner))
		{
			return nullptr;
		}

		for (UActorComponent* Component : Owner->GetComponents())
		{
			if (UCameraComponent* CameraComponent = Cast<UCameraComponent>(Component))
			{
				if (CameraComponent->GetName().Contains(TEXT("FirstPersonCamera")))
				{
					return CameraComponent;
				}
			}
		}

		return Owner->FindComponentByClass<UCameraComponent>();
	}
}

URemainScreenEffectsBridgeComponent::URemainScreenEffectsBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URemainScreenEffectsBridgeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOpeningBlurOnBeginPlay && !bWaitForOpeningSequence)
	{
		if (OpeningBlurDelay <= 0.0f)
		{
			StartOpeningBlur();
		}
		else if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(
				TimerHandle,
				this,
				&URemainScreenEffectsBridgeComponent::StartOpeningBlur,
				OpeningBlurDelay,
				false);
		}
	}
}

void URemainScreenEffectsBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bOpeningBlurActive || OpeningBlurDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UCameraComponent* CameraComponent = ResolveFirstPersonCameraFromOwner(Owner);
	if (!CameraComponent)
	{
		return;
	}

	OpeningBlurElapsed += DeltaTime;
	const float Progress = FMath::Clamp(OpeningBlurElapsed / OpeningBlurDuration, 0.0f, 1.0f);
	const float EasedProgress = FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f);

	FPostProcessSettings& PPS = CameraComponent->PostProcessSettings;
	PPS.bOverride_DepthOfFieldFstop = true;
	PPS.bOverride_DepthOfFieldFocalDistance = true;
	PPS.bOverride_DepthOfFieldSensorWidth = true;
	PPS.bOverride_DepthOfFieldMinFstop = true;
	PPS.DepthOfFieldFocalDistance = FMath::Lerp(50.0f, OriginalDepthOfFieldFocalDistance, EasedProgress);
	PPS.DepthOfFieldFstop = FMath::Lerp(OpeningBlurFstop, OriginalDepthOfFieldFstop, EasedProgress);
	PPS.DepthOfFieldMinFstop = FMath::Min(OpeningBlurFstop, OriginalDepthOfFieldFstop);
	PPS.DepthOfFieldSensorWidth = 50.0f;
	CameraComponent->PostProcessBlendWeight = FMath::Lerp(OpeningBlurStartAlpha, OriginalPostProcessBlendWeight, EasedProgress);

	if (Progress >= 1.0f)
	{
		bOpeningBlurActive = false;
		SetComponentTickEnabled(false);

		PPS.bOverride_DepthOfFieldFstop = bPostProcessOverridden;
		PPS.bOverride_DepthOfFieldFocalDistance = false;
		PPS.bOverride_DepthOfFieldSensorWidth = false;
		PPS.bOverride_DepthOfFieldMinFstop = false;
		PPS.DepthOfFieldFstop = OriginalDepthOfFieldFstop;
		PPS.DepthOfFieldFocalDistance = OriginalDepthOfFieldFocalDistance;
		CameraComponent->PostProcessBlendWeight = OriginalPostProcessBlendWeight;
	}
}

void URemainScreenEffectsBridgeComponent::StartOpeningBlur()
{
	AActor* Owner = GetOwner();
	UCameraComponent* CameraComponent = ResolveFirstPersonCameraFromOwner(Owner);
	if (!CameraComponent)
	{
		return;
	}

	FPostProcessSettings& PPS = CameraComponent->PostProcessSettings;
	bPostProcessOverridden = PPS.bOverride_DepthOfFieldFstop;
	OriginalDepthOfFieldFstop = PPS.DepthOfFieldFstop > 0.0f ? PPS.DepthOfFieldFstop : 22.0f;
	OriginalDepthOfFieldFocalDistance = PPS.DepthOfFieldFocalDistance > 0.0f ? PPS.DepthOfFieldFocalDistance : OpeningBlurFocalDistance;
	OriginalPostProcessBlendWeight = CameraComponent->PostProcessBlendWeight;

	OpeningBlurElapsed = 0.0f;
	bOpeningBlurActive = true;
	SetComponentTickEnabled(true);
}

void URemainScreenEffectsBridgeComponent::StartOpeningBlinkAndBlur()
{
	if (!bUseOpeningBlink)
	{
		StartOpeningBlur();
		return;
	}

	APlayerController* PlayerController = ResolvePlayerControllerFromOwner(GetOwner());
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(
			1.0f,
			1.0f,
			0.0f,
			FLinearColor::Black,
			false,
			true);
	}

	if (OpeningEyeClosedDuration <= KINDA_SMALL_NUMBER)
	{
		BeginOpeningEyeOpenAndBlur();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OpeningBlinkTimerHandle);
		World->GetTimerManager().SetTimer(
			OpeningBlinkTimerHandle,
			this,
			&URemainScreenEffectsBridgeComponent::BeginOpeningEyeOpenAndBlur,
			OpeningEyeClosedDuration,
			false);
	}
}

void URemainScreenEffectsBridgeComponent::BeginOpeningEyeOpenAndBlur()
{
	StartOpeningBlur();

	APlayerController* PlayerController = ResolvePlayerControllerFromOwner(GetOwner());
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(
			1.0f,
			0.0f,
			OpeningEyeOpenDuration,
			FLinearColor::Black,
			false,
			false);
	}
}
