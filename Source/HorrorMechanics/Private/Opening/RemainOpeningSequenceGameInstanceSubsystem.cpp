#include "Opening/RemainOpeningSequenceGameInstanceSubsystem.h"

bool URemainOpeningSequenceGameInstanceSubsystem::HasPlayedOpeningIntroThisSession() const
{
	return bPlayedOpeningIntroThisSession;
}

void URemainOpeningSequenceGameInstanceSubsystem::MarkOpeningIntroPlayed()
{
	bPlayedOpeningIntroThisSession = true;
}

void URemainOpeningSequenceGameInstanceSubsystem::ResetOpeningIntroForNewGame()
{
	bPlayedOpeningIntroThisSession = false;
}
