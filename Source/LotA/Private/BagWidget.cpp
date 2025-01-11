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
        // Save bag state before destroying
        if (ALotACharacter* Character = Cast<ALotACharacter>(OwningBagComponent->GetOwner()))
        {
            Character->SaveBagState(OwningBagComponent);
        }
        OwningBagComponent->CloseBag();
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
    if (BagInfo.ItemType != EItemType::Bag)
    {
        UE_LOG(LogTemp, Error, TEXT("BagWidget::InitializeBag -> tried to init with non-bag item."));
        return;
    }

    if (WindowTitle)
    {
        WindowTitle->SetText(BagInfo.ItemName);
    }

    if (InventoryGrid && OwningBagComponent)
    {
        InventoryGrid->ClearChildren();
        BagSlots.Empty();

        CreateBagSlots();

        // Get saved state if any
        if (ALotACharacter* Character = Cast<ALotACharacter>(OwningBagComponent->GetOwner()))
        {
            FName BagKey = *FString::Printf(TEXT("Bag_%s"), *BagInfo.ItemID.ToString());
            FBagSavedState SavedState;
            if (Character->GetSavedBagState(BagKey, SavedState))
            {
                UE_LOG(LogTemp, Warning, TEXT("InitializeBag -> Restoring saved state for %s"), *BagKey.ToString());
                
                // Update bag component with saved state
                OwningBagComponent->SetSlotStates(SavedState.SlotStates);
                
                // Update UI for each slot
                for (int32 i = 0; i < SavedState.SlotStates.Num(); ++i)
                {
                    const FBagSlotState& SlotState = SavedState.SlotStates[i];
                    if (!SlotState.IsEmpty())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("InitializeBag -> Restoring slot %d: %s (x%d)"), 
                            i, 
                            *SlotState.ItemInfo.ItemName.ToString(), 
                            SlotState.Quantity);
                        UpdateSlotVisuals(i, SlotState.ItemInfo, SlotState.Quantity);
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("InitializeBag -> No saved state found for %s"), *BagKey.ToString());
            }
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
                    UE_LOG(LogTemp, Warning, TEXT("Created slot at row %d, column %d"), r, c);
                }
            }
        }
    }
}

void UBagWidget::OnCloseButtonClicked()
{
    if (OwningBagComponent)
    {
        // Save state before closing
        if (ALotACharacter* Char = Cast<ALotACharacter>(OwningBagComponent->GetOwner()))
        {
            UE_LOG(LogTemp, Warning, TEXT("OnCloseButtonClicked -> Saving state"));
            Char->SaveBagState(OwningBagComponent);
        }
        OwningBagComponent->CloseBag();
    }
    RemoveFromParent();
}

void UBagWidget::OnBagSlotUpdated(int32 SlotIndex, const FS_ItemInfo& ItemInfo, int32 Quantity)
{
    UE_LOG(LogTemp, Warning, TEXT("OnBagSlotUpdated -> Slot %d: %s (x%d)"), 
        SlotIndex, *ItemInfo.ItemName.ToString(), Quantity);
    UpdateSlotVisuals(SlotIndex, ItemInfo, Quantity);
    
    // Save state whenever a slot is updated
    if (OwningBagComponent && Cast<ALotACharacter>(OwningBagComponent->GetOwner()))
    {
        Cast<ALotACharacter>(OwningBagComponent->GetOwner())->SaveBagState(OwningBagComponent);
    }
}

void UBagWidget::UpdateSlotVisuals(int32 SlotIndex, const FS_ItemInfo& ItemInfo, int32 Quantity)
{
    if (ValidateSlotIndex(SlotIndex) && BagSlots[SlotIndex])
    {
        if (Quantity > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateSlotVisuals -> Setting slot %d: %s (x%d)"), 
                SlotIndex, *ItemInfo.ItemName.ToString(), Quantity);
            BagSlots[SlotIndex]->SetItemDetails(ItemInfo, Quantity);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UpdateSlotVisuals -> Clearing slot %d"), SlotIndex);
            BagSlots[SlotIndex]->ClearSlot();
        }
    }
}

bool UBagWidget::ValidateSlotIndex(int32 SlotIndex) const
{
    return (SlotIndex >= 0 && SlotIndex < BagSlots.Num());
}