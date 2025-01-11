// BagComponent.cpp (fixed version)
#include "BagComponent.h"
#include "LotA/LotACharacter.h"
#include "Net/UnrealNetwork.h"

UBagComponent::UBagComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    bIsOpen = false;
    bIsSaving = false;
    bIsClosing = false;
    bIsUpdatingWeight = false;
    bSuppressSave = false;
    bPendingWeightUpdate = false;
    LastCalculatedWeight = 0.0f;
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
}

bool UBagComponent::OpenBag()
{
    if (!BagState.BagInfo.ItemID.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("OpenBag: Invalid bag ID"));
        return false;
    }

    if (!bIsOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("Opening bag %s"), *BagState.BagKey.ToString());

        bIsOpen = true;

        // Notify all slots when opening to ensure visual sync
        for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
        {
            NotifySlotUpdated(i);
        }

        OnBagOpened.Broadcast(this);
    }

    return true;
}

void UBagComponent::CloseBag()
{
    if (bIsClosing)
        return;

    bIsClosing = true;
    SaveState();
    bIsOpen = false;
    OnBagClosed.Broadcast(this);
    bIsClosing = false;
}

void UBagComponent::ForceClose()
{
    if (bIsClosing)
        return;

    bIsClosing = true;
    SaveState();
    bIsOpen = false;
    OnBagClosed.Broadcast(this);
    bIsClosing = false;

    // Clean up component from owner
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->RemoveBagComponent(this);
    }
}

void UBagComponent::InitializeBag(const FS_ItemInfo& BagItemInfo)
{
    if (!BagItemInfo.ItemID.IsValid())
        return;

    BagState.BagInfo = BagItemInfo;
    BagState.BagKey = *FString::Printf(TEXT("Bag_%s"), *BagItemInfo.ItemID.ToString());

    if (BagState.SlotStates.Num() == 0)
    {
        BagState.SlotStates.SetNum(BagItemInfo.BagSlots);
    }

    SaveState();
}

bool UBagComponent::CanAcceptItem(const FS_ItemInfo& Item, int32 TargetSlot) const
{
    // Never accept bags in bags
    if (Item.ItemType == EItemType::Bag)
    {
        UE_LOG(LogTemp, Warning, TEXT("CanAcceptItem: Cannot put bags inside bags"));
        return false;
    }

    // Validate slot index
    if (!BagState.SlotStates.IsValidIndex(TargetSlot))
    {
        UE_LOG(LogTemp, Warning, TEXT("CanAcceptItem: Invalid slot index %d"), TargetSlot);
        return false;
    }

    return true;
}

bool UBagComponent::TryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot)
{
    if (!CanAcceptItem(Item, TargetSlot))
    {
        return false;
    }

    // Update slot state
    BagState.SlotStates[TargetSlot].ItemInfo = Item;
    BagState.SlotStates[TargetSlot].Quantity = Quantity;

    UE_LOG(LogTemp, Warning, TEXT("TryAddItem: Added %s (x%d) to slot %d"), 
        *Item.ItemName.ToString(), Quantity, TargetSlot);

    // Save state first, then notify
    SaveState();
    NotifySlotUpdated(TargetSlot);
    RequestWeightUpdate();

    return true;
}

bool UBagComponent::TryRemoveItem(int32 SlotIndex)
{
    if (!BagState.SlotStates.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("TryRemoveItem: Invalid slot index %d"), SlotIndex);
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("TryRemoveItem: Removing item from slot %d"), SlotIndex);
    BagState.SlotStates[SlotIndex].Clear();
    SaveState();
    NotifySlotUpdated(SlotIndex);
    RequestWeightUpdate();
    return true;
}

void UBagComponent::LoadState(const FBagState& State)
{
    UE_LOG(LogTemp, Warning, TEXT("LoadState: Loading state for bag %s with %d slots"), 
        *State.BagKey.ToString(), State.SlotStates.Num());

    BagState = State;

    // Log and notify for each slot
    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        if (!BagState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s (x%d)"), 
                i, 
                *BagState.SlotStates[i].ItemInfo.ItemName.ToString(),
                BagState.SlotStates[i].Quantity);
            NotifySlotUpdated(i);
        }
    }

    RequestWeightUpdate();
}

void UBagComponent::SaveState()
{
    // Skip save if suppressed or already saving
    if (bSuppressSave || bIsSaving || !BagState.BagKey.IsValid())
    {
        UE_LOG(LogTemp, Verbose, TEXT("SaveState: Skipped save for %s (Suppressed=%d, IsSaving=%d)"), 
            *BagState.BagKey.ToString(), bSuppressSave, bIsSaving);
        return;
    }

    bIsSaving = true;

    // Cancel any pending save
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SaveDebounceTimer);
    }

    UE_LOG(LogTemp, Warning, TEXT("SaveState: Saving state for bag %s with %d slots"), 
        *BagState.BagKey.ToString(), BagState.SlotStates.Num());

    // Log all non-empty slots
    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        if (!BagState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s (x%d)"), 
                i, 
                *BagState.SlotStates[i].ItemInfo.ItemName.ToString(),
                BagState.SlotStates[i].Quantity);
        }
    }

    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->SaveBagState(this);
    }

    // Set timer to allow next save
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SaveDebounceTimer, 
            [this]() { bIsSaving = false; }, 
            0.1f, false);
    }
    else
    {
        bIsSaving = false;
    }
}

void UBagComponent::RequestWeightUpdate()
{
    if (UWorld* World = GetWorld())
    {
        // Clear any pending update
        World->GetTimerManager().ClearTimer(WeightUpdateTimer);
        
        // Schedule a new update
        World->GetTimerManager().SetTimer(WeightUpdateTimer,
            [this]() { UpdateWeight(); },
            0.1f, false);
    }
}

void UBagComponent::UpdateWeight()
{
    // If already updating, mark as pending and return
    if (bIsUpdatingWeight)
    {
        bPendingWeightUpdate = true;
        return;
    }

    bIsUpdatingWeight = true;
    bSuppressSave = true;  // Prevent saves during weight calculation
    
    float NewWeight = BagState.BagInfo.Weight;
    for (const FBagSlotState& Slot : BagState.SlotStates)
    {
        if (!Slot.IsEmpty())
        {
            NewWeight += Slot.ItemInfo.Weight * Slot.Quantity;
        }
    }

    // Only broadcast if weight actually changed
    if (!FMath::IsNearlyEqual(LastCalculatedWeight, NewWeight))
    {
        LastCalculatedWeight = NewWeight;
        OnWeightChanged.Broadcast(NewWeight);
    }

    bSuppressSave = false;
    bIsUpdatingWeight = false;

    // If there's a pending update, handle it after a short delay
    if (bPendingWeightUpdate)
    {
        bPendingWeightUpdate = false;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(WeightUpdateTimer,
                [this]() { UpdateWeight(); },
                0.1f, false);
        }
    }
}

void UBagComponent::NotifySlotUpdated(int32 SlotIndex)
{
    if (BagState.SlotStates.IsValidIndex(SlotIndex))
    {
        const FBagSlotState& State = BagState.SlotStates[SlotIndex];
        OnSlotUpdated.Broadcast(SlotIndex, State.ItemInfo, State.Quantity);

        // Request a weight update any time slot contents change
        RequestWeightUpdate();
    }
}

void UBagComponent::OnRep_IsOpen()
{
    if (bIsOpen)
    {
        OnBagOpened.Broadcast(this);
    }
    else
    {
        OnBagClosed.Broadcast(this);
    }
}