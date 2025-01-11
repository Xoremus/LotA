#include "BagComponent.h"
#include "LotA/LotACharacter.h"
#include "Net/UnrealNetwork.h"

UBagComponent::UBagComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    bIsOpen = false;
}

void UBagComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UBagComponent, BagState);
    DOREPLIFETIME(UBagComponent, bIsOpen);
}

void UBagComponent::BeginPlay()
{
    Super::BeginPlay();
    LogOperation(TEXT("BeginPlay"), TEXT("Component initialized"));
    BagState.LogState(TEXT("Initial State"));
}

bool UBagComponent::OpenBag()
{
    LogOperation(TEXT("OpenBag"), TEXT("Attempting to open bag"));

    if (!BagState.BagInfo.ItemID.IsValid())
    {
        LogOperation(TEXT("OpenBag"), TEXT("Failed - Invalid bag ID"));
        return false;
    }

    BagState.LogState(TEXT("Before Opening"));
    bIsOpen = true;
    OnBagOpened.Broadcast(this);
    BagState.LogState(TEXT("After Opening"));
    return true;
}

void UBagComponent::CloseBag()
{
    LogOperation(TEXT("CloseBag"), TEXT("Closing bag"));
    BagState.LogState(TEXT("Before Close"));

    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        FBagState SavedState;
        SaveState(SavedState);
        Character->SaveBagState(this);
    }

    bIsOpen = false;
    OnBagClosed.Broadcast(this);
}

void UBagComponent::ForceClose()
{
    LogOperation(TEXT("ForceClose"), TEXT("Force closing bag"));
    BagState.LogState(TEXT("Before Force Close"));

    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->SaveBagState(this);
    }

    bIsOpen = false;
    OnBagClosed.Broadcast(this);

    BagState.LogState(TEXT("After Force Close"));
}

void UBagComponent::InitializeBag(const FS_ItemInfo& BagItemInfo)
{
    LogOperation(TEXT("InitializeBag"), FString::Printf(TEXT("Initializing bag: %s"), *BagItemInfo.ItemName.ToString()));

    if (!BagItemInfo.ItemID.IsValid())
    {
        LogOperation(TEXT("InitializeBag"), TEXT("Failed - Invalid bag info"));
        return;
    }

    // Set the bag info
    BagState.BagInfo = BagItemInfo;
    
    // Set the bag key - this was missing before
    BagState.BagKey = *FString::Printf(TEXT("Bag_%s"), *BagItemInfo.ItemID.ToString());

    // Initialize slots if needed
    if (BagState.SlotStates.Num() == 0)
    {
        BagState.SlotStates.SetNum(BagItemInfo.BagSlots);
        LogOperation(TEXT("InitializeBag"), FString::Printf(TEXT("Created %d slots"), BagItemInfo.BagSlots));
    }

    BagState.LogState(TEXT("After Initialize"));
}

bool UBagComponent::CanAcceptItem(const FS_ItemInfo& Item, int32 TargetSlot) const
{
    LogOperation(TEXT("CanAcceptItem"), 
        FString::Printf(TEXT("Checking if can accept %s in slot %d"), *Item.ItemName.ToString(), TargetSlot));

    // Validate slot index
    if (!BagState.SlotStates.IsValidIndex(TargetSlot))
    {
        LogOperation(TEXT("CanAcceptItem"), TEXT("Failed - Invalid slot index"));
        return false;
    }

    // Prevent any bags from being placed in bags
    if (Item.ItemType == EItemType::Bag)
    {
        LogOperation(TEXT("CanAcceptItem"), TEXT("Failed - Cannot place bags in bags"));
        return false;
    }

    LogOperation(TEXT("CanAcceptItem"), TEXT("Success - Can accept item"));
    return true;
}

bool UBagComponent::TryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot)
{
    LogOperation(TEXT("TryAddItem"), FString::Printf(TEXT("Adding %s (x%d) to slot %d"), 
        *Item.ItemName.ToString(), Quantity, TargetSlot));
    
    if (!CanAcceptItem(Item, TargetSlot))
        return false;

    // Update slot state
    BagState.SlotStates[TargetSlot].ItemInfo = Item;
    BagState.SlotStates[TargetSlot].Quantity = Quantity;

    // Log the slot contents
    BagState.LogState(TEXT("After Adding Item"));

    // Save and notify
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->SaveBagState(this);
    }
    
    NotifySlotUpdated(TargetSlot);
    UpdateWeight();
    return true;
}

bool UBagComponent::TryRemoveItem(int32 SlotIndex)
{
    LogOperation(TEXT("TryRemoveItem"), FString::Printf(TEXT("Removing item from slot %d"), SlotIndex));
    BagState.LogState(TEXT("Before Remove Item"));

    if (!BagState.SlotStates.IsValidIndex(SlotIndex))
    {
        LogOperation(TEXT("TryRemoveItem"), TEXT("Failed - Invalid slot index"));
        return false;
    }

    BagState.SlotStates[SlotIndex].Clear();
    
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->SaveBagState(this);
    }

    NotifySlotUpdated(SlotIndex);
    BagState.LogState(TEXT("After Remove Item"));
    return true;
}

bool UBagComponent::TryMoveItem(int32 FromSlot, int32 ToSlot)
{
    LogOperation(TEXT("TryMoveItem"), 
        FString::Printf(TEXT("Moving item from slot %d to slot %d"), FromSlot, ToSlot));

    BagState.LogState(TEXT("Before Move Item"));

    if (!BagState.SlotStates.IsValidIndex(FromSlot) || 
        !BagState.SlotStates.IsValidIndex(ToSlot) || 
        FromSlot == ToSlot)
    {
        LogOperation(TEXT("TryMoveItem"), TEXT("Failed - Invalid slots"));
        return false;
    }

    FBagSlotState Temp = BagState.SlotStates[FromSlot];
    BagState.SlotStates[FromSlot] = BagState.SlotStates[ToSlot];
    BagState.SlotStates[ToSlot] = Temp;

    NotifySlotUpdated(FromSlot);
    NotifySlotUpdated(ToSlot);
    BagState.LogState(TEXT("After Move Item"));
    return true;
}

bool UBagComponent::GetSlotContent(int32 SlotIndex, FS_ItemInfo& OutItem, int32& OutQuantity) const
{
    if (!BagState.SlotStates.IsValidIndex(SlotIndex))
        return false;

    OutItem = BagState.SlotStates[SlotIndex].ItemInfo;
    OutQuantity = BagState.SlotStates[SlotIndex].Quantity;
    return true;
}

int32 UBagComponent::GetFirstEmptySlot() const
{
    for (int32 i = 0; i < BagState.SlotStates.Num(); i++)
    {
        if (BagState.SlotStates[i].IsEmpty())
            return i;
    }
    return INDEX_NONE;
}

void UBagComponent::SetSlotStates(const TArray<FBagSlotState>& NewStates)
{
    LogOperation(TEXT("SetSlotStates"), FString::Printf(TEXT("Setting %d states"), NewStates.Num()));
    BagState.LogState(TEXT("Before Set"));

    if (NewStates.Num() == 0)
    {
        LogOperation(TEXT("SetSlotStates"), TEXT("No states to set"));
        return;
    }

    BagState.SlotStates = NewStates;

    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        if (!BagState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Set slot %d: %s (x%d)"), 
                i, 
                *BagState.SlotStates[i].ItemInfo.ItemName.ToString(),
                BagState.SlotStates[i].Quantity);
        }
        NotifySlotUpdated(i);
    }

    BagState.LogState(TEXT("After Set"));
}

void UBagComponent::NotifySlotUpdated(int32 SlotIndex)
{
    if (!BagState.SlotStates.IsValidIndex(SlotIndex))
        return;

    const FBagSlotState& State = BagState.SlotStates[SlotIndex];
    if (!State.IsEmpty())
    {
        LogOperation(TEXT("NotifySlotUpdated"), 
            FString::Printf(TEXT("Slot %d: %s (x%d)"), 
                SlotIndex, 
                *State.ItemInfo.ItemName.ToString(),
                State.Quantity));
    }
    OnSlotUpdated.Broadcast(SlotIndex, State.ItemInfo, State.Quantity);
}

void UBagComponent::OnRep_IsOpen()
{
    LogOperation(TEXT("OnRep_IsOpen"), bIsOpen ? TEXT("Bag opened") : TEXT("Bag closed"));
    
    if (bIsOpen)
    {
        OnBagOpened.Broadcast(this);
    }
    else
    {
        OnBagClosed.Broadcast(this);
    }
}

void UBagComponent::LogOperation(const FString& Operation, const FString& Details) const
{
    const FString BagName = BagState.BagInfo.ItemName.ToString();
    const FString BagID = BagState.BagInfo.ItemID.ToString();
    
    UE_LOG(LogTemp, Warning, TEXT("=== Bag Operation [%s] ==="), *Operation);
    UE_LOG(LogTemp, Warning, TEXT("Bag: %s (ID: %s)"), *BagName, *BagID);
    UE_LOG(LogTemp, Warning, TEXT("IsOpen: %s"), bIsOpen ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Warning, TEXT("Details: %s"), *Details);
    UE_LOG(LogTemp, Warning, TEXT("==================="));
}

void UBagComponent::LoadState(const FBagState& State)
{
    BagState = State;
    
    // Log loaded contents 
    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        if (!BagState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Loaded slot %d: %s (x%d)"), 
                i,
                *BagState.SlotStates[i].ItemInfo.ItemName.ToString(),
                BagState.SlotStates[i].Quantity);
        }
    }

    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        NotifySlotUpdated(i);
    }
}

void UBagComponent::SaveState(FBagState& OutState) const
{
    LogOperation(TEXT("SaveState"), TEXT("Saving bag state"));
    BagState.LogState(TEXT("Before Save"));

    OutState = BagState;
    OutState.BagKey = *FString::Printf(TEXT("Bag_%s"), *BagState.BagInfo.ItemID.ToString());

    LogOperation(TEXT("SaveState"), TEXT("State saved"));
    OutState.LogState(TEXT("Saved State"));
}

void UBagComponent::UpdateWeight()
{
    LogOperation(TEXT("UpdateWeight"), TEXT("Updating bag weight"));

    float OldWeight = BagState.BagInfo.Weight;
    float NewWeight = 0.0f;

    // Calculate new total weight
    for (const FBagSlotState& Slot : BagState.SlotStates)
    {
        if (!Slot.IsEmpty())
        {
            NewWeight += Slot.ItemInfo.Weight * Slot.Quantity;
        }
    }

    // Add bag's own weight
    NewWeight += BagState.BagInfo.Weight;

    if (!FMath::IsNearlyEqual(OldWeight, NewWeight))
    {
        LogOperation(TEXT("UpdateWeight"), 
            FString::Printf(TEXT("Weight changed from %.2f to %.2f"), OldWeight, NewWeight));
        OnWeightChanged.Broadcast(NewWeight);
    }
}