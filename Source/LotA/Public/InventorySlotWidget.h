// InventorySlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S_ItemInfo.h"
#include "InventorySlotWidget.generated.h"

// Forward declare instead of including
class UBagWidget;
class UImage;
class UTextBlock;

UCLASS()
class LOTA_API UInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventorySlotWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void ClearSlot();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetQuantity() const { return ItemQuantity; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const FS_ItemInfo& GetItemInfo() const { return CurrentItemInfo; }

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

    bool bIsInDragOperation;
    FS_ItemInfo DraggedItemInfo;
    int32 DraggedQuantity;

    void UpdateVisuals();
    void OpenBag();
};