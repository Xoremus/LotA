#pragma once

#include "CoreMinimal.h"
#include "DraggableWindowBase.h"
#include "S_ItemInfo.h"
#include "Components/UniformGridPanel.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "BagComponent.h"
#include "BagWidget.generated.h"

UCLASS()
class LOTA_API UBagWidget : public UDraggableWindowBase
{
	GENERATED_BODY()

public:
	UBagWidget(const FObjectInitializer& ObjectInitializer);

	void InitializeBag(const FS_ItemInfo& BagInfo);
	void SetOwningBagComponent(UBagComponent* BagComp);

	UBagComponent* GetOwningBagComponent() const { return OwningBagComponent; }
	const TArray<class UInventorySlotWidget*>& GetBagSlots() const { return BagSlots; }

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
	TArray<class UInventorySlotWidget*> BagSlots;

	UFUNCTION()
	void OnCloseButtonClicked();

private:
	UPROPERTY()
	FS_ItemInfo BagItemInfo;

	UPROPERTY()
	UBagComponent* OwningBagComponent;

	void CreateBagSlots();
};