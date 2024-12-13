// InventorySlotWidget.cpp
#include "InventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InventoryDragDropOperation.h"
#include "DragDropVisual.h"

UInventorySlotWidget::UInventorySlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ItemQuantity(0)
    , bIsInDragOperation(false)
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
}

void UInventorySlotWidget::UpdateVisuals()
{
    UE_LOG(LogTemp, Warning, TEXT("UpdateVisuals - ItemQuantity: %d, HasIcon: %s"), 
        ItemQuantity, 
        CurrentItemInfo.ItemIcon ? TEXT("Yes") : TEXT("No"));

    if (ItemIcon)
    {
        if (CurrentItemInfo.ItemIcon)
        {
            ItemIcon->SetBrushFromTexture(CurrentItemInfo.ItemIcon);
            ItemIcon->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Warning, TEXT("Set icon visible"));
        }
        else
        {
            ItemIcon->SetVisibility(ESlateVisibility::Hidden);
            UE_LOG(LogTemp, Warning, TEXT("Set icon hidden"));
        }
    }

    if (QuantityText)
    {
        if (ItemQuantity > 1)
        {
            QuantityText->SetText(FText::AsNumber(ItemQuantity));
            QuantityText->SetVisibility(ESlateVisibility::Visible);
            UE_LOG(LogTemp, Warning, TEXT("Set quantity text: %d"), ItemQuantity);
        }
        else
        {
            QuantityText->SetVisibility(ESlateVisibility::Hidden);
            UE_LOG(LogTemp, Warning, TEXT("Hide quantity text"));
        }
    }
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("MouseDown - Starting ItemQuantity: %d"), ItemQuantity);
        
        // Save the current state
        DraggedItemInfo = CurrentItemInfo;
        bool bIsSplitting = false;

        if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
        {
            DraggedQuantity = 1;
            ItemQuantity -= 1;
            bIsSplitting = true;
            // Store the original item info for the remaining stack
            FS_ItemInfo RemainingItemInfo = CurrentItemInfo;
            UE_LOG(LogTemp, Warning, TEXT("Shift-Split: Dragged: %d, Remaining: %d"), DraggedQuantity, ItemQuantity);
    
            // Update the current slot with remaining items
            SetItemDetails(RemainingItemInfo, ItemQuantity);
        }

        else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
        {
            DraggedQuantity = ItemQuantity / 2;
            ItemQuantity -= DraggedQuantity;
            bIsSplitting = true;
            UE_LOG(LogTemp, Warning, TEXT("Ctrl-Split: Dragged: %d, Remaining: %d"), DraggedQuantity, ItemQuantity);
        }
        else
        {
            DraggedQuantity = ItemQuantity;
            UE_LOG(LogTemp, Warning, TEXT("Full Stack Drag: %d"), DraggedQuantity);
            ClearSlot();
        }

        // Only update visuals if we're splitting
        if (bIsSplitting)
        {
            UE_LOG(LogTemp, Warning, TEXT("Updating slot with remaining quantity: %d"), ItemQuantity);
            CurrentItemInfo.ItemIcon = DraggedItemInfo.ItemIcon;  // Ensure icon is preserved
            UpdateVisuals();
        }

        bIsInDragOperation = true;
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Drag Detected ==="));
    UE_LOG(LogTemp, Warning, TEXT("Current Slot Quantity: %d"), ItemQuantity);
    UE_LOG(LogTemp, Warning, TEXT("Dragged Quantity: %d"), DraggedQuantity);

    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    
    if (DragDropOp)
    {
        // Set up drag operation data
        DragDropOp->DraggedItem = DraggedItemInfo;
        DragDropOp->OriginalQuantity = DraggedQuantity;
        DragDropOp->SourceSlot = this;
        DragDropOp->bSplitStack = InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown();

        UDragDropVisual* DragVisual = CreateWidget<UDragDropVisual>(this, LoadClass<UDragDropVisual>(nullptr, TEXT("/Game/Inventory/Widgets/WBP_DragVisual.WBP_DragVisual_C")));
        
        if (DragVisual)
        {
            DragVisual->SetItemIcon(DraggedItemInfo.ItemIcon);
            DragDropOp->DefaultDragVisual = DragVisual;
            DragDropOp->Pivot = EDragPivot::MouseDown;
        }

        UE_LOG(LogTemp, Warning, TEXT("After Drag Setup - Slot Quantity: %d"), ItemQuantity);
        OutOperation = DragDropOp;
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UE_LOG(LogTemp, Warning, TEXT("=== On Drop ==="));
    
    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryDragDrop)
        return false;

    // If dropping on same slot, do nothing
    if (InventoryDragDrop->SourceSlot == this)
        return false;

    // Handle stack merging
    if (ItemQuantity > 0 && CurrentItemInfo.ItemID == InventoryDragDrop->DraggedItem.ItemID)
    {
        int32 SpaceAvailable = CurrentItemInfo.MaxStackSize - ItemQuantity;
        if (SpaceAvailable > 0)
        {
            int32 AmountToAdd = FMath::Min(SpaceAvailable, InventoryDragDrop->OriginalQuantity);
            ItemQuantity += AmountToAdd;
            UpdateVisuals();

            // If this was a split operation, the source slot should keep its remaining items
            if (InventoryDragDrop->bSplitStack)
            {
                return true;
            }
            else
            {
                InventoryDragDrop->SourceSlot->ClearSlot();
            }
            return true;
        }
    }

    // Handle normal swap
    FS_ItemInfo TargetItem = CurrentItemInfo;
    int32 TargetQuantity = ItemQuantity;

    SetItemDetails(InventoryDragDrop->DraggedItem, InventoryDragDrop->OriginalQuantity);

    // Only update source slot if it's not a split operation
    if (!InventoryDragDrop->bSplitStack)
    {
        if (TargetQuantity > 0)
        {
            InventoryDragDrop->SourceSlot->SetItemDetails(TargetItem, TargetQuantity);
        }
        else
        {
            InventoryDragDrop->SourceSlot->ClearSlot();
        }
    }

    return true;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    bIsInDragOperation = false;
}