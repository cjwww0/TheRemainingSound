#include "Events/RemainNurseCorridorEncounterActor.h"

#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Events/RemainNurseEncounterActor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ARemainNurseCorridorEncounterActor::ARemainNurseCorridorEncounterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(120.0f, 120.0f, 120.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(true);
}

void ARemainNurseCorridorEncounterActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ARemainNurseCorridorEncounterActor::HandleTriggerBeginOverlap);
	}

	HideNurseAndLights();
}

bool ARemainNurseCorridorEncounterActor::TriggerCorridorEncounter()
{
	if (bTriggered)
	{
		return false;
	}

	bTriggered = true;
	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorLightsEnabled(FlickerLightActors, false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SilenceTimerHandle,
			this,
			&ARemainNurseCorridorEncounterActor::ShowNurseAndStrongLight,
			SilenceDuration,
			false);
	}
	else
	{
		ShowNurseAndStrongLight();
	}

	return true;
}

void ARemainNurseCorridorEncounterActor::ResetCorridorEncounter()
{
	bTriggered = false;
	if (TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	HideNurseAndLights();
}

void ARemainNurseCorridorEncounterActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (IsPlayerActor(OtherActor))
	{
		TriggerCorridorEncounter();
	}
}

void ARemainNurseCorridorEncounterActor::ShowNurseAndStrongLight()
{
	SetActorLightsEnabled(StrongLightActors, true);

	if (StrongLightSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StrongLightSound, GetActorLocation());
	}

	if (NurseEncounter)
	{
		NurseEncounter->ActivateEncounter();
	}
	else
	{
		SetActorVisible(NurseActorFallback, true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&ARemainNurseCorridorEncounterActor::HideNurseAndLights,
			NurseVisibleDuration,
			false);
	}
}

void ARemainNurseCorridorEncounterActor::HideNurseAndLights()
{
	SetActorLightsEnabled(StrongLightActors, false);

	if (NurseEncounter)
	{
		NurseEncounter->EndEncounter();
	}
	else
	{
		SetActorVisible(NurseActorFallback, false);
	}

	if (bRestoreFlickerLightsAfterEvent)
	{
		SetActorLightsEnabled(FlickerLightActors, true);
	}
}

void ARemainNurseCorridorEncounterActor::SetActorLightsEnabled(const TArray<TObjectPtr<AActor>>& Actors, bool bEnabled) const
{
	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		TInlineComponentArray<ULightComponent*> LightComponents;
		Actor->GetComponents(LightComponents);
		for (ULightComponent* LightComponent : LightComponents)
		{
			if (LightComponent)
			{
				LightComponent->SetVisibility(bEnabled, true);
			}
		}

		Actor->SetActorHiddenInGame(!bEnabled);
	}
}

void ARemainNurseCorridorEncounterActor::SetActorVisible(AActor* Actor, bool bVisible) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorHiddenInGame(!bVisible);
	Actor->SetActorEnableCollision(bVisible);
}

bool ARemainNurseCorridorEncounterActor::IsPlayerActor(const AActor* Actor) const
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return IsValid(Actor) && Actor == PlayerPawn;
}
