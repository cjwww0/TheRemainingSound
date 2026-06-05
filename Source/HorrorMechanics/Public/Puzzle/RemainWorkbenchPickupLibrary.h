#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainWorkbenchPickupLibrary.generated.h"

UCLASS()
class HORRORMECHANICS_API URemainWorkbenchPickupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Workbench|Pickup")
	static void SetWorkbenchPickupEnabled(AActor* PickupActor, bool bEnabled);
};
