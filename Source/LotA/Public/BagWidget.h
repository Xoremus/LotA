// BagWidget.h
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
	void SetOwningBagComponent(class UBagComponent* BagComp);  // Add class keyword here

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
	TArray<class UInventorySlotWidget*> BagSlots;  // Add class keyword here

	UFUNCTION()
	void OnCloseButtonClicked();

private:
	UPROPERTY()
	FS_ItemInfo BagItemInfo;

	UPROPERTY()
	class UBagComponent* OwningBagComponent;  // Add class keyword here

	void CreateBagSlots();
};