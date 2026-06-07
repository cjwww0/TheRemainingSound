#include "Puzzle/RemainWorkshopLightFaultController.h"

#include "Components/LightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ARemainWorkshopLightFaultController::ARemainWorkshopLightFaultController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARemainWorkshopLightFaultController::BeginPlay()
{
	Super::BeginPlay();
	SnapshotLightDefaults();
}

bool ARemainWorkshopLightFaultController::TriggerFaultState()
{
	if (bTriggeredFaultState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[P15Fault] TriggerFaultState ignored because it already triggered | Controller=%s"), *GetNameSafe(this));
		return false;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[P15Fault] TriggerFaultState called | Controller=%s | Lights=%d | FlashDuration=%.2f"),
		*GetNameSafe(this),
		ControlledLights.Num(),
		FlashDuration);

#if !(UE_BUILD_SHIPPING)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("[P15] Light fault triggered"));
	}
#endif

	bTriggeredFaultState = true;
	ApplyFaultPulseState();

	if (ElectricBuzzSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ElectricBuzzSound, GetActorLocation());
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FaultPulseTimerHandle);
		World->GetTimerManager().ClearTimer(FaultFlashTimerHandle);

		if (FaultPulseDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(FaultPulseTimerHandle, this, &ARemainWorkshopLightFaultController::FinishFaultPulse, FaultPulseDuration, false);
		}
		else
		{
			FinishFaultPulse();
		}

		World->GetTimerManager().SetTimer(FaultFlashTimerHandle, this, &ARemainWorkshopLightFaultController::FinishFaultFlash, FlashDuration, false);
	}

	return true;
}

void ARemainWorkshopLightFaultController::ApplyFaultState()
{
	bTriggeredFaultState = true;

	for (int32 Index = 0; Index < ControlledLights.Num(); ++Index)
	{
		if (ULightComponent* LightComponent = ResolveLightComponent(ControlledLights[Index]))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] ApplyFaultState light[%d] | Actor=%s | ToggleVisibility=%s | FaultIntensity=%.1f | BeforeVisible=%s | BeforeIntensity=%.1f"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor),
				ControlledLights[Index].bToggleVisibility ? TEXT("true") : TEXT("false"),
				ControlledLights[Index].FaultIntensity,
				LightComponent->IsVisible() ? TEXT("true") : TEXT("false"),
				LightComponent->Intensity);

			if (ControlledLights[Index].bToggleVisibility)
			{
				LightComponent->SetVisibility(true);
			}

			if (ControlledLights[Index].FaultIntensity > 0.0f)
			{
				LightComponent->SetIntensity(ControlledLights[Index].FaultIntensity);
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] ApplyFaultState light[%d] after | Visible=%s | Intensity=%.1f"),
				Index,
				LightComponent->IsVisible() ? TEXT("true") : TEXT("false"),
				LightComponent->Intensity);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] ApplyFaultState light[%d] missing light component | Actor=%s"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor));
		}
	}
}

bool ARemainWorkshopLightFaultController::HasTriggeredFaultState() const
{
	return bTriggeredFaultState;
}

void ARemainWorkshopLightFaultController::FinishFaultPulse()
{
	SetLightsEnabled(false);
}

void ARemainWorkshopLightFaultController::FinishFaultFlash()
{
	ApplyFaultState();
}

void ARemainWorkshopLightFaultController::ApplyFaultPulseState()
{
	for (int32 Index = 0; Index < ControlledLights.Num(); ++Index)
	{
		if (ULightComponent* LightComponent = ResolveLightComponent(ControlledLights[Index]))
		{
			const float BaselineIntensity = DefaultIntensities.IsValidIndex(Index) ? DefaultIntensities[Index] : LightComponent->Intensity;
			const float PulseIntensity = FMath::Max(BaselineIntensity * FaultPulseIntensityMultiplier, MinimumFaultPulseIntensity);

			if (ControlledLights[Index].bToggleVisibility)
			{
				LightComponent->SetVisibility(true);
			}

			LightComponent->SetIntensity(PulseIntensity);

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] ApplyFaultPulseState light[%d] | Actor=%s | PulseIntensity=%.1f | Baseline=%.1f"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor),
				PulseIntensity,
				BaselineIntensity);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] ApplyFaultPulseState light[%d] missing light component | Actor=%s"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor));
		}
	}
}

void ARemainWorkshopLightFaultController::SetLightsEnabled(bool bEnabled)
{
	for (int32 Index = 0; Index < ControlledLights.Num(); ++Index)
	{
		if (ULightComponent* LightComponent = ResolveLightComponent(ControlledLights[Index]))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] SetLightsEnabled(%s) light[%d] | Actor=%s | BeforeVisible=%s | BeforeIntensity=%.1f"),
				bEnabled ? TEXT("true") : TEXT("false"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor),
				LightComponent->IsVisible() ? TEXT("true") : TEXT("false"),
				LightComponent->Intensity);

			if (ControlledLights[Index].bToggleVisibility)
			{
				LightComponent->SetVisibility(bEnabled);
			}

			if (bEnabled)
			{
				if (DefaultIntensities.IsValidIndex(Index))
				{
					LightComponent->SetIntensity(DefaultIntensities[Index]);
				}
			}
			else
			{
				LightComponent->SetIntensity(0.0f);
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] SetLightsEnabled(%s) light[%d] after | Visible=%s | Intensity=%.1f"),
				bEnabled ? TEXT("true") : TEXT("false"),
				Index,
				LightComponent->IsVisible() ? TEXT("true") : TEXT("false"),
				LightComponent->Intensity);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[P15Fault] SetLightsEnabled(%s) light[%d] missing light component | Actor=%s"),
				bEnabled ? TEXT("true") : TEXT("false"),
				Index,
				*GetNameSafe(ControlledLights[Index].LightActor));
		}
	}
}

void ARemainWorkshopLightFaultController::SnapshotLightDefaults()
{
	DefaultIntensities.Reset();
	DefaultVisibility.Reset();

	for (const FRemainFaultLightConfig& Config : ControlledLights)
	{
		if (ULightComponent* LightComponent = ResolveLightComponent(Config))
		{
			DefaultIntensities.Add(LightComponent->Intensity);
			DefaultVisibility.Add(LightComponent->IsVisible());
		}
		else
		{
			DefaultIntensities.Add(0.0f);
			DefaultVisibility.Add(false);
		}
	}
}

ULightComponent* ARemainWorkshopLightFaultController::ResolveLightComponent(const FRemainFaultLightConfig& Config) const
{
	return IsValid(Config.LightActor) ? Config.LightActor->FindComponentByClass<ULightComponent>() : nullptr;
}
