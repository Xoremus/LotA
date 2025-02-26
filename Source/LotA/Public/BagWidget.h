// BagWidget.h
#pragma once

#include "CoreMinimal.h"
#include "DraggableWindowBase.h"  // Changed from LotA/DraggableWindowBase.h
#include "S_ItemInfo.h"           // Changed from LotA/S_ItemInfo.h
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "BagWidget.generated.h"

// Forward declarations
class UTextBlock;
class UButton;
class UUniformGridPanel;
class UInventorySlotWidget;
class UBagComponent;
struct FS_ItemInfo;

UCLASS()
class LOTA_API UBagWidget : public UDraggableWindowBase
{
	GENERATED_BODY()

public:
	UBagWidget(const FObjectInitializer& ObjectInitializer);

	void SetOwningBagComponent(UBagComponent* BagComp);
	void InitializeBag(const FS_ItemInfo& BagInfo);

	UFUNCTION(BlueprintPure, Category = "Bag")
	UBagComponent* GetOwningBagComponent() const { return OwningBagComponent; }

	UFUNCTION(BlueprintPure, Category = "Bag")
	const TArray<UInventorySlotWidget*>& GetBagSlots() const { return BagSlots; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WindowTitle;

	UPROPERTY(meta = (BindWidget))
	UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* InventoryGrid;

	UPROPERTY()
	TArray<UInventorySlotWidget*> BagSlots;

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	void OnBagSlotUpdated(int32 SlotIndex, const FS_ItemInfo& ItemInfo, int32 Quantity);

	UFUNCTION()
	void OnBagOperationFailed(const FText& FailureReason);

	UFUNCTION(BlueprintNativeEvent, Category = "Bag|Feedback")
	void OnOperationFailedBP(const FText& FailureReason);
	virtual void OnOperationFailedBP_Implementation(const FText& FailureReason);

private:
	void CreateBagSlots();
	bool ValidateSlotIndex(int32 SlotIndex) const;

	UPROPERTY()
	UBagComponent* OwningBagComponent;
};