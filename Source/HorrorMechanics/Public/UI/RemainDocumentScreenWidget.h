#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RemainDocumentScreenWidget.generated.h"

class UTextBlock;
class UWidget;
class UBackgroundBlur;

UCLASS(BlueprintType, Blueprintable)
class HORRORMECHANICS_API URemainDocumentScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URemainDocumentScreenWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	void UpdatePage(int32 CurrentPageIndex, int32 TotalPages);

	UFUNCTION(BlueprintPure, Category="Remain|Document")
	int32 GetCurrentPageIndex() const;

	UFUNCTION(BlueprintPure, Category="Remain|Document")
	int32 GetTotalPages() const;

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	void StartDocumentDizzy();

	UFUNCTION(BlueprintCallable, Category="Remain|Document")
	void StopDocumentDizzy();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Remain|Document")
	TObjectPtr<UTextBlock> PageIndicatorText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Remain|Document")
	TObjectPtr<UTextBlock> TranscriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Remain|Document")
	TObjectPtr<UWidget> PreviousPageArrow = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Remain|Document")
	TObjectPtr<UWidget> NextPageArrow = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Remain|Document|Dizzy")
	TObjectPtr<UBackgroundBlur> DizzyBlurOverlay = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category="Remain|Document")
	void OnPageStateUpdated(int32 CurrentPageIndex, int32 DisplayPageNumber, int32 TotalPages);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyRotationAmplitude = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyRotationSpeed = 1.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTranslationAmplitudeX = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTranslationAmplitudeY = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTranslationSpeedX = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTranslationSpeedY = 2.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="1.0"))
	float DizzyScaleAmplitude = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyScaleSpeed = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTextTranslationAmplitudeX = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyTextTranslationAmplitudeY = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyBlurMaxStrength = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Document|Dizzy", meta=(ClampMin="0.0"))
	float DizzyBlurInterpSpeed = 2.0f;

private:
	void EnsurePageIndicatorTextFallback();
	void UpdateArrowVisibility(int32 CurrentPageIndex, int32 TotalPages);
	void UpdatePageIndicatorText(int32 CurrentPageIndex, int32 TotalPages);
	void RestoreDizzyVisuals();

	UPROPERTY(Transient)
	int32 CachedCurrentPageIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 CachedTotalPages = 0;

	UPROPERTY(Transient)
	bool bDocumentDizzyActive = false;

	UPROPERTY(Transient)
	float DizzyElapsed = 0.0f;

	UPROPERTY(Transient)
	float DizzyVisualStrength = 0.0f;

	UPROPERTY(Transient)
	float DizzyVisualTarget = 0.0f;

	UPROPERTY(Transient)
	FWidgetTransform BaseRenderTransform;

	UPROPERTY(Transient)
	FWidgetTransform BaseTranscriptionTransform;
};
