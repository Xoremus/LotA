// InventorySlotWidget.cpp
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
    , bSuppressWeightUpdate(false)
    , DraggedQuantity(0)
{
}

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ClearSlot();
}

void UInventorySlotWidget::SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity)
{
    CurrentItemInfo = InItemInfo;
    ItemQuantity = Quantity;
    UpdateVisuals();

    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->RequestWeightUpdate();
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
    ItemQuantity = 0;

    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->RequestWeightUpdate();
    }
}

int32 UInventorySlotWidget::GetQuantity() const
{
    return ItemQuantity;
}

const FS_ItemInfo& UInventorySlotWidget::GetItemInfo() const
{
    return CurrentItemInfo;
}

UBagComponent* UInventorySlotWidget::GetParentBagComponent() const
{
    if (UPanelWidget* ParentPanel = GetParent())
    {
        // Use IsA to verify the widget type before casting
        UWidget* GrandParent = ParentPanel->GetParent();
        if (GrandParent && GrandParent->IsA<UBagWidget>())
        {
            UBagWidget* BagW = static_cast<UBagWidget*>(GrandParent);
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
    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnMouseButtonDown on item: %s (qty=%d), Button=%s"),
        *GetName(),
        *CurrentItemInfo.ItemName.ToString(),
        ItemQuantity,
        *InMouseEvent.GetEffectingButton().ToString());

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
        UE_LOG(LogTemp, Warning, TEXT("Left-click -> Drag item: %s, qty=%d"), 
            *CurrentItemInfo.ItemName.ToString(), ItemQuantity);
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,  
                                UDragDropOperation*& OutOperation)
{
    if (ItemQuantity <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragDetected -> No items to drag"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragDetected -> item: %s, qty=%d"),
        *GetName(),
        *CurrentItemInfo.ItemName.ToString(),
        ItemQuantity);

    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(
        UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    
    if (!DragDropOp) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create drag operation"));
        return;
    }

    // If it's a bag, handle bag-specific logic
    if (CurrentItemInfo.ItemType == EItemType::Bag)
    {
        ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn());
        if (ensure(Character))
        {
            FName BagKey = *FString::Printf(TEXT("Bag_%s"), *CurrentItemInfo.ItemID.ToString());
            if (UBagComponent* BagComp = Character->FindBagByKey(BagKey))
            {
                if (BagComp->IsBagOpen())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Forcing bag %s to close."), *BagKey.ToString());
                    BagComp->ForceClose();  // This will handle removal
                }
            }
        }

        DraggedQuantity = ItemQuantity;
        DraggedItemInfo = CurrentItemInfo;
        ClearSlot();
        DragDropOp->bSplitStack = false;
    
        UE_LOG(LogTemp, Warning, TEXT("Dragging a BAG item -> forcibly clearing slot."));
    }
    else if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
    {
        DraggedQuantity = 1;
        ItemQuantity -= 1;
        DraggedItemInfo = CurrentItemInfo;
        DragDropOp->bSplitStack = true;

        UE_LOG(LogTemp, Warning, TEXT("Shift-dragging 1 out of %d."), ItemQuantity + 1);
        UpdateVisuals();
    }
    else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
    {
        DraggedQuantity = ItemQuantity / 2;
        ItemQuantity -= DraggedQuantity;
        DraggedItemInfo = CurrentItemInfo;
        DragDropOp->bSplitStack = true;

        UE_LOG(LogTemp, Warning, TEXT("Ctrl-dragging half -> Dragged: %d, left: %d"), 
            DraggedQuantity, ItemQuantity);
        UpdateVisuals();
    }
    else
    {
        DraggedQuantity = ItemQuantity;
        DraggedItemInfo = CurrentItemInfo;
        ClearSlot();
        DragDropOp->bSplitStack = false;

        UE_LOG(LogTemp, Warning, TEXT("Dragging full stack of %d."), DraggedQuantity);
    }

    DragDropOp->DraggedItem = DraggedItemInfo;
    DragDropOp->OriginalQuantity = DraggedQuantity;
    DragDropOp->SourceSlot = this;

    const FSoftClassPath DragVisualPath(TEXT("/Game/Inventory/Widgets/WBP_DragVisual.WBP_DragVisual_C"));
    if (TSubclassOf<UDragDropVisual> DragVisualClass = DragVisualPath.TryLoadClass<UDragDropVisual>())
    {
        if (UDragDropVisual* Visual = CreateWidget<UDragDropVisual>(this, DragVisualClass))
        {
            Visual->SetItemIcon(DraggedItemInfo.ItemIcon);
            Visual->SetQuantityText(DraggedQuantity);
            DragDropOp->DefaultDragVisual = Visual;
            DragDropOp->Pivot = EDragPivot::MouseDown;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create drag visual widget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load drag visual widget class"));
    }

    bIsInDragOperation = true;
    OutOperation = DragDropOp;

    if (!bSuppressWeightUpdate)
    {
        if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
        {
            MainInv->RequestWeightUpdate();
        }
    }
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragDrop)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragCancelled -> invalid operation"), *GetName());
        bIsInDragOperation = false;
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] NativeOnDragCancelled -> valid operation"), *GetName());

    FVector2D MousePos = InDragDropEvent.GetScreenSpacePosition();
    if (UPanelWidget* ParentPanel = GetParent())
    {
        FGeometry PanelGeo = ParentPanel->GetCachedGeometry();
        FVector2D PPos = PanelGeo.GetAbsolutePosition();
        FVector2D PSize = PanelGeo.GetAbsoluteSize();

        bool bOutside = (MousePos.X < PPos.X || MousePos.X > (PPos.X + PSize.X) ||
                        MousePos.Y < PPos.Y || MousePos.Y > (PPos.Y + PSize.Y));

        UE_LOG(LogTemp, Warning, TEXT("DragCancelled -> bOutside=%s, bag item? %s"),
            bOutside ? TEXT("true") : TEXT("false"),
            (DragDrop->DraggedItem.ItemType == EItemType::Bag) ? TEXT("Yes") : TEXT("No"));

        if (!bOutside || DragDrop->DraggedItem.ItemType == EItemType::Bag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Restoring to available slot if possible..."));
            if (!FindAndRestoreToAvailableSlot())
            {
                UE_LOG(LogTemp, Warning, TEXT("No empty slot found -> revert."));
                if (DragDrop->bSplitStack)
                {
                    ItemQuantity += DraggedQuantity;
                    UpdateVisuals();
                }
                else
                {
                            SetItemDetails(DraggedItemInfo, DraggedQuantity);
                }
            }
        }
        else
        {
            // Show destroy confirmation
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

bool UInventorySlotWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation
)
{
    UInventoryDragDropOperation* DragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragDrop)
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("NativeOnDrop: Dropping %s (x%d) from slot %s"),
        *DragDrop->DraggedItem.ItemName.ToString(),
        DragDrop->OriginalQuantity,
        *DragDrop->SourceSlot->GetName());

    // 1) Same slot => restore
    if (DragDrop->SourceSlot == this)
    {
        SetItemDetails(DragDrop->DraggedItem, DragDrop->OriginalQuantity);
        return true;
    }

    // 2) If dropping into a Bag slot
    if (UBagComponent* TargetBagComp = GetParentBagComponent())
    {
        int32 TargetSlotIndex;
        if (!TryGetParentBagSlotIndex(TargetSlotIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("NativeOnDrop -> Invalid target bag slot index"));
            DragDrop->SourceSlot->FindAndRestoreToAvailableSlot();
            return false;
        }

        // a) Try stacking if same item
        if (ItemQuantity > 0 && CurrentItemInfo.ItemID == DragDrop->DraggedItem.ItemID)
        {
            int32 Space = CurrentItemInfo.MaxStackSize - ItemQuantity;
            if (Space > 0)
            {
                int32 AmountToAdd = FMath::Min(Space, DragDrop->OriginalQuantity);
                int32 NewTotal    = ItemQuantity + AmountToAdd;

                // SERVER => real add
                TargetBagComp->ServerTryAddItem(DragDrop->DraggedItem, NewTotal, TargetSlotIndex);

                // If not split, remove from source
                if (!DragDrop->bSplitStack)
                {
                    if (UBagComponent* SourceBagComp = DragDrop->SourceSlot->GetParentBagComponent())
                    {
                        int32 SourceIndex;
                        if (DragDrop->SourceSlot->TryGetParentBagSlotIndex(SourceIndex))
                        {
                            SourceBagComp->ServerTryRemoveItem(SourceIndex);
                        }
                    }
                    else
                    {
                        DragDrop->SourceSlot->ClearSlot();
                    }
                }

                // Local UI update
                ItemQuantity = NewTotal;
                UpdateVisuals();

                if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
                {
                    MainInv->RequestWeightUpdate();
                }
                return true;
            }
        }
        // b) Otherwise place as new item
        else
        {
            TargetBagComp->ServerTryAddItem(DragDrop->DraggedItem, DragDrop->OriginalQuantity, TargetSlotIndex);

            if (!DragDrop->bSplitStack)
            {
                if (UBagComponent* SourceBagComp = DragDrop->SourceSlot->GetParentBagComponent())
                {
                    int32 SourceIndex;
                    if (DragDrop->SourceSlot->TryGetParentBagSlotIndex(SourceIndex))
                    {
                        SourceBagComp->ServerTryRemoveItem(SourceIndex);
                    }
                }
                else
                {
                    DragDrop->SourceSlot->ClearSlot();
                }
            }

            SetItemDetails(DragDrop->DraggedItem, DragDrop->OriginalQuantity);
            return true;
        }

        // If we get here => revert
        DragDrop->SourceSlot->FindAndRestoreToAvailableSlot();
        return false;
    }

    // 3) Main inventory => attempt stack
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
                MainInv->RequestWeightUpdate();
            }
            return true;
        }
    }

    // 4) Otherwise => swap
    FS_ItemInfo OldItem     = CurrentItemInfo;
    int32       OldQuantity = ItemQuantity;

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
        MainInv->RequestWeightUpdate();
    }

    return true;
}

void UInventorySlotWidget::OpenBag()
{
    if (!ensure(CurrentItemInfo.ItemType == EItemType::Bag))
    {
        UE_LOG(LogTemp, Error, TEXT("OpenBag: Attempted to open non-bag item"));
        return;
    }

    ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn());
    if (!ensure(Character))
    {
        UE_LOG(LogTemp, Error, TEXT("OpenBag: Invalid character"));
        return;
    }

    FName BagKey = *FString::Printf(TEXT("Bag_%s"), *CurrentItemInfo.ItemID.ToString());
    UBagComponent* BagComp = Character->FindBagByKey(BagKey);

    bool bNewBagCreated = false;
    if (!BagComp || !IsValid(BagComp))
    {
        BagComp = Character->AddBagComponent(CurrentItemInfo);
        if (!ensure(BagComp))
        {
            UE_LOG(LogTemp, Error, TEXT("OpenBag: Failed to create bag component"));
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("OpenBag: Created new bag comp for %s"), *BagKey.ToString());
        bNewBagCreated = true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("OpenBag: Using existing bag comp %s"), *BagComp->GetName());
    }

    if (!BagComp->IsBagOpen())
    {
        if (BagComp->OpenBag())
        {
            APlayerController* PC = GetOwningPlayer();
            if (!ensure(PC))
            {
                UE_LOG(LogTemp, Error, TEXT("OpenBag: Invalid player controller"));
                return;
            }

            const FSoftClassPath BagWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_BagWidget.WBP_BagWidget_C"));
            if (TSubclassOf<UBagWidget> BagWidgetClass = BagWidgetPath.TryLoadClass<UBagWidget>())
            {
                if (UBagWidget* NewBagWidget = CreateWidget<UBagWidget>(PC, BagWidgetClass))
                {
                    NewBagWidget->SetOwningBagComponent(BagComp);
                    NewBagWidget->InitializeBag(CurrentItemInfo);
                    NewBagWidget->AddToViewport(100);
                }
            }
        }
    }
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

    // Otherwise, find first empty slot
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

void UInventorySlotWidget::OnItemDestroyConfirmed(const FS_ItemInfo& /*DestroyedItem*/)
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyConfirmed -> item destroyed permanently."));
    bIsInDragOperation = false;
    DraggedQuantity = 0;
    DraggedItemInfo = FS_ItemInfo();
}

void UInventorySlotWidget::OnItemDestroyCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyCancelled -> restoring to an available slot."));
    FindAndRestoreToAvailableSlot();
    bIsInDragOperation = false;
}