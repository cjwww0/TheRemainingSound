#include "Events/RemainPickupEventLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Events/RemainPickupShockController.h"

int32 URemainPickupEventLibrary::NotifyWorkbenchPickupCollected(UObject* WorldContextObject, AActor* PickupActor)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0;
	}

	int32 TriggeredCount = 0;
	for (TActorIterator<ARemainPickupShockController> It(World); It; ++It)
	{
		if (It->NotifyWorkbenchPickupCollected(PickupActor))
		{
			++TriggeredCount;
		}
	}

	return TriggeredCount;
}

int32 URemainPickupEventLibrary::NotifyInventoryItemAddedForRemainEvents(UObject* WorldContextObject, UObject* InventoryItem)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0;
	}

	int32 TriggeredCount = 0;
	for (TActorIterator<ARemainPickupShockController> It(World); It; ++It)
	{
		if (It->NotifyInventoryItemAdded(InventoryItem))
		{
			++TriggeredCount;
		}
	}

	return TriggeredCount;
}
