// InventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UUniformGridPanel;
class UInventorySlotWidget;

UCLASS()
class LOTA_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInventoryWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory(int32 Rows, int32 Columns);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddTestItem(int32 SlotIndex);

protected:
	virtual void NativeConstruct() override;

	// Grid panel to hold slots
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* InventoryGrid;

	// Backend representation of the slots
	UPROPERTY()
	TArray<UInventorySlotWidget*> InventorySlots;

	// Number of rows and columns in the inventory
	int32 NumRows;
	int32 NumColumns;

private:
	void CreateInventorySlots();
};