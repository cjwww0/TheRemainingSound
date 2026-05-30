#include "Puzzle/RemainPuzzleInventoryLibrary.h"

#include "Debug/RemainInventoryDebugLibrary.h"
#include "UObject/UnrealType.h"

namespace
{
	static TArray<UObject*> GetInventoryItems(UObject* Inventory)
	{
		TArray<UObject*> Result;
		if (!IsValid(Inventory))
		{
			return Result;
		}

		FArrayProperty* InventoryItemsProperty = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("InventoryItems"));
		if (!InventoryItemsProperty || !InventoryItemsProperty->Inner)
		{
			return Result;
		}

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(InventoryItemsProperty->Inner);
		if (!ObjectProperty)
		{
			return Result;
		}

		FScriptArrayHelper Helper(InventoryItemsProperty, InventoryItemsProperty->ContainerPtrToValuePtr<void>(Inventory));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			if (UObject* Item = ObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index)))
			{
				Result.Add(Item);
			}
		}

		return Result;
	}

	static bool PuzzleAcceptsItem(UObject* PuzzleActor, UObject* Item)
	{
		if (!IsValid(PuzzleActor) || !IsValid(Item))
		{
			return false;
		}

		UFunction* CanUseItemFunction = PuzzleActor->FindFunction(TEXT("CanUseItem"));
		if (!CanUseItemFunction)
		{
			return false;
		}

		struct FCanUseItemParams
		{
			UObject* PlayerController = nullptr;
			UObject* Item = nullptr;
			bool Yes = false;
		};

		FCanUseItemParams Params;
		Params.Item = Item;
		PuzzleActor->ProcessEvent(CanUseItemFunction, &Params);
		return Params.Yes;
	}
}

TArray<UObject*> URemainPuzzleInventoryLibrary::GetUsableInventoryItemsForPuzzle(UObject* WorldContextObject, UObject* PuzzleActor)
{
	bool bValidInventory = false;
	UObject* Inventory = URemainInventoryDebugLibrary::GetRuntimeInventory(WorldContextObject, bValidInventory);
	if (!bValidInventory)
	{
		return {};
	}

	const TArray<UObject*> InventoryItems = GetInventoryItems(Inventory);
	TArray<UObject*> UsableItems;
	for (UObject* Item : InventoryItems)
	{
		if (PuzzleAcceptsItem(PuzzleActor, Item))
		{
			UsableItems.Add(Item);
		}
	}

	return UsableItems;
}

bool URemainPuzzleInventoryLibrary::HasUsableInventoryItemsForPuzzle(UObject* WorldContextObject, UObject* PuzzleActor)
{
	return GetUsableInventoryItemsForPuzzle(WorldContextObject, PuzzleActor).Num() > 0;
}
