#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainPickupEventLibrary.generated.h"

UCLASS()
class HORRORMECHANICS_API URemainPickupEventLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Events", meta=(WorldContext="WorldContextObject"))
	static int32 NotifyWorkbenchPickupCollected(UObject* WorldContextObject, AActor* PickupActor);

	UFUNCTION(BlueprintCallable, Category="Remain|Events", meta=(WorldContext="WorldContextObject"))
	static int32 NotifyInventoryItemAddedForRemainEvents(UObject* WorldContextObject, UObject* InventoryItem);
};
