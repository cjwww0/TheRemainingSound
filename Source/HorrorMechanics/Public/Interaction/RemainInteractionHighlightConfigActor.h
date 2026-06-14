#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RemainInteractionHighlightConfigActor.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API ARemainInteractionHighlightConfigActor : public AActor
{
	GENERATED_BODY()

public:
	ARemainInteractionHighlightConfigActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bEnableFocusedHighlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight", meta=(ClampMin="0", ClampMax="255"))
	int32 HighlightStencilValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TObjectPtr<UMaterialInterface> HighlightOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TObjectPtr<UMaterialInterface> PostProcessMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bAutoApplyPostProcessMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TArray<FName> IgnoredActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	TArray<FName> IgnoredComponentTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight")
	bool bClearHighlightWhenHUDHasActiveScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction Highlight|Debug")
	bool bDebugFocusedHighlight = false;
};
