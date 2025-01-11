#include "InventorySlotDataComponent.h"

UInventorySlotDataComponent::UInventorySlotDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	StackCount = 0;
}

void UInventorySlotDataComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UInventorySlotDataComponent::IsEmpty() const
{
	return StackCount == 0;
}

bool UInventorySlotDataComponent::AddItems(const FS_ItemInfo& NewItem, int32 Count)
{
	if (IsEmpty() || (ItemData.ItemID == NewItem.ItemID && StackCount + Count <= ItemData.MaxStackSize))
	{
		ItemData = NewItem;
		StackCount += Count;
		return true;
	}
	return false;
}

bool UInventorySlotDataComponent::RemoveItems(int32 Count)
{
	if (Count <= StackCount)
	{
		StackCount -= Count;
		if (StackCount == 0)
		{
			ItemData = FS_ItemInfo(); // Reset the slot
		}
		return true;
	}
	return false;
}
