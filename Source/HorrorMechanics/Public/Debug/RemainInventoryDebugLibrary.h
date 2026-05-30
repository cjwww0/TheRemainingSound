#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainInventoryDebugLibrary.generated.h"

UCLASS()
class HORRORMECHANICS_API URemainInventoryDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Debug", meta=(WorldContext="WorldContextObject"))
	static UObject* GetRuntimeInventory(UObject* WorldContextObject, bool& bValid);

	UFUNCTION(BlueprintCallable, Category="Remain|Debug", meta=(WorldContext="WorldContextObject"))
	static bool InjectInventoryItem(UObject* WorldContextObject, TSubclassOf<UObject> ItemClass, int32 OptionalSlotIndex, UObject*& CreatedItem);

	UFUNCTION(BlueprintCallable, Category="Remain|Debug", meta=(WorldContext="WorldContextObject"))
	static int32 InjectInventoryItems(UObject* WorldContextObject, const TArray<TSubclassOf<UObject>>& ItemClasses);

	UFUNCTION(BlueprintPure, Category="Remain|Debug", meta=(WorldContext="WorldContextObject"))
	static int32 GetRuntimeInventoryItemCount(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Remain|Debug", meta=(WorldContext="WorldContextObject"))
	static FString GetInventoryDebugSummary(UObject* WorldContextObject, TSubclassOf<UObject> ItemClass);

private:
	static UObject* ResolveInventoryFromGameState(UObject* WorldContextObject);
};
