#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S_ItemInfo.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class LOTA_API UInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventorySlotWidget(const FObjectInitializer& ObjectInitializer);

    // Set item details for the slot
    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity);

    // Clear the slot
    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void ClearSlot();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
    UPROPERTY(meta = (BindWidget))
    UImage* ItemIcon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* QuantityText;

    UPROPERTY()
    FS_ItemInfo CurrentItemInfo;

    UPROPERTY()
    int32 ItemQuantity;

    // Drag operation data
    bool bIsInDragOperation;
    FS_ItemInfo DraggedItemInfo;
    int32 DraggedQuantity;

    void UpdateVisuals();
};