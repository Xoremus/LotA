// InventoryWidget.cpp
#include "InventoryWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InventorySlotWidget.h"
#include "S_ItemInfo.h"

UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
   , NumRows(0)
   , NumColumns(0)
{
}

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Add debug prints
    UE_LOG(LogTemp, Warning, TEXT("=== InventoryWidget NativeConstruct start ==="));
    UE_LOG(LogTemp, Warning, TEXT("InventoryGrid valid: %s"), InventoryGrid ? TEXT("Yes") : TEXT("No"));
    InitializeInventory(5, 2);  // 5 rows, 2 columns
}

void UInventoryWidget::InitializeInventory(int32 Rows, int32 Columns)
{
    UE_LOG(LogTemp, Warning, TEXT("=== InitializeInventory start ==="));
    UE_LOG(LogTemp, Warning, TEXT("Initializing with Rows: %d, Columns: %d"), Rows, Columns);
    
    NumRows = Rows;
    NumColumns = Columns;

    if (!InventoryGrid)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryGrid is null!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Clearing children and creating slots"));
    InventoryGrid->ClearChildren();
    InventorySlots.Empty();

    CreateInventorySlots();
}

void UInventoryWidget::CreateInventorySlots()
{
    UE_LOG(LogTemp, Warning, TEXT("=== CreateInventorySlots start ==="));
    if (!InventoryGrid)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryGrid is null in CreateInventorySlots"));
        return;
    }

    // Load the actual blueprint widget class
    const FSoftClassPath WidgetClassPath(TEXT("/Game/Inventory/Widgets/WBP_InventorySlot.WBP_InventorySlot_C"));
    UClass* SlotWidgetClass = WidgetClassPath.TryLoadClass<UInventorySlotWidget>();
    
    if (!SlotWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_InventorySlot class"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Creating %d x %d grid"), NumRows, NumColumns);

    for (int32 Row = 0; Row < NumRows; ++Row)
    {
        for (int32 Col = 0; Col < NumColumns; ++Col)
        {
            UInventorySlotWidget* NewSlot = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
            if (NewSlot)
            {
                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(InventoryGrid->AddChild(NewSlot));
                if (GridSlot)
                {
                    GridSlot->SetRow(Row);
                    GridSlot->SetColumn(Col);
                    UE_LOG(LogTemp, Warning, TEXT("Created slot at row %d, column %d"), Row, Col);
                }
                InventorySlots.Add(NewSlot);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create slot widget"));
            }
        }
    }
}

void UInventoryWidget::AddTestItem(int32 SlotIndex, const FS_ItemInfo& Item, int32 Quantity)
{
    if (!InventorySlots.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid slot index: %d"), SlotIndex);
        return;
    }

    if (UInventorySlotWidget* InventorySlot = InventorySlots[SlotIndex])
    {
        UE_LOG(LogTemp, Warning, TEXT("Setting slot %d with item: %s"), SlotIndex, *Item.ItemName.ToString());
        InventorySlot->SetItemDetails(Item, Quantity);
    }
}