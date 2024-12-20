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
        DraggedItemInfo = CurrentItemInfo;
        
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
        DragDropOp->DraggedItem = DraggedItemInfo;
        DragDropOp->OriginalQuantity = DraggedQuantity;
        DragDropOp->SourceSlot = this;
        DragDropOp->bSplitStack = InMouseEvent.IsShiftDown() || InMouseEvent.IsControlDown();

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

    if (InventoryDragDrop->SourceSlot == this)
        return false;

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
            
            // Update weight after stack change
            if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(GetParent()->GetParent()))
            {
                MainInv->UpdateInventoryWeight();
            }
            return true;
        }
    }

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

    // Update weight after item swap
    if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(GetParent()->GetParent()))
    {
        MainInv->UpdateInventoryWeight();
    }
    return true;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UE_LOG(LogTemp, Warning, TEXT("NativeOnDragCancelled called"));

    UInventoryDragDropOperation* InventoryDragDrop = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryDragDrop)
    {
        bIsInDragOperation = false;
        return;
    }

    FVector2D MousePos = InDragDropEvent.GetScreenSpacePosition();
    UE_LOG(LogTemp, Warning, TEXT("Mouse Position: %s"), *MousePos.ToString());
    
    // Get player controller once
    APlayerController* PC = GetOwningPlayer();
    if (!PC) 
    {
        bIsInDragOperation = false;
        return;
    }

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
            // Show destroy confirmation using the PC we already got
            const FSoftClassPath DestroyWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_Destroy.WBP_Destroy_C"));
            TSubclassOf<UDestroyConfirmationWidget> DestroyWidgetClass = DestroyWidgetPath.TryLoadClass<UDestroyConfirmationWidget>();

            if (DestroyWidgetClass)
            {
                UDestroyConfirmationWidget* DestroyWidget = CreateWidget<UDestroyConfirmationWidget>(PC, DestroyWidgetClass);
                if (DestroyWidget)
                {
                    DestroyWidget->SetItemToDestroy(DraggedItemInfo, DraggedQuantity);
                    DestroyWidget->OnDestroyConfirmed.AddDynamic(this, &UInventorySlotWidget::OnItemDestroyConfirmed);
                    DestroyWidget->AddToViewport();
                    DestroyWidget->SetPositionInViewport(MousePos);
                }
            }
        }
        else
        {
            // Return item only if dropped inside inventory
            if (InventoryDragDrop->SourceSlot == this)
            {
                SetItemDetails(InventoryDragDrop->DraggedItem, InventoryDragDrop->OriginalQuantity);
            }
        }
    }

    bIsInDragOperation = false;
}

void UInventorySlotWidget::OnItemDestroyConfirmed(const FS_ItemInfo& DestroyedItem)
{
    UE_LOG(LogTemp, Warning, TEXT("Item Destroy Confirmed: %s"), *DestroyedItem.ItemName.ToString());
    
    // Only destroy the dragged amount
    int32 RemainingQuantity = ItemQuantity - DraggedQuantity;
    
    if (RemainingQuantity > 0)
    {
        SetItemDetails(CurrentItemInfo, RemainingQuantity);
    }
    else
    {
        ClearSlot();
    }

    // Update weight by going through parent widgets properly
    if (UPanelWidget* ParentPanel = GetParent())
    {
        if (UUserWidget* ParentWidget = Cast<UUserWidget>(ParentPanel->GetParent()))
        {
            if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(ParentWidget))
            {
                MainInv->UpdateInventoryWeight();
            }
        }
    }
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

    UE_LOG(LogTemp, Warning, TEXT("Looking for BagComponent on pawn: %s"), *OwningPawn->GetName());

    UBagComponent* BagComp = OwningPawn->FindComponentByClass<UBagComponent>();
    if (!BagComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Creating new BagComponent"));
        BagComp = NewObject<UBagComponent>(OwningPawn, TEXT("BagComponent"));
        BagComp->RegisterComponent();
    }

    if (BagComp->IsBagOpen())
    {
        UE_LOG(LogTemp, Warning, TEXT("Bag is already open"));
        return;
    }

    BagComp->InitializeBag(CurrentItemInfo);

    if (BagComp->OpenBag())
    {
        // Use one PlayerController variable
        APlayerController* PC = GetOwningPlayer();
        if (!PC) return;

        // Create one BagWidget class path and class
        const FSoftClassPath BagWidgetPath(TEXT("/Game/Inventory/Widgets/WBP_BagWidget.WBP_BagWidget_C"));
        TSubclassOf<UBagWidget> BagWidgetClass = BagWidgetPath.TryLoadClass<UBagWidget>();

        if (BagWidgetClass)
        {
            // Use the variables we already declared
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