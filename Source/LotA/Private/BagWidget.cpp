// BagWidget.cpp
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
    if (OwningBagComponent)
    {
        OwningBagComponent->CloseBag();
    }

    Super::NativeDestruct();
}

void UBagWidget::OnCloseButtonClicked()
{
    if (OwningBagComponent)
    {
        OwningBagComponent->CloseBag();
    }
    RemoveFromParent();
}

void UBagWidget::SetOwningBagComponent(UBagComponent* BagComp)
{
    OwningBagComponent = BagComp;
}

void UBagWidget::InitializeBag(const FS_ItemInfo& BagInfo)
{
    if (BagInfo.ItemType != EItemType::Bag)
    {
        UE_LOG(LogTemp, Error, TEXT("Attempted to initialize bag with non-bag item"));
        return;
    }

    BagItemInfo = BagInfo;

    // Calculate grid dimensions based on total slots
    int32 TotalSlots = BagInfo.BagSlots;
    int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalSlots)));
    int32 Rows = FMath::CeilToInt(static_cast<float>(TotalSlots) / Columns);

    if (WindowTitle)
    {
        WindowTitle->SetText(BagInfo.ItemName);
    }

    if (InventoryGrid)
    {
        InventoryGrid->ClearChildren();
        BagSlots.Empty();
        CreateBagSlots();
    }
}


void UBagWidget::CreateBagSlots()
{
    if (!InventoryGrid || BagItemInfo.BagSlots <= 0)
        return;

    int32 TotalSlots = BagItemInfo.BagSlots;
    int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalSlots)));
    int32 Rows = FMath::CeilToInt(static_cast<float>(TotalSlots) / Columns);

    // Load the slot widget class
    const FSoftClassPath WidgetClassPath(TEXT("/Game/Inventory/Widgets/WBP_InventorySlot.WBP_InventorySlot_C"));
    UClass* SlotWidgetClass = WidgetClassPath.TryLoadClass<UInventorySlotWidget>();
   
    if (!SlotWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_InventorySlot class"));
        return;
    }

    // Create slots
    int32 SlotIndex = 0;
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Col = 0; Col < Columns && SlotIndex < TotalSlots; ++Col)
        {
            UInventorySlotWidget* NewSlot = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
            if (NewSlot)
            {
                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(InventoryGrid->AddChild(NewSlot));
                if (GridSlot)
                {
                    GridSlot->SetRow(Row);
                    GridSlot->SetColumn(Col);
                }
                BagSlots.Add(NewSlot);
                SlotIndex++;
            }
        }
    }
}