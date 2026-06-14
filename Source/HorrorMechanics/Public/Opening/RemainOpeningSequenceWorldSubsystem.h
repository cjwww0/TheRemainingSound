#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "RemainOpeningSequenceWorldSubsystem.generated.h"

class URemainOpeningIntroWidget;
class URemainScreenEffectsBridgeComponent;

UCLASS()
class HORRORMECHANICS_API URemainOpeningSequenceWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening")
	bool bEnableOpeningSequence = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening", meta=(ClampMin="0.0"))
	float IntroDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening")
	FText IntroText = FText::FromString(TEXT("2018 年 3 月 15 日。\n协会来信了。\n\n有些声音，不能被留下。"));

private:
	UPROPERTY(Transient)
	TObjectPtr<URemainOpeningIntroWidget> ActiveIntroWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> LockedPlayerController = nullptr;

	FTimerHandle StartTimerHandle;
	FTimerHandle IntroTimerHandle;
	FTimerHandle RestoreInputTimerHandle;

	int32 StartRetryCount = 0;
	bool bSequenceActive = false;
	bool bInputLocked = false;

	void TryStartOpeningSequence();
	void StartOpeningSequence(APlayerController* PlayerController);

	UFUNCTION()
	void FinishIntro();

	void RestorePlayerInputForBlur();
	void LockPlayerInput(APlayerController* PlayerController);
	void RestorePlayerInput();
	void SetMainHUDVisible(APlayerController* PlayerController, bool bVisible) const;
	bool ShouldSkipForCheckpoint() const;
	bool IsHorrorGameModeWorld() const;
	URemainScreenEffectsBridgeComponent* ResolveScreenEffectsComponent(APlayerController* PlayerController) const;
};
