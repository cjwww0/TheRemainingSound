#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RemainInteractionSetupLibrary.generated.h"

class AActor;
class URemainDoorInteractionFallbackComponent;
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

	UFUNCTION(BlueprintCallable, Category="Remain|Interaction")
	static URemainDoorInteractionFallbackComponent* EnsureDoorInteractionFallback(
		AActor* Actor,
		FName InteractActionName,
		float TraceDistance,
		bool bForceUnlocked,
		bool bDebug);
};
