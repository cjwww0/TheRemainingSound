#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemainInteractionTraceProxyComponent.generated.h"

class UBoxComponent;

UCLASS(ClassGroup=(Remain), meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainInteractionTraceProxyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainInteractionTraceProxyComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction")
	bool bEnableProxy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction")
	FVector RelativeLocation = FVector(0.0f, 0.0f, 115.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction")
	FVector BoxExtent = FVector(120.0f, 60.0f, 130.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Interaction")
	bool bHiddenInGame = true;

	UFUNCTION(BlueprintCallable, Category="Remain|Interaction")
	void RebuildProxy();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> ProxyBox = nullptr;

	void EnsureProxyBox();
	void ConfigureProxyBox();
};
