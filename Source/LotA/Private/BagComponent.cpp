#include "BagComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"

UBagComponent::UBagComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bIsOpen = false;
    SetIsReplicatedByDefault(true);
}

void UBagComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UBagComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBagComponent, bIsOpen);
    DOREPLIFETIME(UBagComponent, BagInfo);
}

bool UBagComponent::OpenBag()
{
    if (bIsOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Cannot open bag - already open. Forcing close first."));
        ForceClose();
    }

    if (!BagInfo.ItemID.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Cannot open bag - invalid bag info"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Opening bag: %s"), *BagInfo.ItemID.ToString());
    bIsOpen = true;
    OnBagOpened.Broadcast(this);
    return true;
}

void UBagComponent::CloseBag()
{
    if (!bIsOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Bag already closed"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Closing bag: %s"), *BagInfo.ItemID.ToString());
    bIsOpen = false;
    OnBagClosed.Broadcast(this);
}

void UBagComponent::ForceClose()
{
    UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Force closing bag: %s"), *BagInfo.ItemID.ToString());
    bIsOpen = false;
    OnBagClosed.Broadcast(this);
}

bool UBagComponent::HasItems() const
{
    for (const FBagSlotData& SlotData : BagContents)
    {
        if (SlotData.Quantity > 0)
        {
            return true;
        }
    }
    return false;
}

float UBagComponent::GetTotalWeight() const
{
    float TotalWeight = BagInfo.Weight;

    // Add weight of contents
    for (const FBagSlotData& SlotData : BagContents)
    {
        if (SlotData.Quantity > 0)
        {
            TotalWeight += SlotData.ItemInfo.Weight * SlotData.Quantity;
        }
    }

    // Apply weight reduction if any
    if (BagInfo.WeightReductionPercentage > 0.0f)
    {
        float ContentsWeight = TotalWeight - BagInfo.Weight;
        float ReducedContentsWeight = ContentsWeight * (1.0f - (BagInfo.WeightReductionPercentage / 100.0f));
        TotalWeight = BagInfo.Weight + ReducedContentsWeight;
    }

    return TotalWeight;
}

void UBagComponent::InitializeBag(const FS_ItemInfo& BagItemInfo)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        BagInfo = BagItemInfo;
        
        // Initialize or resize the bag contents array
        if (BagContents.Num() != BagInfo.BagSlots)
        {
            UE_LOG(LogTemp, Warning, TEXT("Initializing bag with %d slots"), BagInfo.BagSlots);
            BagContents.SetNum(BagInfo.BagSlots);
        }
    }
}

bool UBagComponent::AddItem(int32 SlotIndex, const FS_ItemInfo& Item, int32 Quantity)
{
    if (SlotIndex < 0 || SlotIndex >= BagContents.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("AddItem: Invalid slot index %d (Max: %d)"), SlotIndex, BagContents.Num());
        return false;
    }

    // Store the item data
    BagContents[SlotIndex].ItemInfo = Item;
    BagContents[SlotIndex].Quantity = Quantity;
    
    UE_LOG(LogTemp, Warning, TEXT("Added item to bag slot %d: %s (Quantity: %d)"), 
        SlotIndex, *Item.ItemName.ToString(), Quantity);
    
    return true;
}

bool UBagComponent::GetSlotContent(int32 SlotIndex, FS_ItemInfo& OutItem, int32& OutQuantity) const
{
    if (SlotIndex < 0 || SlotIndex >= BagContents.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("GetSlotContent: Invalid slot index %d (Max: %d)"), SlotIndex, BagContents.Num());
        return false;
    }

    OutItem = BagContents[SlotIndex].ItemInfo;
    OutQuantity = BagContents[SlotIndex].Quantity;
    
    if (OutQuantity > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Retrieved item from bag slot %d: %s (Quantity: %d)"), 
            SlotIndex, *OutItem.ItemName.ToString(), OutQuantity);
    }
    
    return true;
}

bool UBagComponent::RemoveItem(int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= BagContents.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("RemoveItem: Invalid slot index %d (Max: %d)"), SlotIndex, BagContents.Num());
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("Removing item from bag slot %d"), SlotIndex);
    
    BagContents[SlotIndex].ItemInfo = FS_ItemInfo();
    BagContents[SlotIndex].Quantity = 0;
    
    return true;
}

void UBagComponent::OnRep_IsOpen()
{
    UE_LOG(LogTemp, Warning, TEXT("[BagComponent] Bag state changed - IsOpen: %s"), bIsOpen ? TEXT("true") : TEXT("false"));
    if (bIsOpen)
    {
        OnBagOpened.Broadcast(this);
    }
    else
    {
        OnBagClosed.Broadcast(this);
    }
}