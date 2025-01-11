// BagWidget.cpp
#include "BagWidget.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "LotA/LotACharacter.h"
#include "InventorySlotWidget.h"

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
        // Save state before destroying
        OwningBagComponent->SaveState();
        OwningBagComponent->CloseBag();  // Don't force close, just regular close
        OwningBagComponent->OnSlotUpdated.RemoveAll(this);
        OwningBagComponent = nullptr;
    }
    Super::NativeDestruct();
}

void UBagWidget::SetOwningBagComponent(UBagComponent* BagComp)
{
    if (OwningBagComponent)
    {
        OwningBagComponent->OnSlotUpdated.RemoveAll(this);
    }

    OwningBagComponent = BagComp;

    if (OwningBagComponent)
    {
        OwningBagComponent->OnSlotUpdated.AddDynamic(this, &UBagWidget::OnBagSlotUpdated);
    }
}

void UBagWidget::InitializeBag(const FS_ItemInfo& BagInfo)
{
    if (!ensure(BagInfo.ItemType == EItemType::Bag))
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeBag: Invalid item type"));
        return;
    }

    if (!ensure(InventoryGrid))
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeBag: Missing InventoryGrid"));
        return;
    }

    if (!ensure(OwningBagComponent))
    {
        UE_LOG(LogTemp, Error, TEXT("InitializeBag: Missing BagComponent"));
        return;
    }

    if (WindowTitle)
    {
        WindowTitle->SetText(BagInfo.ItemName);
    }

    InventoryGrid->ClearChildren();
    BagSlots.Empty();

    CreateBagSlots();

    // Load existing state from bag component
    const TArray<FBagSlotState>& CurrentStates = OwningBagComponent->GetSlotStates();
    UE_LOG(LogTemp, Warning, TEXT("Loading %d states from bag component"), CurrentStates.Num());

    for (int32 i = 0; i < CurrentStates.Num() && i < BagSlots.Num(); ++i)
    {
        const FBagSlotState& SlotState = CurrentStates[i];
        if (!SlotState.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Restoring slot %d: %s (x%d)"), 
                i, 
                *SlotState.ItemInfo.ItemName.ToString(),
                SlotState.Quantity);
            BagSlots[i]->SetItemDetails(SlotState.ItemInfo, SlotState.Quantity);
        }
    }
}

void UBagWidget::CreateBagSlots()
{
    if (!InventoryGrid || !OwningBagComponent)
        return;

    int32 TotalSlots = OwningBagComponent->GetBagSlots();
    int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalSlots)));
    int32 Rows = FMath::CeilToInt(static_cast<float>(TotalSlots) / Columns);

    UE_LOG(LogTemp, Warning, TEXT("CreateBagSlots -> Creating %dx%d grid for %d slots"), 
        Rows, Columns, TotalSlots);

    const FSoftClassPath SlotClassPath(TEXT("/Game/Inventory/Widgets/WBP_InventorySlot.WBP_InventorySlot_C"));
    if (UClass* SlotClass = SlotClassPath.TryLoadClass<UInventorySlotWidget>())
    {
        for (int32 r = 0; r < Rows; ++r)
        {
            for (int32 c = 0; c < Columns && BagSlots.Num() < TotalSlots; ++c)
            {
                UInventorySlotWidget* NewSlot = CreateWidget<UInventorySlotWidget>(this, SlotClass);
                if (NewSlot)
                {
                    if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(InventoryGrid->AddChild(NewSlot)))
                    {
                        GridSlot->SetRow(r);
                        GridSlot->SetColumn(c);
                    }
                    BagSlots.Add(NewSlot);
                }
            }
        }
    }
}

void UBagWidget::OnCloseButtonClicked()
{
    if (OwningBagComponent)
    {
        OwningBagComponent->SaveState();
        OwningBagComponent->CloseBag();
    }
    RemoveFromParent();
}

void UBagWidget::OnBagSlotUpdated(int32 SlotIndex, const FS_ItemInfo& ItemInfo, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("OnBagSlotUpdated: Slot %d -> %s (x%d)"), 
        SlotIndex, 
        *ItemInfo.ItemName.ToString(), 
        Quantity);

    if (BagSlots.IsValidIndex(SlotIndex))
    {
        if (Quantity > 0)
        {
            BagSlots[SlotIndex]->SetItemDetails(ItemInfo, Quantity);
        }
        else
        {
            BagSlots[SlotIndex]->ClearSlot();
        }

        // Request weight update whenever slot changes
        if (OwningBagComponent)
        {
            OwningBagComponent->RequestWeightUpdate();
        }
    }
}

bool UBagWidget::ValidateSlotIndex(int32 SlotIndex) const
{
    return (SlotIndex >= 0 && SlotIndex < BagSlots.Num());
}