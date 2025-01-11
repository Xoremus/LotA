#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S_ItemInfo.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InventorySlotWidget.generated.h"

class UBagComponent;
class UMainInventoryWidget;

UCLASS()
class LOTA_API UInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventorySlotWidget(const FObjectInitializer& ObjectInitializer);

    // Sets the current item info + quantity
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetItemDetails(const FS_ItemInfo& InItemInfo, int32 Quantity);

    // Clears the slot (icon, quantity, etc.)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ClearSlot();

    // Gets the current quantity in this slot
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetQuantity() const;

    // Returns the item info for this slot
    UFUNCTION(BlueprintPure, Category = "Inventory")
    const FS_ItemInfo& GetItemInfo() const;

    // If this slot is in a BagWidget, get the associated bag component
    UFUNCTION(BlueprintPure, Category = "Inventory")
    UBagComponent* GetParentBagComponent() const;

    // Gets the index of this slot in the parent bag (if it exists)
    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool TryGetParentBagSlotIndex(int32& OutSlotIndex) const;

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    UPROPERTY(meta = (BindWidget))
    UImage* ItemIcon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* QuantityText;

private:
    void UpdateVisuals();
    void OpenBag();
    UMainInventoryWidget* GetMainInventoryWidget() const;
    bool FindAndRestoreToAvailableSlot();
    UInventorySlotWidget* FindFirstAvailableSlot();

    UFUNCTION()
    void OnItemDestroyConfirmed(const FS_ItemInfo& DestroyedItem);

    UFUNCTION()
    void OnItemDestroyCancelled();

    UPROPERTY()
    FS_ItemInfo CurrentItemInfo;

    UPROPERTY()
    int32 ItemQuantity;

    bool bIsInDragOperation;
    FS_ItemInfo DraggedItemInfo;
    int32 DraggedQuantity;
};
