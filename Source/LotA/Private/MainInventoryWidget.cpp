// MainInventoryWidget.cpp
#include "MainInventoryWidget.h"
#include "InventoryWidget.h"
#include "S_ItemInfo.h"

void UMainInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WBP_Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidget successfully bound in MainInventoryWidget."));
		AddTestItems();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryWidget binding failed in MainInventoryWidget!"));
	}
}

void UMainInventoryWidget::AddTestItems()
{
	if (!WBP_Inventory)
	{
		UE_LOG(LogTemp, Error, TEXT("WBP_Inventory is null"));
		return;
	}

	// Create test health potion
	FS_ItemInfo HealthPotion;
	HealthPotion.ItemID = FName("Item_HealthPotion");
	HealthPotion.ItemName = FText::FromString("Health Potion");
	HealthPotion.ItemType = EItemType::Consumable;
	HealthPotion.Weight = 0.5f;
	HealthPotion.MaxStackSize = 20;
	HealthPotion.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_HealthPotion")));

	// Create test bag
	FS_ItemInfo TestBag;
	TestBag.ItemID = FName("Item_SmallBag");
	TestBag.ItemName = FText::FromString("Small Bag");
	TestBag.ItemType = EItemType::Bag;
	TestBag.Weight = 1.0f;
	TestBag.MaxStackSize = 1;
	TestBag.BagSlots = 6;  // 6 slot bag
	TestBag.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_StandardBagIcon"))); // Using same icon for now

	if (WBP_Inventory)
	{
		WBP_Inventory->AddTestItem(0, HealthPotion, 5);  // Health potion in first slot
		WBP_Inventory->AddTestItem(1, TestBag, 1);      // Bag in second slot
		UE_LOG(LogTemp, Warning, TEXT("Added potion and bag to inventory"));
	}
}