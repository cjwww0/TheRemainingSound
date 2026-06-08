#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainInteractionSetupLibrary.generated.h"

class AActor;
class URemainInteractionTraceProxyComponent;

UCLASS()
class HORRORMECHANICS_API URemainInteractionSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Remain|Interaction")
	static URemainInteractionTraceProxyComponent* EnsureInteractionTraceProxy(
		AActor* Actor,
		FVector RelativeLocation,
		FVector BoxExtent,
		FRotator RelativeRotation);
};
