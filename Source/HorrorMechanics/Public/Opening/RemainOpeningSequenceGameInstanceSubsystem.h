#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RemainOpeningSequenceGameInstanceSubsystem.generated.h"

UCLASS()
class HORRORMECHANICS_API URemainOpeningSequenceGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Opening")
	bool HasPlayedOpeningIntroThisSession() const;

	UFUNCTION(BlueprintCallable, Category="Remain|Opening")
	void MarkOpeningIntroPlayed();

	UFUNCTION(BlueprintCallable, Category="Remain|Opening")
	void ResetOpeningIntroForNewGame();

private:
	UPROPERTY(Transient)
	bool bPlayedOpeningIntroThisSession = false;
};
