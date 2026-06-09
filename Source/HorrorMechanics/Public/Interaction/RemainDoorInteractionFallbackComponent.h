#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemainDoorInteractionFallbackComponent.generated.h"

class APlayerController;
class UPrimitiveComponent;
class USceneComponent;

UCLASS(ClassGroup=(Remain), meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainDoorInteractionFallbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainDoorInteractionFallbackComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door")
	bool bEnableFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door")
	bool bForceUnlocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door")
	FName InteractActionName = TEXT("Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door", meta=(ClampMin="50.0"))
	float TraceDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door", meta=(ClampMin="0.0"))
	float FallbackDelay = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door")
	bool bDebugDoorFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door|Visual")
	bool bDriveDoorVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door|Visual")
	FName DoorVisualComponentName = TEXT("Door");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door|Visual")
	FRotator OpenRelativeRotationOffset = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Door|Visual")
	bool bDisableDoorVisualCollisionWhenOpen = true;

	UFUNCTION(BlueprintCallable, Category="Remain|Door")
	void RebindInput();

protected:
	virtual void BeginPlay() override;

private:
	bool bPendingFallback = false;
	bool bStateBeforeInteraction = true;
	TWeakObjectPtr<APlayerController> PendingPlayerController;
	FTimerHandle FallbackTimerHandle;
	FRotator CachedClosedRelativeRotation = FRotator::ZeroRotator;
	bool bCachedClosedRelativeRotation = false;
	ECollisionEnabled::Type CachedDoorVisualCollision = ECollisionEnabled::QueryAndPhysics;
	bool bCachedDoorVisualCollision = false;

	void HandleInteractPressed();
	void EvaluateFallback();
	bool IsPlayerLookingAtOwner(APlayerController* PlayerController) const;
	bool GetDoorClosedState(bool& bOutIsClosed) const;
	void SetDoorBoolProperty(FName PropertyName, bool bValue) const;
	void InvokeDoorFunction(FName FunctionName, APlayerController* PlayerController) const;
	void ApplyUnlockedState() const;
	void CacheVisualClosedRotation();
	void CacheDoorVisualCollision(UPrimitiveComponent* DoorPrimitive);
	void ApplyDoorVisualState(bool bIsClosed);
	void ApplyDoorVisualCollision(bool bIsClosed, UPrimitiveComponent* DoorPrimitive);
	USceneComponent* ResolveDoorVisualComponent() const;
	void DebugMessage(const FString& Message) const;
};
