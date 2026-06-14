#include "Opening/RemainOpeningIntroWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

URemainOpeningIntroWidget::URemainOpeningIntroWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
	IntroText = FText::FromString(TEXT("2018 年 3 月 15 日。\n协会来信了。\n\n有些声音，不能被留下。"));
	SkipText = FText::FromString(TEXT("按任意键跳过"));
}

TSharedRef<SWidget> URemainOpeningIntroWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
		.BorderBackgroundColor(FLinearColor::Black)
		.Padding(FMargin(96.0f))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(IntroText)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.74f, 0.66f, 1.0f)))
				.Justification(ETextJustify::Center)
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 34))
				.LineHeightPercentage(1.35f)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(SkipText)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.39f, 0.34f, 1.0f)))
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18))
			]
		];
}

FReply URemainOpeningIntroWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	RequestSkip();
	return FReply::Handled();
}

FReply URemainOpeningIntroWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	RequestSkip();
	return FReply::Handled();
}

void URemainOpeningIntroWidget::RequestSkip()
{
	OnSkipRequested.Broadcast();
}
