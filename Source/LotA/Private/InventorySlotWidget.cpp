// InventorySlotWidget.cpp
#include "InventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "DragDropVisual.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InventoryDragDropOperation.h"

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
    if (bIsInDragOperation)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempting to clear slot during drag operation - ignoring"));
        return;
    }

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
    if (!CurrentItemInfo.ItemIcon || ItemQuantity <= 0)
    {
        if (ItemIcon)
            ItemIcon->SetVisibility(ESlateVisibility::Hidden);
        if (QuantityText)
            QuantityText->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    if (ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(CurrentItemInfo.ItemIcon);
        ItemIcon->SetVisibility(ESlateVisibility::Visible);
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

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Store the original data for the drag operation
        DraggedItemInfo = CurrentItemInfo;
        
        if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
        {
            DraggedQuantity = 1;
            // Just reduce quantity, don't clear
            ItemQuantity -= 1;
            UpdateVisuals();
        }
        else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
        {
            DraggedQuantity = ItemQuantity / 2;
            // Just reduce quantity, don't clear
            ItemQuantity -= DraggedQuantity;
            UpdateVisuals();
        }
        else
        {
            DraggedQuantity = ItemQuantity;
            ClearSlot();
        }

        bIsInDragOperation = true;
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    
    if (DragDropOp)
    {
        // Store the original item data
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

        OutOperation = DragDropOp;
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryDragDrop || !InventoryDragDrop->SourceSlot)
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

            // Update source slot
            UInventorySlotWidget* SourceSlot = Cast<UInventorySlotWidget>(InventoryDragDrop->SourceSlot);
            if (SourceSlot)
            {
                if (SourceSlot->ItemQuantity > AmountToAdd)
                {
                    SourceSlot->ItemQuantity -= AmountToAdd;
                    SourceSlot->UpdateVisuals();
                }
                else
                {
                    SourceSlot->ClearSlot();
                }
            }
            return true;
        }
    }

    // Handle normal swap
    FS_ItemInfo TempItem = CurrentItemInfo;
    int32 TempQuantity = ItemQuantity;

    SetItemDetails(InventoryDragDrop->DraggedItem, InventoryDragDrop->OriginalQuantity);

    if (UInventorySlotWidget* SourceSlot = Cast<UInventorySlotWidget>(InventoryDragDrop->SourceSlot))
    {
        if (TempQuantity > 0)
        {
            SourceSlot->SetItemDetails(TempItem, TempQuantity);
        }
        else
        {
            SourceSlot->ClearSlot();
        }
    }

    bIsInDragOperation = false;
    return true;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    bIsInDragOperation = false;
}