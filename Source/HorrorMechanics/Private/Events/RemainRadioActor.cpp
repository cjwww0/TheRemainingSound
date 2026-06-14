#include "Events/RemainRadioActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ARemainRadioActor::ARemainRadioActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	InputPriority = 950;

	RadioMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RadioMesh"));
	SetRootComponent(RadioMesh);
	RadioMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RadioMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RadioMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RadioMesh);
	InteractionBox->SetBoxExtent(FVector(80.0f, 45.0f, 45.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionBox->SetGenerateOverlapEvents(false);
	InteractionBox->SetHiddenInGame(true);

	RadioAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RadioAudio"));
	RadioAudio->SetupAttachment(RadioMesh);
	RadioAudio->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RadioMeshFinder(TEXT("/Game/NotKnowWhereToDeposit/Radio/Mesh/RadioVintage.RadioVintage"));
	if (RadioMeshFinder.Succeeded())
	{
		RadioMesh->SetStaticMesh(RadioMeshFinder.Object);
	}
}

void ARemainRadioActor::BeginPlay()
{
	Super::BeginPlay();

	// Keep the radio usable even if the Blueprint Interface graph is temporarily miswired.
	bEnableDirectInteractInputFallback = true;
	bStartAmbientOnBeginPlay = false;
	bLockMovementWhileListening = false;

	if (RadioAudio && BroadcastSound)
	{
		RadioAudio->SetSound(BroadcastSound);
		RadioAudio->Stop();
	}

	SetActorTickEnabled(bEnableDirectInteractInputFallback);
}

void ARemainRadioActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDirectInteractInputFallback();
}

void ARemainRadioActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopListening();
	DisableDirectInteractInput();
	Super::EndPlay(EndPlayReason);
}

void ARemainRadioActor::ToggleRadio()
{
	StartListening(UGameplayStatics::GetPlayerController(this, 0));
}

void ARemainRadioActor::HandleRadioInteraction(APlayerController* PlayerController, UPrimitiveComponent* Component)
{
	StartListening(PlayerController ? PlayerController : UGameplayStatics::GetPlayerController(this, 0));
}

bool ARemainRadioActor::IsRadioInteractionAvailable(UPrimitiveComponent* Component) const
{
	return true;
}

bool ARemainRadioActor::IsRadioPlayingActiveBroadcast() const
{
	return bListeningBroadcast;
}

bool ARemainRadioActor::IsRadioListening() const
{
	return bListeningBroadcast;
}

void ARemainRadioActor::StartAmbientBroadcast()
{
	if (RadioAudio)
	{
		RadioAudio->Stop();
	}
}

void ARemainRadioActor::PlayActiveBroadcast()
{
	StartListening(UGameplayStatics::GetPlayerController(this, 0));
}

void ARemainRadioActor::PauseBroadcast()
{
	StopListening();
}

void ARemainRadioActor::StartListening(APlayerController* PlayerController)
{
	if (!RadioAudio || !BroadcastSound)
	{
		return;
	}

	if (!IsValid(PlayerController))
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	ListeningPlayerController = PlayerController;
	if (IsValid(PlayerController))
	{
		EnableDirectInteractInput(PlayerController);
		RestorePlayerAfterListening();
	}

	bListeningBroadcast = true;
	RadioAudio->Stop();
	RadioAudio->SetSound(BroadcastSound);
	RadioAudio->SetVolumeMultiplier(ActiveVolume);
	RadioAudio->Play(0.0f);
	SetActorTickEnabled(true);
}

void ARemainRadioActor::StopListening()
{
	if (!bListeningBroadcast && !bMoveInputLockedByRadio)
	{
		return;
	}

	bListeningBroadcast = false;
	if (RadioAudio)
	{
		RadioAudio->Stop();
	}

	RestorePlayerAfterListening();
	ListeningPlayerController = nullptr;
}

void ARemainRadioActor::UpdateDirectInteractInputFallback()
{
	if (!bEnableDirectInteractInputFallback || !GetWorld())
	{
		DisableDirectInteractInput();
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PlayerController))
	{
		DisableDirectInteractInput();
		return;
	}

	if (bListeningBroadcast)
	{
		EnableDirectInteractInput(PlayerController);
		return;
	}

	if (IsPlayerLookingAtRadio(PlayerController))
	{
		EnableDirectInteractInput(PlayerController);
	}
	else
	{
		DisableDirectInteractInput();
	}
}

bool ARemainRadioActor::IsPlayerLookingAtRadio(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerCameraManager) || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = PlayerController->PlayerCameraManager->GetCameraLocation();
	const FVector TraceEnd = TraceStart + PlayerController->PlayerCameraManager->GetCameraRotation().Vector() * DirectInteractTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RemainRadioDirectInteractTrace), false);
	if (APawn* Pawn = PlayerController->GetPawn())
	{
		QueryParams.AddIgnoredActor(Pawn);
	}

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const AActor* HitActor = Hit.GetActor();
	return HitActor == this || (Hit.GetComponent() && Hit.GetComponent()->GetOwner() == this);
}

void ARemainRadioActor::EnableDirectInteractInput(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	if (!bDirectInputEnabled || DirectInputPlayerController != PlayerController)
	{
		DirectInputPlayerController = PlayerController;
		EnableInput(PlayerController);
		bDirectInputEnabled = true;
	}

	if (InputComponent && !bDirectInputBound && !DirectInteractActionName.IsNone())
	{
		InputComponent->Priority = InputPriority;
		FInputActionBinding& Binding = InputComponent->BindAction(DirectInteractActionName, IE_Pressed, this, &ARemainRadioActor::HandleDirectInteractInput);
		Binding.bConsumeInput = true;
		bDirectInputBound = true;
	}

	if (InputComponent && !bDirectBackInputBound && !DirectBackActionName.IsNone())
	{
		InputComponent->Priority = InputPriority;
		FInputActionBinding& Binding = InputComponent->BindAction(DirectBackActionName, IE_Pressed, this, &ARemainRadioActor::HandleDirectBackInput);
		Binding.bConsumeInput = true;
		bDirectBackInputBound = true;
	}
}

void ARemainRadioActor::DisableDirectInteractInput()
{
	if (!bDirectInputEnabled)
	{
		return;
	}

	if (IsValid(DirectInputPlayerController))
	{
		DisableInput(DirectInputPlayerController);
	}

	DirectInputPlayerController = nullptr;
	bDirectInputEnabled = false;
}

void ARemainRadioActor::HandleDirectInteractInput()
{
	if (!bListeningBroadcast)
	{
		StartListening(DirectInputPlayerController.Get());
	}
}

void ARemainRadioActor::HandleDirectBackInput()
{
	if (bListeningBroadcast)
	{
		StopListening();
	}
}

void ARemainRadioActor::LockPlayerForListening(APlayerController* PlayerController)
{
	if (!bLockMovementWhileListening || !IsValid(PlayerController) || bMoveInputLockedByRadio)
	{
		return;
	}

	PlayerController->SetIgnoreMoveInput(true);
	bMoveInputLockedByRadio = true;
}

void ARemainRadioActor::RestorePlayerAfterListening()
{
	if (!bMoveInputLockedByRadio)
	{
		return;
	}

	APlayerController* PlayerController = ListeningPlayerController.Get();
	if (!IsValid(PlayerController))
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (IsValid(PlayerController))
	{
		PlayerController->SetIgnoreMoveInput(false);
	}

	bMoveInputLockedByRadio = false;
}
