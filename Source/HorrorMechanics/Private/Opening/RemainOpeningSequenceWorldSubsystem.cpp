#include "Opening/RemainOpeningSequenceWorldSubsystem.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/RemainScreenEffectsBridgeComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Opening/RemainOpeningIntroWidget.h"
#include "Opening/RemainOpeningSequenceGameInstanceSubsystem.h"
#include "UObject/UnrealType.h"

bool URemainOpeningSequenceWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URemainOpeningSequenceWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!bEnableOpeningSequence || InWorld.IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	InWorld.GetTimerManager().SetTimer(
		StartTimerHandle,
		this,
		&URemainOpeningSequenceWorldSubsystem::TryStartOpeningSequence,
		0.1f,
		false);
}

void URemainOpeningSequenceWorldSubsystem::TryStartOpeningSequence()
{
	UWorld* World = GetWorld();
	if (!World || bSequenceActive)
	{
		return;
	}

	if (!IsHorrorGameModeWorld())
	{
		return;
	}

	if (ShouldSkipForCheckpoint())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	URemainOpeningSequenceGameInstanceSubsystem* OpeningState =
		GameInstance ? GameInstance->GetSubsystem<URemainOpeningSequenceGameInstanceSubsystem>() : nullptr;
	if (OpeningState && OpeningState->HasPlayedOpeningIntroThisSession())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!IsValid(PlayerController) || !IsValid(PlayerController->GetPawn()))
	{
		if (++StartRetryCount < 30)
		{
			World->GetTimerManager().SetTimer(
				StartTimerHandle,
				this,
				&URemainOpeningSequenceWorldSubsystem::TryStartOpeningSequence,
				0.1f,
				false);
		}
		return;
	}

	if (OpeningState)
	{
		OpeningState->MarkOpeningIntroPlayed();
	}

	StartOpeningSequence(PlayerController);
}

void URemainOpeningSequenceWorldSubsystem::StartOpeningSequence(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	bSequenceActive = true;
	LockedPlayerController = PlayerController;
	LockPlayerInput(PlayerController);
	SetMainHUDVisible(PlayerController, false);

	ActiveIntroWidget = CreateWidget<URemainOpeningIntroWidget>(PlayerController, URemainOpeningIntroWidget::StaticClass());
	if (ActiveIntroWidget)
	{
		ActiveIntroWidget->IntroText = IntroText;
		ActiveIntroWidget->OnSkipRequested.AddDynamic(this, &URemainOpeningSequenceWorldSubsystem::FinishIntro);
		ActiveIntroWidget->AddToViewport(10000);
		ActiveIntroWidget->SetKeyboardFocus();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveIntroWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			IntroTimerHandle,
			this,
			&URemainOpeningSequenceWorldSubsystem::FinishIntro,
			IntroDuration,
			false);
	}
}

void URemainOpeningSequenceWorldSubsystem::FinishIntro()
{
	if (!bSequenceActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(IntroTimerHandle);
	}

	if (ActiveIntroWidget)
	{
		ActiveIntroWidget->RemoveFromParent();
		ActiveIntroWidget = nullptr;
	}

	APlayerController* PlayerController = LockedPlayerController.Get();
	URemainScreenEffectsBridgeComponent* ScreenEffects = ResolveScreenEffectsComponent(PlayerController);
	const float RestoreDelay = ScreenEffects ? FMath::Max(0.0f, ScreenEffects->OpeningEyeClosedDuration) : 0.45f;

	if (ScreenEffects)
	{
		ScreenEffects->StartOpeningBlinkAndBlur();
	}
	else if (PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.4f, FLinearColor::Black, false, false);
	}

	if (World)
	{
		World->GetTimerManager().SetTimer(
			RestoreInputTimerHandle,
			this,
			&URemainOpeningSequenceWorldSubsystem::RestorePlayerInputForBlur,
			RestoreDelay,
			false);
	}
	else
	{
		RestorePlayerInputForBlur();
	}
}

void URemainOpeningSequenceWorldSubsystem::RestorePlayerInputForBlur()
{
	RestorePlayerInput();
	bSequenceActive = false;
}

void URemainOpeningSequenceWorldSubsystem::LockPlayerInput(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	bInputLocked = true;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	PlayerController->bShowMouseCursor = false;
}

void URemainOpeningSequenceWorldSubsystem::RestorePlayerInput()
{
	APlayerController* PlayerController = LockedPlayerController.Get();
	if (!IsValid(PlayerController) || !bInputLocked)
	{
		return;
	}

	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->ResetIgnoreMoveInput();
	PlayerController->ResetIgnoreLookInput();
	PlayerController->bShowMouseCursor = false;
	SetMainHUDVisible(PlayerController, true);

	bInputLocked = false;
	LockedPlayerController = nullptr;
}

void URemainOpeningSequenceWorldSubsystem::SetMainHUDVisible(APlayerController* PlayerController, bool bVisible) const
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	AHUD* HUD = PlayerController->GetHUD();
	if (!IsValid(HUD))
	{
		return;
	}

	const FName FunctionName = bVisible ? TEXT("ShowMainHUD") : TEXT("HideMainHUD");
	if (UFunction* Function = HUD->FindFunction(FunctionName))
	{
		HUD->ProcessEvent(Function, nullptr);
	}
}

bool URemainOpeningSequenceWorldSubsystem::ShouldSkipForCheckpoint() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (!IsValid(GameInstance))
	{
		return false;
	}

	if (const FBoolProperty* CheckpointProperty = FindFProperty<FBoolProperty>(GameInstance->GetClass(), TEXT("CheckpointIsValid")))
	{
		return CheckpointProperty->GetPropertyValue_InContainer(GameInstance);
	}

	return false;
}

bool URemainOpeningSequenceWorldSubsystem::IsHorrorGameModeWorld() const
{
	const UWorld* World = GetWorld();
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return IsValid(GameMode) && GameMode->GetClass()->GetName().Contains(TEXT("HorrorGameMode"));
}

URemainScreenEffectsBridgeComponent* URemainOpeningSequenceWorldSubsystem::ResolveScreenEffectsComponent(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	APawn* Pawn = PlayerController->GetPawn();
	return IsValid(Pawn) ? Pawn->FindComponentByClass<URemainScreenEffectsBridgeComponent>() : nullptr;
}
