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
#include "Components/PanelWidget.h"
#include "MainInventoryWidget.h"

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

UMainInventoryWidget* UInventorySlotWidget::GetMainInventoryWidget() const
{
    // Start from this widget
    UWidget* CurrentWidget = const_cast<UInventorySlotWidget*>(this);
    
    // Keep track of the widgets we've traversed for debugging
    TArray<FString> WidgetHierarchy;
    
    while (CurrentWidget)
    {
        WidgetHierarchy.Add(FString::Printf(TEXT("%s (%s)"), 
            *CurrentWidget->GetName(), 
            *CurrentWidget->GetClass()->GetName()));

        // Try to get the main inventory widget directly
        if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(CurrentWidget))
        {
            return MainInv;
        }

        // Try to get the owning user widget if this is a UserWidget
        if (UUserWidget* UserWidget = Cast<UUserWidget>(CurrentWidget))
        {
            UObject* Outer = UserWidget->GetOuter();
            if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(Outer))
            {
                return MainInv;
            }
        }

        // Move to the parent widget
        CurrentWidget = CurrentWidget->GetParent();
        
        // If we have no parent but we're in a UserWidget, try to get its outer
        if (!CurrentWidget && Cast<UUserWidget>(this))
        {
            UObject* Outer = GetOuter();
            while (Outer)
            {
                if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(Outer))
                {
                    return MainInv;
                }
                Outer = Outer->GetOuter();
            }
        }
    }

    // If we get here, log the hierarchy for debugging
    FString HierarchyString = TEXT("Widget Hierarchy:\n");
    for (int32 i = 0; i < WidgetHierarchy.Num(); ++i)
    {
        HierarchyString += FString::Printf(TEXT("%s%s\n"), 
            *FString::ChrN(i * 2, ' '), 
            *WidgetHierarchy[i]);
    }
    UE_LOG(LogTemp, Warning, TEXT("%s"), *HierarchyString);

    // As a last resort, try to find it in the world
    if (APlayerController* PC = GetOwningPlayer())
    {
        TArray<UUserWidget*> FoundWidgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UMainInventoryWidget::StaticClass(), false);
        
        for (UUserWidget* Widget : FoundWidgets)
        {
            if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(Widget))
            {
                UE_LOG(LogTemp, Warning, TEXT("Found MainInventoryWidget through world search: %s"), *MainInv->GetName());
                return MainInv;
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Could not find MainInventoryWidget in hierarchy - see above for widget tree"));
    return nullptr;
}

void UInventorySlotWidget::SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity)
{
    CurrentItemInfo = InItemInfo;
    ItemQuantity = Quantity;
    UpdateVisuals();

    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        UE_LOG(LogTemp, Warning, TEXT("Updating weight in SetItemDetails for item: %s, Quantity: %d"), 
            *InItemInfo.ItemName.ToString(), Quantity);
        MainInv->UpdateInventoryWeight();
    }
}

void UInventorySlotWidget::ClearSlot()
{
    UMainInventoryWidget* MainInv = GetMainInventoryWidget();
    bool hadItem = ItemQuantity > 0;
    
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

    // Only update weight if we actually removed an item
    if (hadItem && MainInv)
    {
        UE_LOG(LogTemp, Warning, TEXT("Updating weight after clearing slot"));
        MainInv->UpdateInventoryWeight();
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

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // If no item in slot, do nothing
    if (ItemQuantity <= 0)
        return FReply::Unhandled();

    // Handle right-click for bags
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (CurrentItemInfo.ItemType == EItemType::Bag)
        {
            OpenBag();
            return FReply::Handled();
        }
        return FReply::Unhandled();
    }

    // Handle left-click for drag operations
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // Just initiate drag detection, don't modify the slot yet
        DraggedItemInfo = CurrentItemInfo;
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }

    return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    // Create the drag-drop operation
    UInventoryDragDropOperation* DragDropOp = Cast<UInventoryDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UInventoryDragDropOperation::StaticClass()));
    
    if (DragDropOp)
    {
        // Handle stack splitting
        if (InMouseEvent.IsShiftDown() && ItemQuantity > 1)
        {
            DraggedQuantity = 1;
            ItemQuantity -= 1;
            UpdateVisuals();
        }
        else if (InMouseEvent.IsControlDown() && ItemQuantity > 1)
        {
            DraggedQuantity = ItemQuantity / 2;
            ItemQuantity -= DraggedQuantity;
            UpdateVisuals();
        }
        else
        {
            DraggedQuantity = ItemQuantity;
            ClearSlot();
        }

        // Setup drag operation
        DragDropOp->DraggedItem = DraggedItemInfo;
        DragDropOp->OriginalQuantity = DraggedQuantity;
        DragDropOp->SourceSlot = this;
        DragDropOp->bSplitStack = InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown();

        // Create drag visual
        UDragDropVisual* DragVisual = CreateWidget<UDragDropVisual>(this, LoadClass<UDragDropVisual>(nullptr, TEXT("/Game/Inventory/Widgets/WBP_DragVisual.WBP_DragVisual_C")));
        if (DragVisual)
        {
            DragVisual->SetItemIcon(DraggedItemInfo.ItemIcon);
            DragVisual->SetQuantityText(DraggedQuantity);
            DragDropOp->DefaultDragVisual = DragVisual;
            DragDropOp->Pivot = EDragPivot::MouseDown;
        }

        bIsInDragOperation = true;
        OutOperation = DragDropOp;

        // Update weight after modifying quantities
        if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
        {
            MainInv->UpdateInventoryWeight();
        }
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryDragDrop || InventoryDragDrop->SourceSlot == this)
        return false;

    bool bHandled = false;

    // Store original state in case we need to revert
    FS_ItemInfo OriginalItem = CurrentItemInfo;
    int32 OriginalQuantity = ItemQuantity;

    // Handle stacking case
    if (ItemQuantity > 0 && CurrentItemInfo.ItemID == InventoryDragDrop->DraggedItem.ItemID)
    {
        int32 SpaceAvailable = CurrentItemInfo.MaxStackSize - ItemQuantity;
        if (SpaceAvailable > 0)
        {
            int32 AmountToAdd = FMath::Min(SpaceAvailable, InventoryDragDrop->OriginalQuantity);
            ItemQuantity += AmountToAdd;
            UpdateVisuals();

            if (!InventoryDragDrop->bSplitStack)
            {
                InventoryDragDrop->SourceSlot->ClearSlot();
            }
            
            bHandled = true;
        }
    }
    // Handle swap case
    else
    {
        FS_ItemInfo TargetItem = CurrentItemInfo;
        int32 TargetQuantity = ItemQuantity;

        SetItemDetails(InventoryDragDrop->DraggedItem, InventoryDragDrop->OriginalQuantity);

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

        bHandled = true;
    }

    // If the drop was handled, check if we're in a bag and store the data
    if (bHandled)
    {
        // Get our parent widgets
        if (UPanelWidget* ParentPanel = GetParent())
        {
            if (UUserWidget* GrandParent = Cast<UUserWidget>(ParentPanel->GetParent()))
            {
                // Check if we're in a bag
                if (UBagWidget* BagWidget = Cast<UBagWidget>(GrandParent))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Drop handled in bag slot"));
                    
                    // Get the bag component
                    if (UBagComponent* BagComp = BagWidget->GetOwningBagComponent())
                    {
                        // Find our slot index
                        const TArray<UInventorySlotWidget*>& BagSlots = BagWidget->GetBagSlots();
                        int32 SlotIndex = BagSlots.Find(this);
                        
                        if (SlotIndex != INDEX_NONE)
                        {
                            // Store the data in the bag component
                            if (ItemQuantity > 0)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Storing item in bag slot %d: %s (Quantity: %d)"), 
                                    SlotIndex, *CurrentItemInfo.ItemName.ToString(), ItemQuantity);
                                    
                                BagComp->AddItem(SlotIndex, CurrentItemInfo, ItemQuantity);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Clearing bag slot %d"), SlotIndex);
                                BagComp->RemoveItem(SlotIndex);
                            }
                        }
                    }
                }
            }
        }
    }

    // Update inventory weight
    if (UMainInventoryWidget* MainInv = GetMainInventoryWidget())
    {
        MainInv->UpdateInventoryWeight();
    }

    return bHandled;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryDragDrop)
    {
        bIsInDragOperation = false;
        return;
    }

    FVector2D MousePos = InDragDropEvent.GetScreenSpacePosition();
    
    UPanelWidget* ParentPanel = GetParent();
    if (ParentPanel)
    {
        FGeometry ParentGeometry = ParentPanel->GetCachedGeometry();
        FVector2D InventoryPos = ParentGeometry.GetAbsolutePosition();
        FVector2D InventorySize = ParentGeometry.GetAbsoluteSize();

        bool bIsOutsideInventory = 
            MousePos.X < InventoryPos.X || 
            MousePos.X > (InventoryPos.X + InventorySize.X) ||
            MousePos.Y < InventoryPos.Y || 
            MousePos.Y > (InventoryPos.Y + InventorySize.Y);

        if (bIsOutsideInventory)
        {
            // Show destroy confirmation
            const FSoftClassPath DestroyWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_Destroy.WBP_Destroy_C"));
            TSubclassOf<UDestroyConfirmationWidget> DestroyWidgetClass = DestroyWidgetPath.TryLoadClass<UDestroyConfirmationWidget>();

            if (DestroyWidgetClass)
            {
                UDestroyConfirmationWidget* DestroyWidget = CreateWidget<UDestroyConfirmationWidget>(GetOwningPlayer(), DestroyWidgetClass);
                if (DestroyWidget)
                {
                    DestroyWidget->SetItemToDestroy(DraggedItemInfo, DraggedQuantity);
                    DestroyWidget->OnDestroyConfirmed.AddDynamic(this, &UInventorySlotWidget::OnItemDestroyConfirmed);
                    DestroyWidget->OnDestroyCancelled.AddDynamic(this, &UInventorySlotWidget::OnItemDestroyCancelled);
                    DestroyWidget->AddToViewport();
                    DestroyWidget->SetPositionInViewport(MousePos);
                    return;
                }
            }
        }
        else
        {
            // If returning to the original slot, restore the original quantity
            if (InventoryDragDrop->SourceSlot == this && (InventoryDragDrop->bSplitStack))
            {
                SetItemDetails(InventoryDragDrop->DraggedItem, ItemQuantity + DraggedQuantity);
                UE_LOG(LogTemp, Warning, TEXT("Restoring original stack quantity: %d"), ItemQuantity + DraggedQuantity);
            }
            else if (InventoryDragDrop->SourceSlot == this)
            {
                SetItemDetails(InventoryDragDrop->DraggedItem, InventoryDragDrop->OriginalQuantity);
            }
        }
    }

    bIsInDragOperation = false;
}

void UInventorySlotWidget::OnItemDestroyConfirmed(const FS_ItemInfo& DestroyedItem)
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyConfirmed: %s, Quantity: %d"), 
        *DestroyedItem.ItemName.ToString(), DraggedQuantity);

    // Debug log our widget hierarchy
    UWidget* CurrentWidget = this;
    int32 Depth = 0;
    FString Indent;
    
    while (CurrentWidget)
    {
        Indent = FString::ChrN(Depth * 2, ' ');
        UE_LOG(LogTemp, Warning, TEXT("%sWidget[%d]: %s (%s)"), 
            *Indent, Depth, 
            *CurrentWidget->GetName(), 
            *CurrentWidget->GetClass()->GetName());
        
        CurrentWidget = CurrentWidget->GetParent();
        Depth++;
    }

    // Get MainInventoryWidget
    UMainInventoryWidget* MainInv = GetMainInventoryWidget();
    if (!MainInv)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find MainInventoryWidget for weight update - widget tree logged above"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Successfully found MainInventoryWidget: %s"), *MainInv->GetName());
    }

    // Handle item destruction
    if (bIsInDragOperation)
    {
        int32 NewQuantity = ItemQuantity;
        if (NewQuantity > 0)
        {
            SetItemDetails(CurrentItemInfo, NewQuantity);
        }
        else
        {
            ClearSlot();
        }
    }
    else
    {
        int32 NewQuantity = ItemQuantity - DraggedQuantity;
        if (NewQuantity > 0)
        {
            SetItemDetails(CurrentItemInfo, NewQuantity);
        }
        else
        {
            ClearSlot();
        }
    }

    // Update weight if we found MainInv
    if (MainInv)
    {
        UE_LOG(LogTemp, Warning, TEXT("Updating inventory weight after destroying items"));
        MainInv->UpdateInventoryWeight();
    }

    // Clean up drag visual
    if (UDragDropOperation* DragDropOp = UWidgetBlueprintLibrary::GetDragDroppingContent())
    {
        DragDropOp->DefaultDragVisual = nullptr;
    }

    bIsInDragOperation = false;
}

void UInventorySlotWidget::OnItemDestroyCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("OnItemDestroyCancelled"));
    
    if (bIsInDragOperation)
    {
        // Return the dragged items to the slot
        if (ItemQuantity > 0)
        {
            SetItemDetails(CurrentItemInfo, ItemQuantity + DraggedQuantity);
        }
        else
        {
            SetItemDetails(DraggedItemInfo, DraggedQuantity);
        }
    }

    // Clean up any remaining drag visual
    if (UDragDropOperation* DragDropOp = UWidgetBlueprintLibrary::GetDragDroppingContent())
    {
        DragDropOp->DefaultDragVisual = nullptr;
    }
    
    bIsInDragOperation = false;
}

void UInventorySlotWidget::OpenBag()
{
    if (CurrentItemInfo.ItemType != EItemType::Bag)
        return;

    APawn* OwningPawn = GetOwningPlayerPawn();
    if (!OwningPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get owning pawn"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Looking for BagComponent for bag: %s"), *CurrentItemInfo.ItemID.ToString());

    // First try to find an existing BagComponent for this bag
    TArray<UBagComponent*> BagComps;
    OwningPawn->GetComponents<UBagComponent>(BagComps);
    
    UBagComponent* BagComp = nullptr;
    for (UBagComponent* Comp : BagComps)
    {
        // Match by ItemID to find the right bag component
        if (Comp && Comp->GetBagInfo().ItemID == CurrentItemInfo.ItemID)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found existing BagComponent for %s"), *CurrentItemInfo.ItemID.ToString());
            BagComp = Comp;
            break;
        }
    }

    // If no existing component found for this bag, create a new one
    if (!BagComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Creating new BagComponent for %s"), *CurrentItemInfo.ItemID.ToString());
        BagComp = NewObject<UBagComponent>(OwningPawn, *FString::Printf(TEXT("BagComponent_%s"), *CurrentItemInfo.ItemID.ToString()));
        BagComp->RegisterComponent();
        BagComp->InitializeBag(CurrentItemInfo);
    }

    if (BagComp->IsBagOpen())
    {
        UE_LOG(LogTemp, Warning, TEXT("Bag is already open"));
        return;
    }

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
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load WBP_BagWidget class"));
        }
    }
}