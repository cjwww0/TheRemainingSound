#include "Puzzle/RemainWorkshopLightFaultController.h"

#include "Components/LightComponent.h"
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
		return false;
	}

	bTriggeredFaultState = true;
	SetLightsEnabled(false);

	if (ElectricBuzzSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ElectricBuzzSound, GetActorLocation());
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FaultFlashTimerHandle);
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
			if (ControlledLights[Index].bToggleVisibility)
			{
				LightComponent->SetVisibility(true);
			}

			if (ControlledLights[Index].FaultIntensity > 0.0f)
			{
				LightComponent->SetIntensity(ControlledLights[Index].FaultIntensity);
			}
		}
	}
}

bool ARemainWorkshopLightFaultController::HasTriggeredFaultState() const
{
	return bTriggeredFaultState;
}

void ARemainWorkshopLightFaultController::FinishFaultFlash()
{
	ApplyFaultState();
}

void ARemainWorkshopLightFaultController::SetLightsEnabled(bool bEnabled)
{
	for (int32 Index = 0; Index < ControlledLights.Num(); ++Index)
	{
		if (ULightComponent* LightComponent = ResolveLightComponent(ControlledLights[Index]))
		{
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
