#include "Events/RemainNurseEncounterActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ARemainNurseEncounterActor::ARemainNurseEncounterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NurseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NurseMesh"));
	NurseMesh->SetupAttachment(SceneRoot);
	NurseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NurseStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NurseStaticMesh"));
	NurseStaticMesh->SetupAttachment(SceneRoot);
	NurseStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FaceFogMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceFogMesh"));
	FaceFogMesh->SetupAttachment(SceneRoot);
	FaceFogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProximitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProximitySphere"));
	ProximitySphere->SetupAttachment(SceneRoot);
	ProximitySphere->SetSphereRadius(ProximityRadius);
	ProximitySphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProximitySphere->SetCollisionObjectType(ECC_WorldDynamic);
	ProximitySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProximitySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProximitySphere->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> NurseStaticMeshFinder(TEXT("/Game/NurseNPC/SM_Nurse.SM_Nurse"));
	if (NurseStaticMeshFinder.Succeeded())
	{
		NurseStaticMesh->SetStaticMesh(NurseStaticMeshFinder.Object);
	}
}

void ARemainNurseEncounterActor::BeginPlay()
{
	Super::BeginPlay();

	if (ProximitySphere)
	{
		ProximitySphere->SetSphereRadius(ProximityRadius);
		ProximitySphere->OnComponentBeginOverlap.AddDynamic(this, &ARemainNurseEncounterActor::HandleProximityBeginOverlap);
		ProximitySphere->OnComponentEndOverlap.AddDynamic(this, &ARemainNurseEncounterActor::HandleProximityEndOverlap);
	}

	if (bAutoHideOnBeginPlay)
	{
		SetEncounterVisible(false);
	}
}

bool ARemainNurseEncounterActor::ActivateEncounter()
{
	if (bEncounterActive)
	{
		return false;
	}

	bEncounterActive = true;
	SetEncounterVisible(true);

	if (AppearSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AppearSound, GetActorLocation());
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveTimerHandle);
		World->GetTimerManager().SetTimer(ActiveTimerHandle, this, &ARemainNurseEncounterActor::EndEncounter, ActiveDuration, false);
	}

	OnEncounterActivated.Broadcast();
	return true;
}

void ARemainNurseEncounterActor::EndEncounter()
{
	if (!bEncounterActive)
	{
		return;
	}

	bEncounterActive = false;
	StopProximityFeedback();
	SetEncounterVisible(false);

	if (DisappearSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DisappearSound, GetActorLocation());
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveTimerHandle);
	}

	OnEncounterEnded.Broadcast();
}

bool ARemainNurseEncounterActor::IsEncounterActive() const
{
	return bEncounterActive;
}

void ARemainNurseEncounterActor::HandleProximityBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bEncounterActive && IsPlayerActor(OtherActor))
	{
		StartProximityFeedback();
	}
}

void ARemainNurseEncounterActor::HandleProximityEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (IsPlayerActor(OtherActor))
	{
		StopProximityFeedback();
	}
}

void ARemainNurseEncounterActor::SetEncounterVisible(const bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);

	if (NurseMesh)
	{
		const bool bUseSkeletalMesh = !bPreferStaticNurseMesh || !NurseStaticMesh || !NurseStaticMesh->GetStaticMesh();
		NurseMesh->SetVisibility(bVisible && bUseSkeletalMesh, true);
		NurseMesh->SetHiddenInGame(!bVisible || !bUseSkeletalMesh, true);
	}

	if (NurseStaticMesh)
	{
		const bool bUseStaticMesh = bPreferStaticNurseMesh && NurseStaticMesh->GetStaticMesh();
		NurseStaticMesh->SetVisibility(bVisible && bUseStaticMesh, true);
		NurseStaticMesh->SetHiddenInGame(!bVisible || !bUseStaticMesh, true);
	}

	if (FaceFogMesh)
	{
		FaceFogMesh->SetVisibility(bVisible, true);
		FaceFogMesh->SetHiddenInGame(!bVisible, true);
	}

	if (ProximitySphere)
	{
		ProximitySphere->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		ProximitySphere->SetGenerateOverlapEvents(bVisible);
	}
}

void ARemainNurseEncounterActor::StartProximityFeedback()
{
	if (bProximityFeedbackActive)
	{
		return;
	}

	bProximityFeedbackActive = true;

	if (APlayerController* PlayerController = ResolvePlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				0.0f,
				BlackFadeAlpha,
				BlackFadeInDuration,
				FLinearColor::Black,
				false,
				true);
		}
	}

	if (TinnitusSound)
	{
		TinnitusAudioComponent = UGameplayStatics::SpawnSound2D(this, TinnitusSound, 1.0f, 1.0f, 0.0f, nullptr, bLoopProximitySounds);
	}

	if (BreathingSound)
	{
		BreathingAudioComponent = UGameplayStatics::SpawnSound2D(this, BreathingSound, 1.0f, 1.0f, 0.0f, nullptr, bLoopProximitySounds);
	}

	OnProximityFeedbackStarted.Broadcast();
}

void ARemainNurseEncounterActor::StopProximityFeedback()
{
	if (!bProximityFeedbackActive)
	{
		return;
	}

	bProximityFeedbackActive = false;

	if (APlayerController* PlayerController = ResolvePlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				BlackFadeAlpha,
				0.0f,
				BlackFadeOutDuration,
				FLinearColor::Black,
				false,
				false);
		}
	}

	if (TinnitusAudioComponent)
	{
		TinnitusAudioComponent->Stop();
		TinnitusAudioComponent = nullptr;
	}

	if (BreathingAudioComponent)
	{
		BreathingAudioComponent->Stop();
		BreathingAudioComponent = nullptr;
	}

	OnProximityFeedbackStopped.Broadcast();
}

bool ARemainNurseEncounterActor::IsPlayerActor(const AActor* Actor) const
{
	const APlayerController* PlayerController = ResolvePlayerController();
	return IsValid(Actor) && IsValid(PlayerController) && Actor == PlayerController->GetPawn();
}

APlayerController* ARemainNurseEncounterActor::ResolvePlayerController() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetFirstPlayerController();
	}

	return nullptr;
}
