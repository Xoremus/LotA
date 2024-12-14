// InventorySlotWidget.cpp
#include "InventorySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "DragDropVisual.h"
#include "BagWidget.h"
#include "BagComponent.h"
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

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    // Handle right-click for bags
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (CurrentItemInfo.ItemType == EItemType::Bag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Right click on bag detected"));
            OpenBag();
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Save the current state for potential dragging
        DraggedItemInfo = CurrentItemInfo;
        
        if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
        {
            DraggedQuantity = 1;
            // We'll update ItemQuantity only when drag actually starts
        }
        else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
        {
            DraggedQuantity = ItemQuantity / 2;
            // We'll update ItemQuantity only when drag actually starts
        }
        else
        {
            DraggedQuantity = ItemQuantity;
        }

        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    
    if (DragDropOp)
    {
        bool bIsSplitting = InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown();
        
        // Set up drag operation data
        DragDropOp->DraggedItem = DraggedItemInfo;
        DragDropOp->OriginalQuantity = DraggedQuantity;
        DragDropOp->SourceSlot = this;
        DragDropOp->bSplitStack = bIsSplitting;

        // Update the source slot's quantity
        if (bIsSplitting)
        {
            ItemQuantity -= DraggedQuantity;
            UpdateVisuals();
        }
        else
        {
            // Hide the visuals but don't clear the data yet
            if (ItemIcon)
                ItemIcon->SetVisibility(ESlateVisibility::Hidden);
            if (QuantityText)
                QuantityText->SetVisibility(ESlateVisibility::Hidden);
        }

        // Create and set up the drag visual
        UDragDropVisual* DragVisual = CreateWidget<UDragDropVisual>(this, LoadClass<UDragDropVisual>(nullptr, TEXT("/Game/Inventory/Widgets/WBP_DragVisual.WBP_DragVisual_C")));
        if (DragVisual)
        {
            DragVisual->SetItemIcon(DraggedItemInfo.ItemIcon);
            DragVisual->SetQuantityText(DraggedQuantity);
            DragDropOp->DefaultDragVisual = DragVisual;
            DragDropOp->Pivot = EDragPivot::MouseDown;
        }

        OutOperation = DragDropOp;
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
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
    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (InventoryDragDrop && InventoryDragDrop->SourceSlot == this)
    {
        if (!InventoryDragDrop->bSplitStack)
        {
            // Restore visibility
            UpdateVisuals();
        }
    }
}

void UInventorySlotWidget::OpenBag()
{
    if (CurrentItemInfo.ItemType != EItemType::Bag)
        return;

    // Find BagComponent in owner's components
    APawn* OwningPawn = GetOwningPlayerPawn();
    if (!OwningPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get owning pawn"));
        return;
    }

    // Add debug logging
    UE_LOG(LogTemp, Warning, TEXT("Looking for BagComponent on pawn: %s"), *OwningPawn->GetName());

    UBagComponent* BagComp = OwningPawn->FindComponentByClass<UBagComponent>();
    if (!BagComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("BagComponent not found on pawn"));
        // If BagComponent isn't found, we might need to create it
        BagComp = NewObject<UBagComponent>(OwningPawn, TEXT("BagComponent"));
        BagComp->RegisterComponent();
    }

    if (BagComp->IsBagOpen())
    {
        UE_LOG(LogTemp, Warning, TEXT("Bag is already open"));
        return;
    }

    // Initialize the bag component
    BagComp->InitializeBag(CurrentItemInfo);

    // Open the bag through the component
    if (BagComp->OpenBag())
    {
        APlayerController* PC = GetOwningPlayer();
        if (!PC) return;

        const FSoftClassPath BagWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_BagWidget.WBP_BagWidget_C"));
        TSubclassOf<UBagWidget> BagWidgetClass = BagWidgetPath.TryLoadClass<UBagWidget>();

        if (BagWidgetClass)
        {
            UBagWidget* NewBagWidget = CreateWidget<UBagWidget>(PC, BagWidgetClass);
            if (NewBagWidget)
            {
                NewBagWidget->SetOwningBagComponent(BagComp);
                NewBagWidget->InitializeBag(CurrentItemInfo);
                NewBagWidget->AddToViewport();
                UE_LOG(LogTemp, Warning, TEXT("Bag widget created and added to viewport"));
            }
        }
    }
}