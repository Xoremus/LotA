#include "InventorySlotWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "BagWidget.h"
#include "LotA/LotACharacter.h"
#include "BagComponent.h"
#include "Components/PanelWidget.h"
#include "MainInventoryWidget.h"
#include "DragDropVisual.h"
#include "InventoryDragDropOperation.h"
#include "DestroyConfirmationWidget.h"

UInventorySlotWidget::UInventorySlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ItemQuantity(0)
    , bIsInDragOperation(false)
    , DraggedQuantity(0)
{
}

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ClearSlot();
}

int32 UInventorySlotWidget::GetQuantity() const
{
    return ItemQuantity;
}

const FS_ItemInfo& UInventorySlotWidget::GetItemInfo() const
{
    return CurrentItemInfo;
}

void UInventorySlotWidget::SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity)
{
    CurrentItemInfo = InItemInfo;
    ItemQuantity    = Quantity;
    UpdateVisuals();

    // Update main inventory weight if needed
    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->UpdateInventoryWeight();
    }
}

void UInventorySlotWidget::ClearSlot()
{
    if (ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(nullptr);
        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }
    if (QuantityText)
    {
        QuantityText->SetText(FText::GetEmpty());
        QuantityText->SetVisibility(ESlateVisibility::Hidden);
    }
    CurrentItemInfo = FS_ItemInfo();
    ItemQuantity    = 0;
}

UBagComponent* UInventorySlotWidget::GetParentBagComponent() const
{
    if (UPanelWidget* ParentPanel = GetParent())
    {
        if (UBagWidget* BagW = Cast<UBagWidget>(ParentPanel->GetParent()))
        {
            return BagW->GetOwningBagComponent();
        }
    }
    return nullptr;
}

bool UInventorySlotWidget::TryGetParentBagSlotIndex(int32& OutSlotIndex) const
{
    if (UPanelWidget* ParentPanel = GetParent())
    {
        if (UBagWidget* BagW = Cast<UBagWidget>(ParentPanel->GetParent()))
        {
            const TArray<UInventorySlotWidget*>& Slots = BagW->GetBagSlots();
            OutSlotIndex = Slots.Find(const_cast<UInventorySlotWidget*>(this));
            return (OutSlotIndex != INDEX_NONE);
        }
    }
    return false;
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnMouseButtonDown on item: %s (qty=%d), Button=%s"),
           *GetName(),
           *CurrentItemInfo.ItemName.ToString(),
           ItemQuantity,
           *InMouseEvent.GetEffectingButton().ToString());

    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // If it's a Bag, open it
        if (CurrentItemInfo.ItemType == EItemType::Bag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Right-clicked bag: %s -> OpenBag"), *CurrentItemInfo.ItemName.ToString());
            OpenBag();
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Prepare for drag
        DraggedItemInfo = CurrentItemInfo;
        UE_LOG(LogTemp, Warning, TEXT("Left-click -> Drag item: %s, qty=%d"), *CurrentItemInfo.ItemName.ToString(), ItemQuantity);
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                                UDragDropOperation*& OutOperation)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragDetected -> item: %s, qty=%d"),
           *GetName(),
           *CurrentItemInfo.ItemName.ToString(),
           ItemQuantity);

    if (ItemQuantity <= 0) return;

    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(
        UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    if (!DragDropOp) return;

    // If it's a bag, never split
    if (CurrentItemInfo.ItemType == EItemType::Bag)
    {
        DraggedQuantity = ItemQuantity;
        DraggedItemInfo = CurrentItemInfo;
        ClearSlot();
        DragDropOp->bSplitStack = false;

        UE_LOG(LogTemp, Warning, TEXT("Dragging a BAG item -> forcibly clearing slot."));

        // Force close if open
        if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
        {
            FName BagKey = *FString::Printf(TEXT("Bag_%s"), *CurrentItemInfo.ItemID.ToString());
            if (UBagComponent* BagComp = Character->FindBagByKey(BagKey))
            {
                if (BagComp->IsBagOpen())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Forcing bag %s to close."), *BagKey.ToString());
                    BagComp->ForceClose();
                }
            }
        }
    }
    else if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
    {
        DraggedQuantity = 1;
        ItemQuantity   -= 1;
        DraggedItemInfo = CurrentItemInfo;
        DragDropOp->bSplitStack = true;

        UE_LOG(LogTemp, Warning, TEXT("Shift-dragging 1 out of %d."), ItemQuantity + 1);

        UpdateVisuals();
    }
    else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
    {
        DraggedQuantity = ItemQuantity / 2;
        ItemQuantity   -= DraggedQuantity;
        DraggedItemInfo = CurrentItemInfo;
        DragDropOp->bSplitStack = true;

        UE_LOG(LogTemp, Warning, TEXT("Ctrl-dragging half -> Dragged: %d, left: %d"), DraggedQuantity, ItemQuantity);

        UpdateVisuals();
    }
    else
    {
        // Full stack
        DraggedQuantity = ItemQuantity;
        DraggedItemInfo = CurrentItemInfo;
        ClearSlot();
        DragDropOp->bSplitStack = false;

        UE_LOG(LogTemp, Warning, TEXT("Dragging full stack of %d."), DraggedQuantity);
    }

    DragDropOp->DraggedItem      = DraggedItemInfo;
    DragDropOp->OriginalQuantity = DraggedQuantity;
    DragDropOp->SourceSlot       = this;

    // If in a bag, remove from that bag
    if (UBagComponent* BagComp = GetParentBagComponent())
    {
        int32 SlotIndex;
        if (TryGetParentBagSlotIndex(SlotIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("Removing item from bag slot: %d"), SlotIndex);
            BagComp->TryRemoveItem(SlotIndex);
        }
    }

    // Optionally create a drag visual
    const FSoftClassPath DragVisualPath(TEXT("/Game/Inventory/Widgets/WBP_DragVisual.WBP_DragVisual_C"));
    if (TSubclassOf<UDragDropVisual> DragVisualClass = DragVisualPath.TryLoadClass<UDragDropVisual>())
    {
        if (UDragDropVisual* Visual = CreateWidget<UDragDropVisual>(this, DragVisualClass))
        {
            Visual->SetItemIcon(DraggedItemInfo.ItemIcon);
            Visual->SetQuantityText(DraggedQuantity);
            DragDropOp->DefaultDragVisual = Visual;
            DragDropOp->Pivot             = EDragPivot::MouseDown;
        }
    }

    bIsInDragOperation = true;
    OutOperation       = DragDropOp;

    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->UpdateInventoryWeight();
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragDrop) return false;

    UE_LOG(LogTemp, Warning, TEXT("NativeOnDrop: Dropping %s (x%d) from slot %s"), 
           *DragDrop->DraggedItem.ItemName.ToString(), 
           DragDrop->OriginalQuantity,
           *DragDrop->SourceSlot->GetName());

    // Dropping onto same slot
    if (DragDrop->SourceSlot == this)
    {
        SetItemDetails(DragDrop->DraggedItem, DragDrop->OriginalQuantity);
        return true;
    }

    // If dropping into a Bag slot
    if (UBagComponent* TargetBagComp = GetParentBagComponent())
    {
        // Prevent any bags from being placed in bags
        if (DragDrop->DraggedItem.ItemType == EItemType::Bag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Prevented dropping bag into another bag"));
            DragDrop->SourceSlot->FindAndRestoreToAvailableSlot();
            return false;
        }

        int32 TargetSlotIndex;
        if (TryGetParentBagSlotIndex(TargetSlotIndex))
        {
            if (!TargetBagComp->CanAcceptItem(DragDrop->DraggedItem, TargetSlotIndex))
            {
                DragDrop->SourceSlot->FindAndRestoreToAvailableSlot();
                return false;
            }

            // Attempt stacking
            if (ItemQuantity > 0 && CurrentItemInfo.ItemID == DragDrop->DraggedItem.ItemID)
            {
                int32 Space = CurrentItemInfo.MaxStackSize - ItemQuantity;
                if (Space > 0)
                {
                    int32 AmountToAdd = FMath::Min(Space, DragDrop->OriginalQuantity);
                    
                    if (TargetBagComp->TryAddItem(DragDrop->DraggedItem, ItemQuantity + AmountToAdd, TargetSlotIndex))
                    {
                        if (!DragDrop->bSplitStack)
                        {
                            DragDrop->SourceSlot->ClearSlot();
                        }
                        return true;
                    }
                }
            }

            // Normal add
            if (TargetBagComp->TryAddItem(DragDrop->DraggedItem, DragDrop->OriginalQuantity, TargetSlotIndex))
            {
                if (!DragDrop->bSplitStack)
                {
                    DragDrop->SourceSlot->ClearSlot();
                }
                return true;
            }
        }
    }

    // Handle main inventory slots
    // Attempt stacking if same item
    if (ItemQuantity > 0 && CurrentItemInfo.ItemID == DragDrop->DraggedItem.ItemID)
    {
        int32 Space = CurrentItemInfo.MaxStackSize - ItemQuantity;
        if (Space > 0)
        {
            int32 ToAdd = FMath::Min(Space, DragDrop->OriginalQuantity);
            ItemQuantity += ToAdd;
            UpdateVisuals();

            if (!DragDrop->bSplitStack)
            {
                DragDrop->SourceSlot->ClearSlot();
            }
            
            if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
            {
                MainInv->UpdateInventoryWeight();
            }
            return true;
        }
    }

    // Swap items
    FS_ItemInfo OldItem = CurrentItemInfo;
    int32 OldQuantity = ItemQuantity;

    SetItemDetails(DragDrop->DraggedItem, DragDrop->OriginalQuantity);

    if (!DragDrop->bSplitStack)
    {
        if (OldQuantity > 0)
        {
            DragDrop->SourceSlot->SetItemDetails(OldItem, OldQuantity);
        }
        else
        {
            DragDrop->SourceSlot->ClearSlot();
        }
    }

    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->UpdateInventoryWeight();
    }
    
    return true;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragDrop = Cast<UInventoryDragDropOperation>(InOperation);

    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragCancelled -> valid operation? %s"),
           *GetName(),
           (DragDrop ? TEXT("Yes") : TEXT("No")));

    if (!DragDrop)
    {
        bIsInDragOperation = false;
        return;
    }

    FVector2D MousePos = InDragDropEvent.GetScreenSpacePosition();
    if (UPanelWidget* ParentPanel = GetParent())
    {
        FGeometry PanelGeo = ParentPanel->GetCachedGeometry();
        FVector2D PPos    = PanelGeo.GetAbsolutePosition();
        FVector2D PSize   = PanelGeo.GetAbsoluteSize();

        bool bOutside = (MousePos.X < PPos.X || MousePos.X > (PPos.X + PSize.X)
                      || MousePos.Y < PPos.Y || MousePos.Y > (PPos.Y + PSize.Y));

        UE_LOG(LogTemp, Warning, TEXT("DragCancelled -> bOutside=%s, bag item? %s"),
            bOutside ? TEXT("true") : TEXT("false"),
            (DragDrop->DraggedItem.ItemType == EItemType::Bag) ? TEXT("Yes") : TEXT("No"));

        // If inside or if bag item, restore
        if (!bOutside || DragDrop->DraggedItem.ItemType == EItemType::Bag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Restoring to available slot if possible..."));
            if (!FindAndRestoreToAvailableSlot())
            {
                UE_LOG(LogTemp, Warning, TEXT("No empty slot found -> revert."));
                if (DragDrop->bSplitStack)
                {
                    ItemQuantity += DragDrop->OriginalQuantity;
                }
                else
                {
                    SetItemDetails(DraggedItemInfo, DraggedQuantity);
                }
            }
        }
        else
        {
            // Prompt destroy
            const FSoftClassPath DestroyWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_Destroy.WBP_Destroy_C"));
            if (TSubclassOf<UDestroyConfirmationWidget> DestroyWidgetClass = DestroyWidgetPath.TryLoadClass<UDestroyConfirmationWidget>())
            {
                if (UDestroyConfirmationWidget* DestroyWidget = CreateWidget<UDestroyConfirmationWidget>(GetOwningPlayer(), DestroyWidgetClass))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Creating DestroyConfirmationWidget for item %s"), *DraggedItemInfo.ItemName.ToString());
                    DestroyWidget->SetItemToDestroy(DraggedItemInfo, DraggedQuantity);
                    DestroyWidget->OnDestroyConfirmed.AddDynamic(this, &UInventorySlotWidget::OnItemDestroyConfirmed);
                    DestroyWidget->OnDestroyCancelled.AddDynamic(this, &UInventorySlotWidget::OnItemDestroyCancelled);
                    DestroyWidget->AddToViewport();
                    DestroyWidget->SetPositionInViewport(MousePos);
                    return;
                }
            }
        }
    }

    bIsInDragOperation = false;
}

bool UInventorySlotWidget::FindAndRestoreToAvailableSlot()
{
    UE_LOG(LogTemp, Warning, TEXT("FindAndRestoreToAvailableSlot for item: %s (x%d)"),
        *DraggedItemInfo.ItemName.ToString(), DraggedQuantity);

    // If this slot is empty, restore here
    if (ItemQuantity == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Slot is empty -> restoring item here."));
        SetItemDetails(DraggedItemInfo, DraggedQuantity);
        DraggedQuantity = 0;
        DraggedItemInfo = FS_ItemInfo();
        return true;
    }

    // Otherwise, find first empty
    if (UInventorySlotWidget* EmptySlot = FindFirstAvailableSlot())
    {
        UE_LOG(LogTemp, Warning, TEXT("Found an empty slot: %s -> restoring item."), *EmptySlot->GetName());
        EmptySlot->SetItemDetails(DraggedItemInfo, DraggedQuantity);
        DraggedQuantity = 0;
        DraggedItemInfo = FS_ItemInfo();
        return true;
    }
    return false;
}

UInventorySlotWidget* UInventorySlotWidget::FindFirstAvailableSlot()
{
    if (UPanelWidget* ParentPanel = GetParent())
    {
        // If in a bag, search bag slots
        if (UBagWidget* ParentBag = Cast<UBagWidget>(ParentPanel->GetParent()))
        {
            UE_LOG(LogTemp, Warning, TEXT("Searching bag for an empty slot..."));
            for (UInventorySlotWidget* SlotW : ParentBag->GetBagSlots())
            {
                if (SlotW && SlotW->GetQuantity() == 0)
                {
                    return SlotW;
                }
            }
        }
        // Otherwise, search main inventory
        else if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
        {
            if (UInventoryWidget* Inv = MainInv->WBP_Inventory)
            {
                UE_LOG(LogTemp, Warning, TEXT("Searching main inventory for an empty slot..."));
                for (UInventorySlotWidget* SlotW : Inv->GetInventorySlots())
                {
                    if (SlotW && SlotW->GetQuantity() == 0)
                    {
                        return SlotW;
                    }
                }
            }
        }
    }
    return nullptr;
}

UMainInventoryWidget* UInventorySlotWidget::GetMainInventoryWidget() const
{
    // Climb up the hierarchy
    UWidget* Current = const_cast<UInventorySlotWidget*>(this);
    while (Current)
    {
        if (UMainInventoryWidget* MIW = Cast<UMainInventoryWidget>(Current))
        {
            return MIW;
        }
        Current = Current->GetParent();
    }

    // Fallback: search all
    if (APlayerController* PC = GetOwningPlayer())
    {
        TArray<UUserWidget*> Found;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Found, UMainInventoryWidget::StaticClass(), false);
        for (UUserWidget* W : Found)
        {
            if (UMainInventoryWidget* MIW = Cast<UMainInventoryWidget>(W))
            {
                return MIW;
            }
        }
    }
    return nullptr;
}

void UInventorySlotWidget::UpdateVisuals()
{
    if (ItemIcon)
    {
        if (CurrentItemInfo.ItemIcon)
        {
            ItemIcon->SetBrushFromTexture(CurrentItemInfo.ItemIcon);
            ItemIcon->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            ItemIcon->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    if (QuantityText)
    {
        if (ItemQuantity > 1)
        {
            QuantityText->SetText(FText::AsNumber(ItemQuantity));
            QuantityText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            QuantityText->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void UInventorySlotWidget::OpenBag()
{
    if (CurrentItemInfo.ItemType != EItemType::Bag)
        return;

    // Use ALotACharacter's system to spawn or find existing BagComponent
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
    {
        FName BagKey = *FString::Printf(TEXT("Bag_%s"), *CurrentItemInfo.ItemID.ToString());
        UBagComponent* BagComp = Character->FindBagByKey(BagKey);

        if (!BagComp || !IsValid(BagComp))
        {
            BagComp = Character->AddBagComponent(CurrentItemInfo);
            UE_LOG(LogTemp, Warning, TEXT("OpenBag -> Created new bag comp for %s"), *BagKey.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("OpenBag -> Using existing bag comp %s"), *BagComp->GetName());
        }

        if (BagComp && !BagComp->IsBagOpen())
        {
            if (BagComp->OpenBag())
            {
                // Create a separate BagWidget if you want a new window
                if (APlayerController* PC = GetOwningPlayer())
                {
                    const FSoftClassPath BagWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_BagWidget.WBP_BagWidget_C"));
                    if (TSubclassOf<UBagWidget> BagWidgetClass = BagWidgetPath.TryLoadClass<UBagWidget>())
                    {
                        if (UBagWidget* NewBagWidget = CreateWidget<UBagWidget>(PC, BagWidgetClass))
                        {
                            NewBagWidget->SetOwningBagComponent(BagComp);
                            NewBagWidget->InitializeBag(CurrentItemInfo);
                            NewBagWidget->AddToViewport(/*ZOrder=*/ 100);
                        }
                    }
                }
            }
        }
    }
}

void UInventorySlotWidget::OnItemDestroyConfirmed(const FS_ItemInfo& /*DestroyedItem*/)
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyConfirmed -> item destroyed permanently."));
    bIsInDragOperation = false;
    DraggedQuantity    = 0;
    DraggedItemInfo    = FS_ItemInfo();
}

void UInventorySlotWidget::OnItemDestroyCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyCancelled -> restoring to an available slot."));
    FindAndRestoreToAvailableSlot();
    bIsInDragOperation = false;
}
