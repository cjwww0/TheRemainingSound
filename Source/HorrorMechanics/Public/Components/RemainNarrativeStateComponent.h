#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RemainNarrativeStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRemainDocumentPageChangedSignature, int32, CurrentPageIndex, int32, TotalPages);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemainDocumentSimpleSignature);

UENUM(BlueprintType)
enum class ERemainDocumentState : uint8
{
	Closed,
	Open
};

UCLASS(ClassGroup=(Remain), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class HORRORMECHANICS_API URemainNarrativeStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URemainNarrativeStateComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document", meta=(ClampMin="1"))
	int32 TotalPages = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document", meta=(ClampMin="0.1"))
	float DizzyTriggerDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document", meta=(ClampMin="0.1"))
	float DizzyDuration = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Remain|Document")
	ERemainDocumentState DocumentState = ERemainDocumentState::Closed;

	UPROPERTY(BlueprintReadOnly, Category="Remain|Document")
	int32 CurrentPageIndex = INDEX_NONE;

	UPROPERTY(BlueprintAssignable, Category="Remain|Document")
	FRemainDocumentSimpleSignature OnDocumentOpened;

	UPROPERTY(BlueprintAssignable, Category="Remain|Document")
	FRemainDocumentSimpleSignature OnDocumentClosed;

	UPROPERTY(BlueprintAssignable, Category="Remain|Document")
	FRemainDocumentPageChangedSignature OnDocumentPageChanged;

	UPROPERTY(BlueprintAssignable, Category="Remain|Document")
	FRemainDocumentSimpleSignature OnDocumentDizzyTriggered;

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	bool OpenDocument(int32 InTotalPages = 3);

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	bool AdvancePage();

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	bool CloseDocument();

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	bool IsDocumentOpen() const;

protected:
	UPROPERTY(Transient)
	float CurrentPageOpenElapsed = 0.0f;

	UPROPERTY(Transient)
	bool bDizzyTriggeredForCurrentPage = false;

	void ResetCurrentPageTimer();
};
