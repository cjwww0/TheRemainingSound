#include "UI/RemainDocumentScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

URemainDocumentScreenWidget::URemainDocumentScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URemainDocumentScreenWidget::NativeOnInitialized()
{
	EnsurePageIndicatorTextFallback();
	Super::NativeOnInitialized();
	EnsurePageIndicatorTextFallback();
}

void URemainDocumentScreenWidget::NativeConstruct()
{
	EnsurePageIndicatorTextFallback();
	Super::NativeConstruct();
	EnsurePageIndicatorTextFallback();

	BaseRenderTransform = GetRenderTransform();
	if (TranscriptionText)
	{
		BaseTranscriptionTransform = TranscriptionText->GetRenderTransform();
	}

	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (DizzyBlurOverlay)
	{
		DizzyBlurOverlay->SetVisibility(ESlateVisibility::Hidden);
		DizzyBlurOverlay->SetBlurStrength(0.0f);
	}
}

void URemainDocumentScreenWidget::NativeDestruct()
{
	StopDocumentDizzy();
	Super::NativeDestruct();
}

void URemainDocumentScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bDocumentDizzyActive)
	{
		if (DizzyVisualStrength <= KINDA_SMALL_NUMBER)
		{
			return;
		}
	}

	DizzyElapsed += InDeltaTime;
	DizzyVisualStrength = FMath::FInterpTo(DizzyVisualStrength, DizzyVisualTarget, InDeltaTime, DizzyBlurInterpSpeed);

	FWidgetTransform DizzyTransform = BaseRenderTransform;
	DizzyTransform.Angle += FMath::Sin(DizzyElapsed * DizzyRotationSpeed) * DizzyRotationAmplitude * DizzyVisualStrength;
	DizzyTransform.Translation.X += FMath::Sin(DizzyElapsed * DizzyTranslationSpeedX) * DizzyTranslationAmplitudeX * DizzyVisualStrength;
	DizzyTransform.Translation.Y += FMath::Cos(DizzyElapsed * DizzyTranslationSpeedY) * DizzyTranslationAmplitudeY * DizzyVisualStrength;

	const float ScalePulse = 1.0f + (FMath::Sin(DizzyElapsed * DizzyScaleSpeed) * DizzyScaleAmplitude * DizzyVisualStrength);
	DizzyTransform.Scale *= FVector2D(ScalePulse, ScalePulse);
	SetRenderTransform(DizzyTransform);

	if (TranscriptionText)
	{
		FWidgetTransform TextTransform = BaseTranscriptionTransform;
		TextTransform.Translation.X += FMath::Cos(DizzyElapsed * (DizzyTranslationSpeedX * 1.6f)) * DizzyTextTranslationAmplitudeX * DizzyVisualStrength;
		TextTransform.Translation.Y += FMath::Sin(DizzyElapsed * (DizzyTranslationSpeedY * 1.4f)) * DizzyTextTranslationAmplitudeY * DizzyVisualStrength;
		TranscriptionText->SetRenderTransform(TextTransform);
	}

	if (DizzyBlurOverlay)
	{
		if (DizzyVisualStrength > KINDA_SMALL_NUMBER)
		{
			DizzyBlurOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
			DizzyBlurOverlay->SetBlurStrength(DizzyBlurMaxStrength * DizzyVisualStrength);
		}
		else
		{
			DizzyBlurOverlay->SetBlurStrength(0.0f);
			DizzyBlurOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (!bDocumentDizzyActive && DizzyVisualStrength <= KINDA_SMALL_NUMBER)
	{
		RestoreDizzyVisuals();
	}
}

void URemainDocumentScreenWidget::UpdatePage(int32 CurrentPageIndex, int32 TotalPages)
{
	CachedCurrentPageIndex = CurrentPageIndex;
	CachedTotalPages = FMath::Max(TotalPages, 0);

	UpdateArrowVisibility(CachedCurrentPageIndex, CachedTotalPages);
	UpdatePageIndicatorText(CachedCurrentPageIndex, CachedTotalPages);

	const int32 DisplayPageNumber = CachedTotalPages > 0 ? FMath::Clamp(CachedCurrentPageIndex + 1, 1, CachedTotalPages) : 0;
	if (PageIndicatorText)
	{
		OnPageStateUpdated(CachedCurrentPageIndex, DisplayPageNumber, CachedTotalPages);
	}
}

int32 URemainDocumentScreenWidget::GetCurrentPageIndex() const
{
	return CachedCurrentPageIndex;
}

int32 URemainDocumentScreenWidget::GetTotalPages() const
{
	return CachedTotalPages;
}

void URemainDocumentScreenWidget::StartDocumentDizzy()
{
	if (!bDocumentDizzyActive)
	{
		BaseRenderTransform = GetRenderTransform();
		if (TranscriptionText)
		{
			BaseTranscriptionTransform = TranscriptionText->GetRenderTransform();
		}
	}

	bDocumentDizzyActive = true;
	DizzyElapsed = 0.0f;
	DizzyVisualTarget = 1.0f;
}

void URemainDocumentScreenWidget::StopDocumentDizzy()
{
	bDocumentDizzyActive = false;
	DizzyVisualTarget = 0.0f;
}

void URemainDocumentScreenWidget::EnsurePageIndicatorTextFallback()
{
	if (PageIndicatorText)
	{
		return;
	}

	const FName FallbackName = WidgetTree
		? MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), TEXT("RemainPageIndicatorTextFallback"))
		: MakeUniqueObjectName(this, UTextBlock::StaticClass(), TEXT("RemainPageIndicatorTextFallback"));

	UTextBlock* FallbackText = WidgetTree
		? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FallbackName)
		: NewObject<UTextBlock>(this, FallbackName);

	if (!FallbackText)
	{
		return;
	}

	FallbackText->SetText(FText::GetEmpty());
	FallbackText->SetVisibility(ESlateVisibility::Collapsed);
	PageIndicatorText = FallbackText;
}

void URemainDocumentScreenWidget::UpdateArrowVisibility(int32 CurrentPageIndex, int32 TotalPages)
{
	if (PreviousPageArrow)
	{
		const bool bCanGoPrevious = TotalPages > 0 && CurrentPageIndex > 0;
		PreviousPageArrow->SetVisibility(bCanGoPrevious ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (NextPageArrow)
	{
		const bool bCanGoNext = TotalPages > 0 && CurrentPageIndex >= 0 && CurrentPageIndex < (TotalPages - 1);
		NextPageArrow->SetVisibility(bCanGoNext ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void URemainDocumentScreenWidget::UpdatePageIndicatorText(int32 CurrentPageIndex, int32 TotalPages)
{
	if (!PageIndicatorText)
	{
		return;
	}

	if (TotalPages <= 0)
	{
		PageIndicatorText->SetText(FText::GetEmpty());
		return;
	}

	const int32 DisplayPageNumber = FMath::Clamp(CurrentPageIndex + 1, 1, TotalPages);
	PageIndicatorText->SetText(FText::Format(
		NSLOCTEXT("RemainDocumentScreen", "PageIndicator", "Page {0}/{1}"),
		FText::AsNumber(DisplayPageNumber),
		FText::AsNumber(TotalPages)));
}

void URemainDocumentScreenWidget::RestoreDizzyVisuals()
{
	SetRenderTransform(BaseRenderTransform);

	if (TranscriptionText)
	{
		TranscriptionText->SetRenderTransform(BaseTranscriptionTransform);
	}

	if (DizzyBlurOverlay)
	{
		DizzyBlurOverlay->SetBlurStrength(0.0f);
		DizzyBlurOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}
