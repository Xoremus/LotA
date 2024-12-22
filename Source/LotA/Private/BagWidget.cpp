#include "BagWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "InventorySlotWidget.h"
#include "BagComponent.h"

UBagWidget::UBagWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UBagWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UBagWidget::OnCloseButtonClicked);
    }
}

void UBagWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Warning, TEXT("[BagWidget] NativeDestruct called"));
    
    if (OwningBagComponent)
    {
        // Save the state of all slots
        for (int32 i = 0; i < BagSlots.Num(); ++i)
        {
            if (BagSlots[i])
            {
                const FS_ItemInfo& ItemInfo = BagSlots[i]->GetItemInfo();
                int32 Quantity = BagSlots[i]->GetQuantity();
                
                if (Quantity > 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[BagWidget] Saving bag slot %d: %s (Quantity: %d)"), 
                        i, *ItemInfo.ItemName.ToString(), Quantity);
                    OwningBagComponent->AddItem(i, ItemInfo, Quantity);
                }
                else
                {
                    OwningBagComponent->RemoveItem(i);
                }
            }
        }

        // Force close the bag component
        OwningBagComponent->ForceClose();
        UE_LOG(LogTemp, Warning, TEXT("[BagWidget] Forced bag component close"));
    }

    Super::NativeDestruct();
}

void UBagWidget::OnCloseButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("[BagWidget] Close button clicked"));
    
    if (OwningBagComponent)
    {
        // Force close the bag component
        OwningBagComponent->ForceClose();
        UE_LOG(LogTemp, Warning, TEXT("[BagWidget] Forced bag component close from button click"));
    }
    
    RemoveFromParent();
}

void UBagWidget::SetOwningBagComponent(UBagComponent* BagComp)
{
    OwningBagComponent = BagComp;
    UE_LOG(LogTemp, Warning, TEXT("[BagWidget] Set owning bag component: %s"), 
        OwningBagComponent ? *OwningBagComponent->GetName() : TEXT("null"));
}

void UBagWidget::InitializeBag(const FS_ItemInfo& BagInfo)
{
    if (BagInfo.ItemType != EItemType::Bag)
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to initialize bag with non-bag item"));
        return;
    }

    BagItemInfo = BagInfo;

    if (WindowTitle)
    {
        WindowTitle->SetText(BagInfo.ItemName);
    }

    if (InventoryGrid)
    {
        InventoryGrid->ClearChildren();
        BagSlots.Empty();
        CreateBagSlots();

        // Restore bag contents from component
        if (OwningBagComponent)
        {
            for (int32 i = 0; i < BagSlots.Num(); ++i)
            {
                FS_ItemInfo ItemInfo;
                int32 Quantity;
                if (OwningBagComponent->GetSlotContent(i, ItemInfo, Quantity))
                {
                    if (Quantity > 0 && BagSlots[i])
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Restoring bag slot %d: %s (Quantity: %d)"), 
                            i, *ItemInfo.ItemName.ToString(), Quantity);
                        BagSlots[i]->SetItemDetails(ItemInfo, Quantity);
                    }
                }
            }
        }
    }
}

void UBagWidget::CreateBagSlots()
{
    if (!InventoryGrid || BagItemInfo.BagSlots <= 0)
        return;

    int32 TotalSlots = BagItemInfo.BagSlots;
    int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalSlots)));
    int32 Rows = FMath::CeilToInt(static_cast<float>(TotalSlots) / Columns);

    UE_LOG(LogTemp, Warning, TEXT("Creating %d bag slots (%d rows x %d columns)"), 
        TotalSlots, Rows, Columns);

    const FSoftClassPath WidgetClassPath(TEXT("/Game/Inventory/Widgets/WBP_InventorySlot.WBP_InventorySlot_C"));
    UClass* SlotWidgetClass = WidgetClassPath.TryLoadClass<UInventorySlotWidget>();
   
    if (!SlotWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_InventorySlot class"));
        return;
    }

    // Create slots
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Col = 0; Col < Columns && BagSlots.Num() < TotalSlots; ++Col)
        {
            UInventorySlotWidget* NewSlot = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
            if (NewSlot)
            {
                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(InventoryGrid->AddChild(NewSlot));
                if (GridSlot)
                {
                    GridSlot->SetRow(Row);
                    GridSlot->SetColumn(Col);
                    UE_LOG(LogTemp, Warning, TEXT("Created bag slot at row %d, column %d"), Row, Col);
                }
                BagSlots.Add(NewSlot);
            }
        }
    }
}