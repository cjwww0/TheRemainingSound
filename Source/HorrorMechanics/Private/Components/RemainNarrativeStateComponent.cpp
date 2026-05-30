#include "Components/RemainNarrativeStateComponent.h"

URemainNarrativeStateComponent::URemainNarrativeStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
}

void URemainNarrativeStateComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void URemainNarrativeStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DocumentState != ERemainDocumentState::Open || CurrentPageIndex == INDEX_NONE)
	{
		return;
	}

	if (!bDizzyTriggeredForCurrentPage)
	{
		CurrentPageOpenElapsed += DeltaTime;
		if (CurrentPageOpenElapsed >= DizzyTriggerDelay)
		{
			bDizzyTriggeredForCurrentPage = true;
			OnDocumentDizzyTriggered.Broadcast();
		}
	}
}

bool URemainNarrativeStateComponent::OpenDocument(int32 InTotalPages)
{
	const bool bWasAlreadyOpen = (DocumentState == ERemainDocumentState::Open);

	TotalPages = FMath::Max(1, InTotalPages);
	DocumentState = ERemainDocumentState::Open;
	CurrentPageIndex = 0;
	ResetCurrentPageTimer();
	SetComponentTickEnabled(true);

	if (!bWasAlreadyOpen)
	{
		OnDocumentOpened.Broadcast();
	}

	OnDocumentPageChanged.Broadcast(CurrentPageIndex, TotalPages);
	return true;
}

bool URemainNarrativeStateComponent::AdvancePage()
{
	if (DocumentState != ERemainDocumentState::Open)
	{
		return false;
	}

	if (CurrentPageIndex + 1 >= TotalPages)
	{
		CloseDocument();
		return true;
	}

	++CurrentPageIndex;
	ResetCurrentPageTimer();
	OnDocumentPageChanged.Broadcast(CurrentPageIndex, TotalPages);
	return true;
}

bool URemainNarrativeStateComponent::CloseDocument()
{
	if (DocumentState == ERemainDocumentState::Closed)
	{
		return false;
	}

	DocumentState = ERemainDocumentState::Closed;
	CurrentPageIndex = INDEX_NONE;
	ResetCurrentPageTimer();
	SetComponentTickEnabled(false);
	OnDocumentClosed.Broadcast();
	return true;
}

bool URemainNarrativeStateComponent::IsDocumentOpen() const
{
	return DocumentState == ERemainDocumentState::Open;
}

void URemainNarrativeStateComponent::ResetCurrentPageTimer()
{
	CurrentPageOpenElapsed = 0.0f;
	bDizzyTriggeredForCurrentPage = false;
}
