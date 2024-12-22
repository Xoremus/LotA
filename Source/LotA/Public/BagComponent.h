#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S_ItemInfo.h"
#include "BagComponent.generated.h"

USTRUCT()
struct FBagSlotData
{
    GENERATED_BODY()

    UPROPERTY()
    FS_ItemInfo ItemInfo;

    UPROPERTY()
    int32 Quantity = 0;
};

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

    // Force close regardless of current state
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void ForceClose();

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

    // Get the bag's item info
    UFUNCTION(BlueprintPure, Category = "Bag")
    const FS_ItemInfo& GetBagInfo() const { return BagInfo; }

    // Set bag data
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void InitializeBag(const FS_ItemInfo& BagItemInfo);

    // Get number of bag slots
    UFUNCTION(BlueprintPure, Category = "Bag")
    int32 GetBagSlots() const { return BagInfo.BagSlots; }

    // Item management
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool AddItem(int32 SlotIndex, const FS_ItemInfo& Item, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool GetSlotContent(int32 SlotIndex, FS_ItemInfo& OutItem, int32& OutQuantity) const;

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool RemoveItem(int32 SlotIndex);

    // Events for bag state changes
    UPROPERTY(BlueprintAssignable, Category = "Bag")
    FOnBagOpened OnBagOpened;

    UPROPERTY(BlueprintAssignable, Category = "Bag")
    FOnBagClosed OnBagClosed;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    // Whether the bag is currently open
    UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
    bool bIsOpen;

    // Bag item information
    UPROPERTY(Replicated)
    FS_ItemInfo BagInfo;

    // Array to store bag contents
    UPROPERTY()
    TArray<FBagSlotData> BagContents;

    // Handle replication of open state
    UFUNCTION()
    void OnRep_IsOpen();
};