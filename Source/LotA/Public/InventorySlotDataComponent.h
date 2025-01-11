#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S_ItemInfo.h"
#include "InventorySlotDataComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UInventorySlotDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventorySlotDataComponent();

	// Store item data
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FS_ItemInfo ItemData;

	// Current stack count for the item
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 StackCount;

	// Check if the slot is empty
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsEmpty() const;

	// Add items to the slot
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool AddItems(const FS_ItemInfo& NewItem, int32 Count);

	// Remove items from the slot
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool RemoveItems(int32 Count);

protected:
	virtual void BeginPlay() override;
};
