#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "S_ItemInfo.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    General UMETA(DisplayName = "General"),
    Equipment UMETA(DisplayName = "Equipment"),
    Consumable UMETA(DisplayName = "Consumable"),
    Bag UMETA(DisplayName = "Bag")
};

USTRUCT(BlueprintType)
struct LOTA_API FS_ItemInfo
{
    GENERATED_BODY()

public:
    // Item ID for identification
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    FName ItemID;

    // Item name (displayed to the player)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    FText ItemName;

    // Item icon for UI
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    UTexture2D* ItemIcon;

    // Item description
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    FText ItemDescription;

    // Item type (General, Equipment, Bag, etc.)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    EItemType ItemType;

    // Weight of the item
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    float Weight;

    // Maximum stack size for the item
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info")
    int32 MaxStackSize;

    // Bag-specific properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info", meta = (EditCondition = "ItemType == EItemType::Bag"))
    int32 BagSlots;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Info", meta = (EditCondition = "ItemType == EItemType::Bag"))
    float WeightReductionPercentage;

    // Default constructor
    FS_ItemInfo()
        : ItemID(NAME_None)
        , ItemName(FText::GetEmpty())
        , ItemIcon(nullptr)
        , ItemDescription(FText::GetEmpty())
        , ItemType(EItemType::General)
        , Weight(0.0f)
        , MaxStackSize(1)
        , BagSlots(0)
        , WeightReductionPercentage(0.0f)
    {}
};