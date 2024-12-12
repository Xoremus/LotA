#include "BagWidget.h"
#include "Components/UniformGridPanel.h"
#include "InventorySlotWidget.h"

UBagWidget::UBagWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InventoryGrid)
	{
		InitializeBag(4, 4);
	}
}

void UBagWidget::InitializeBag(int32 Rows, int32 Columns)
{
	if (InventoryGrid)
	{
		InventoryGrid->ClearChildren();
		// Add slots to the grid
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			for (int32 Column = 0; Column < Columns; ++Column)
			{
				// Create and add slots
				// Implementation similar to InventoryWidget
			}
		}
	}
}