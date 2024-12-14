// BagWidget.h
#pragma once

#include "CoreMinimal.h"
#include "DraggableWindowBase.h"
#include "S_ItemInfo.h"
#include "BagWidget.generated.h"

class UUniformGridPanel;
class UInventorySlotWidget;
class UBagComponent;

UCLASS()
class LOTA_API UBagWidget : public UDraggableWindowBase
{
	GENERATED_BODY()

public:
	UBagWidget(const FObjectInitializer& ObjectInitializer);

	void InitializeBag(const FS_ItemInfo& BagInfo);
	void SetOwningBagComponent(UBagComponent* BagComp);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* InventoryGrid;

	UPROPERTY()
	TArray<UInventorySlotWidget*> BagSlots;

private:
	UPROPERTY()
	FS_ItemInfo BagItemInfo;

	UPROPERTY()
	UBagComponent* OwningBagComponent;

	void CreateBagSlots();
};