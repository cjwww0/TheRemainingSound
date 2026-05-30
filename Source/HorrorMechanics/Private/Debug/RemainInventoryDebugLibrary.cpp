#include "Debug/RemainInventoryDebugLibrary.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/ScriptArray.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FAddItemParams
	{
		UClass* ItemClass = nullptr;
		FGuid Optional_GUID;
		int32 OptionalSlotIndex = -1;
		UObject* InventoryItem = nullptr;
	};

	static FArrayProperty* FindInventoryItemsProperty(UObject* Inventory)
	{
		return Inventory ? FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("InventoryItems")) : nullptr;
	}

	static int32 GetInventoryItemsCount(UObject* Inventory)
	{
		FArrayProperty* ArrayProperty = FindInventoryItemsProperty(Inventory);
		if (!ArrayProperty)
		{
			return INDEX_NONE;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Inventory));
		return Helper.Num();
	}

	static UObject* GetInventoryItemAt(UObject* Inventory, int32 Index)
	{
		FArrayProperty* ArrayProperty = FindInventoryItemsProperty(Inventory);
		if (!ArrayProperty || !ArrayProperty->Inner)
		{
			return nullptr;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Inventory));
		if (!Helper.IsValidIndex(Index))
		{
			return nullptr;
		}

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
		if (!ObjectProperty)
		{
			return nullptr;
		}

		return ObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index));
	}

	static AGameStateBase* ResolveGameState(UObject* WorldContextObject)
	{
		if (!IsValid(WorldContextObject))
		{
			return nullptr;
		}

		return UGameplayStatics::GetGameState(WorldContextObject);
	}

	static bool TryManualInventoryInsert(UObject* Inventory, AGameStateBase* GameState, UClass* ItemClass, UObject*& CreatedItem)
	{
		CreatedItem = nullptr;

		FArrayProperty* ArrayProperty = FindInventoryItemsProperty(Inventory);
		if (!ArrayProperty || !ArrayProperty->Inner)
		{
			return false;
		}

		CreatedItem = NewObject<UObject>(Inventory, ItemClass);
		if (!IsValid(CreatedItem))
		{
			return false;
		}

		if (FObjectProperty* GameStateProperty = FindFProperty<FObjectProperty>(CreatedItem->GetClass(), TEXT("GameState")))
		{
			GameStateProperty->SetObjectPropertyValue_InContainer(CreatedItem, GameState);
		}

		if (FStructProperty* GuidProperty = FindFProperty<FStructProperty>(CreatedItem->GetClass(), TEXT("GUID")))
		{
			if (GuidProperty->Struct == TBaseStructure<FGuid>::Get())
			{
				*GuidProperty->ContainerPtrToValuePtr<FGuid>(CreatedItem) = FGuid::NewGuid();
			}
		}

		const int32 SlotIndex = GetInventoryItemsCount(Inventory);
		if (UFunction* SetSlotIndexFunction = CreatedItem->FindFunction(TEXT("SetSlotIndex")))
		{
			struct FSetSlotIndexParams
			{
				int32 Index = 0;
			};

			FSetSlotIndexParams Params;
			Params.Index = SlotIndex;
			CreatedItem->ProcessEvent(SetSlotIndexFunction, &Params);
		}
		else if (FIntProperty* SlotProperty = FindFProperty<FIntProperty>(CreatedItem->GetClass(), TEXT("Slot")))
		{
			SlotProperty->SetPropertyValue_InContainer(CreatedItem, SlotIndex);
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Inventory));
		const int32 NewIndex = Helper.AddValue();
		FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
		if (!ObjectProperty)
		{
			return false;
		}

		ObjectProperty->SetObjectPropertyValue(Helper.GetRawPtr(NewIndex), CreatedItem);
		return true;
	}
}

UObject* URemainInventoryDebugLibrary::GetRuntimeInventory(UObject* WorldContextObject, bool& bValid)
{
	UObject* Inventory = ResolveInventoryFromGameState(WorldContextObject);
	bValid = IsValid(Inventory);
	return Inventory;
}

bool URemainInventoryDebugLibrary::InjectInventoryItem(UObject* WorldContextObject, TSubclassOf<UObject> ItemClass, int32 OptionalSlotIndex, UObject*& CreatedItem)
{
	CreatedItem = nullptr;

	UObject* Inventory = ResolveInventoryFromGameState(WorldContextObject);
	if (!IsValid(Inventory) || !ItemClass)
	{
		return false;
	}

	UFunction* AddItemFunction = Inventory->FindFunction(TEXT("AddItem"));
	if (!AddItemFunction)
	{
		return false;
	}

	const int32 CountBefore = GetInventoryItemsCount(Inventory);

	FAddItemParams Params;
	Params.ItemClass = ItemClass.Get();
	Params.Optional_GUID.Invalidate();
	Params.OptionalSlotIndex = OptionalSlotIndex;
	Inventory->ProcessEvent(AddItemFunction, &Params);

	CreatedItem = Params.InventoryItem;
	if (IsValid(CreatedItem))
	{
		return true;
	}

	const int32 CountAfter = GetInventoryItemsCount(Inventory);
	if (CountBefore != INDEX_NONE && CountAfter > CountBefore)
	{
		CreatedItem = GetInventoryItemAt(Inventory, CountAfter - 1);
		return true;
	}

	AGameStateBase* GameState = ResolveGameState(WorldContextObject);
	if (!TryManualInventoryInsert(Inventory, GameState, ItemClass.Get(), CreatedItem))
	{
		return false;
	}

	return IsValid(CreatedItem);
}

int32 URemainInventoryDebugLibrary::InjectInventoryItems(UObject* WorldContextObject, const TArray<TSubclassOf<UObject>>& ItemClasses)
{
	int32 AddedCount = 0;

	for (const TSubclassOf<UObject>& ItemClass : ItemClasses)
	{
		UObject* CreatedItem = nullptr;
		if (InjectInventoryItem(WorldContextObject, ItemClass, -1, CreatedItem))
		{
			++AddedCount;
		}
	}

	return AddedCount;
}

int32 URemainInventoryDebugLibrary::GetRuntimeInventoryItemCount(UObject* WorldContextObject)
{
	UObject* Inventory = ResolveInventoryFromGameState(WorldContextObject);
	return GetInventoryItemsCount(Inventory);
}

FString URemainInventoryDebugLibrary::GetInventoryDebugSummary(UObject* WorldContextObject, TSubclassOf<UObject> ItemClass)
{
	TArray<FString> Parts;

	if (!IsValid(WorldContextObject))
	{
		Parts.Add(TEXT("WorldContext=Invalid"));
		return FString::Join(Parts, TEXT(" | "));
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	Parts.Add(World ? FString::Printf(TEXT("World=%s"), *World->GetName()) : TEXT("World=None"));

	AGameStateBase* GameState = ResolveGameState(WorldContextObject);
	Parts.Add(GameState ? FString::Printf(TEXT("GameState=%s"), *GameState->GetClass()->GetName()) : TEXT("GameState=None"));

	UObject* Inventory = ResolveInventoryFromGameState(WorldContextObject);
	Parts.Add(Inventory ? FString::Printf(TEXT("Inventory=%s"), *Inventory->GetClass()->GetName()) : TEXT("Inventory=None"));

	const int32 ItemCount = GetInventoryItemsCount(Inventory);
	Parts.Add(ItemCount == INDEX_NONE ? TEXT("InventoryItems=Missing") : FString::Printf(TEXT("InventoryItems=%d"), ItemCount));

	Parts.Add(ItemClass ? FString::Printf(TEXT("ItemClass=%s"), *ItemClass->GetName()) : TEXT("ItemClass=None"));

	if (ItemClass)
	{
		UObject* DefaultObject = ItemClass->GetDefaultObject();
		Parts.Add(DefaultObject ? FString::Printf(TEXT("DefaultObject=%s"), *DefaultObject->GetClass()->GetName()) : TEXT("DefaultObject=None"));
		if (FObjectProperty* GameStateProperty = FindFProperty<FObjectProperty>(ItemClass.Get(), TEXT("GameState")))
		{
			Parts.Add(FString::Printf(TEXT("ClassHasGameStateProp=%s"), *GameStateProperty->GetName()));
		}
		if (FStructProperty* GuidProperty = FindFProperty<FStructProperty>(ItemClass.Get(), TEXT("GUID")))
		{
			Parts.Add(FString::Printf(TEXT("ClassHasGuidProp=%s"), *GuidProperty->GetName()));
		}
	}

	return FString::Join(Parts, TEXT(" | "));
}

UObject* URemainInventoryDebugLibrary::ResolveInventoryFromGameState(UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	AGameStateBase* GameState = UGameplayStatics::GetGameState(WorldContextObject);
	if (!IsValid(GameState))
	{
		return nullptr;
	}

	FObjectProperty* InventoryProperty = FindFProperty<FObjectProperty>(GameState->GetClass(), TEXT("Inventory"));
	if (!InventoryProperty)
	{
		return nullptr;
	}

	return InventoryProperty->GetObjectPropertyValue_InContainer(GameState);
}
