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

	WBP_Inventory->AddTestItem(0);
	UE_LOG(LogTemp, Warning, TEXT("Added test item to inventory"));
}