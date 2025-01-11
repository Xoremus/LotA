#pragma once

#include "CoreMinimal.h"
#include "S_ItemInfo.h"
#include "BagTypes.generated.h"

// Individual slot state within a bag
USTRUCT(BlueprintType)
struct FBagSlotState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FS_ItemInfo ItemInfo;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    int32 Quantity = 0;

    bool IsEmpty() const { return Quantity <= 0; }
    void Clear() 
    { 
        UE_LOG(LogTemp, Warning, TEXT("Clearing slot that contained: %s (x%d)"), 
            *ItemInfo.ItemName.ToString(), Quantity);
            
        ItemInfo = FS_ItemInfo(); 
        Quantity = 0; 
    }

    // Debug function
    FString ToString() const
    {
        if (IsEmpty())
            return TEXT("Empty");
        return FString::Printf(TEXT("%s (x%d)"), *ItemInfo.ItemName.ToString(), Quantity);
    }
};

// Full state of a bag
USTRUCT(BlueprintType)
struct FBagState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FName BagKey;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FS_ItemInfo BagInfo;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    TArray<FBagSlotState> SlotStates;

    // Debug function
    void LogState(const FString& Context) const
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Bag State [%s] ==="), *Context);
        UE_LOG(LogTemp, Warning, TEXT("Bag Name: %s"), *BagInfo.ItemName.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Bag ID: %s"), *BagInfo.ItemID.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Bag Key: %s"), *BagKey.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Number of Slots: %d"), SlotStates.Num());
        
        for (int32 i = 0; i < SlotStates.Num(); ++i)
        {
            if (!SlotStates[i].IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s"), i, *SlotStates[i].ToString());
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("==================="));
    }
};

// Legacy structure for backward compatibility
USTRUCT(BlueprintType)
struct FBagSavedState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    TArray<FBagSlotState> SlotStates;

    UPROPERTY(BlueprintReadWrite, Category = "Inventory")
    FS_ItemInfo BagInfo;

    // Debug function
    void LogState(const FString& Context) const
    {
        UE_LOG(LogTemp, Warning, TEXT("=== BagSavedState [%s] ==="), *Context);
        UE_LOG(LogTemp, Warning, TEXT("Bag Name: %s"), *BagInfo.ItemName.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Bag ID: %s"), *BagInfo.ItemID.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Number of Slots: %d"), SlotStates.Num());
        
        for (int32 i = 0; i < SlotStates.Num(); ++i)
        {
            if (!SlotStates[i].IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s"), i, *SlotStates[i].ToString());
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("==================="));
    }
};

// Container for all saved bag states
USTRUCT(BlueprintType)
struct FBagSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FBagState> SavedBags;

    float GetTotalTopLevelWeight() const
    {
        float Total = 0.0f;
        for (const auto& BagState : SavedBags)
        {
            Total += BagState.BagInfo.Weight;
            
            // Add weight of contents
            for (const auto& SlotState : BagState.SlotStates)
            {
                if (!SlotState.IsEmpty())
                {
                    Total += SlotState.ItemInfo.Weight * SlotState.Quantity;
                }
            }
        }
        return Total;
    }
    
    TArray<FName> GetBagsAtDepth(int32 Depth) const
    {
        TArray<FName> Result;
        for (const auto& BagState : SavedBags)
        {
            if (BagState.BagKey.IsValid())
            {
                Result.Add(BagState.BagKey);
            }
        }
        return Result;
    }

    void LogAllBags(const FString& Context) const
    {
        UE_LOG(LogTemp, Warning, TEXT("=== All Saved Bags [%s] ==="), *Context);
        UE_LOG(LogTemp, Warning, TEXT("Number of Saved Bags: %d"), SavedBags.Num());
        
        for (const auto& BagState : SavedBags)
        {
            BagState.LogState(TEXT("Saved Bag"));
        }
        UE_LOG(LogTemp, Warning, TEXT("==================="));
    }
};