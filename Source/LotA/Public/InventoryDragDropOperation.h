// InventoryDragDropOperation.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "S_ItemInfo.h"
#include "InventorySlotWidget.h"
#include "InventoryDragDropOperation.generated.h"

UCLASS()
class LOTA_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// The source inventory slot this drag operation started from
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	UInventorySlotWidget* SourceSlot;

	// The item and quantity being dragged
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	FS_ItemInfo DraggedItem;

	// Whether this is a split operation (shift or ctrl drag)
	UPROPERTY()
	bool bSplitStack;

	// The original quantity in the source slot before the drag started
	UPROPERTY()
	int32 OriginalQuantity;
};