#pragma once

#include "CoreMinimal.h"
#include "DraggableWindowBase.h"
#include "BagWidget.generated.h"

class UUniformGridPanel;  // Changed from UInventoryWidget

UCLASS()
class LOTA_API UBagWidget : public UDraggableWindowBase
{
	GENERATED_BODY()

public:
	UBagWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Bag")
	void InitializeBag(int32 Rows, int32 Columns);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* InventoryGrid;  // Changed to match your blueprint
};