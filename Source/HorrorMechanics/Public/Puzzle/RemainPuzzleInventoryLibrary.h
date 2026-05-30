#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainPuzzleInventoryLibrary.generated.h"

UCLASS()
class HORRORMECHANICS_API URemainPuzzleInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Puzzle", meta=(WorldContext="WorldContextObject"))
	static TArray<UObject*> GetUsableInventoryItemsForPuzzle(UObject* WorldContextObject, UObject* PuzzleActor);

	UFUNCTION(BlueprintPure, Category="Remain|Puzzle", meta=(WorldContext="WorldContextObject"))
	static bool HasUsableInventoryItemsForPuzzle(UObject* WorldContextObject, UObject* PuzzleActor);
};
