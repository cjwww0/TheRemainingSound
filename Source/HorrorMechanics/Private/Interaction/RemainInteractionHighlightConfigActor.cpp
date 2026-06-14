#include "Interaction/RemainInteractionHighlightConfigActor.h"

#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARemainInteractionHighlightConfigActor::ARemainInteractionHighlightConfigActor()
{
	PrimaryActorTick.bCanEverTick = false;

	HighlightStencilValue = 1;
	bAutoApplyPostProcessMaterial = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultPostProcessMaterial(
		TEXT("/Game/SoftOutline/Styles/SoftOutline_Simple.SoftOutline_Simple"));
	if (DefaultPostProcessMaterial.Succeeded())
	{
		PostProcessMaterial = DefaultPostProcessMaterial.Object;
	}

	IgnoredComponentTags.Add(TEXT("InteractionBox"));
	IgnoredComponentTags.Add(TEXT("Trigger"));
	IgnoredComponentTags.Add(TEXT("Collision"));
}
