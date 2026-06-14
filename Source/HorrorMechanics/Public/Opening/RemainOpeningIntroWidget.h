#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RemainOpeningIntroWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemainOpeningIntroWidgetSimpleSignature);

UCLASS()
class HORRORMECHANICS_API URemainOpeningIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URemainOpeningIntroWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening")
	FText IntroText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remain|Opening")
	FText SkipText;

	UPROPERTY(BlueprintAssignable, Category="Remain|Opening")
	FRemainOpeningIntroWidgetSimpleSignature OnSkipRequested;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void RequestSkip();
};
