// BagComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S_ItemInfo.h"
#include "InventorySlotDataComponent.h"
#include "BagComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UBagComponent*, Bag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UBagComponent*, Bag);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UBagComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UBagComponent();

    // Open/Close bag functionality
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool OpenBag();
    
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void CloseBag();

    // Check if bag is currently open
    UFUNCTION(BlueprintPure, Category = "Bag")
    bool IsBagOpen() const { return bIsOpen; }

    // Check if bag has items
    UFUNCTION(BlueprintPure, Category = "Bag")
    bool HasItems() const;

    // Calculate total weight including contents
    UFUNCTION(BlueprintPure, Category = "Bag")
    float GetTotalWeight() const;

    // Get the weight reduction percentage
    UFUNCTION(BlueprintPure, Category = "Bag")
    float GetWeightReduction() const { return BagInfo.WeightReductionPercentage; }

    // Set bag data
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void InitializeBag(const FS_ItemInfo& BagItemInfo);

    // Get number of bag slots
    UFUNCTION(BlueprintPure, Category = "Bag")
    int32 GetBagSlots() const { return BagInfo.BagSlots; }

    // Events for bag state changes
    UPROPERTY(BlueprintAssignable, Category = "Bag")
    FOnBagOpened OnBagOpened;

    UPROPERTY(BlueprintAssignable, Category = "Bag")
    FOnBagClosed OnBagClosed;

    // Get bag inventory slots
    UFUNCTION(BlueprintPure, Category = "Bag")
    const TArray<UInventorySlotDataComponent*>& GetInventorySlots() const { return InventorySlots; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // Whether the bag is currently open
    UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
    bool bIsOpen;

    // Bag item information
    UPROPERTY(Replicated)
    FS_ItemInfo BagInfo;

    // Array of inventory slots in the bag
    UPROPERTY()
    TArray<UInventorySlotDataComponent*> InventorySlots;

    // Reference to the UI widget
    UPROPERTY()
    class UWidget* BagWidget;

    // Handle replication of open state
    UFUNCTION()
    void OnRep_IsOpen();

    // Initialize inventory slots
    void CreateInventorySlots();
};