// BagComponent.cpp
#include "BagComponent.h"
#include "LotA/LotACharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBagComponent::UBagComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    bIsOpen = false;
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

void UBagComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }
    CleanupTimers();
    Super::EndPlay(EndPlayReason);
}

bool UBagComponent::OpenBag()
{
    if (!BagState.BagInfo.ItemID.IsValid())
    {
        return false;
    }

    if (!bIsOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("Opening bag %s"), *BagState.BagKey.ToString());
        bIsOpen = true;

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
    if (bIsClosing || bPendingRemoval)
        return;

    bIsClosing = true;
    UE_LOG(LogTemp, Warning, TEXT("Closing bag %s"), *BagState.BagKey.ToString());

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }

    bIsOpen = false;
    OnBagClosed.Broadcast(this);
    bIsClosing = false;
}

void UBagComponent::ForceClose()
{
    if (bIsClosing || bPendingRemoval)
        return;

    bIsClosing = true;
    
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }
    
    bIsOpen = false;
    OnBagClosed.Broadcast(this);
    bIsClosing = false;

    bPendingRemoval = true;
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->RemoveBagComponent(this);
    }
}

void UBagComponent::InitializeBag(const FS_ItemInfo& BagItemInfo)
{
    if (!BagItemInfo.ItemID.IsValid() || !ValidateItemOperation(BagItemInfo))
        return;

    BagState.BagInfo = BagItemInfo;
    BagState.BagKey = *FString::Printf(TEXT("Bag_%s"), *BagItemInfo.ItemID.ToString());

    if (BagState.SlotStates.Num() == 0)
    {
        BagState.SlotStates.SetNum(BagItemInfo.BagSlots);
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }
}

bool UBagComponent::CanAcceptItem(const FS_ItemInfo& Item, int32 TargetSlot, FText& OutReason) const
{
    if (!BagState.SlotStates.IsValidIndex(TargetSlot))
    {
        OutReason = NSLOCTEXT("BagComponent", "InvalidSlot", "Invalid slot index");
        return false;
    }

    if (Item.ItemType == EItemType::Bag)
    {
        if (Item.ItemID == BagState.BagInfo.ItemID)
        {
            OutReason = NSLOCTEXT("BagComponent", "SelfNesting", "Cannot put a bag inside itself");
            return false;
        }

        if (HasCircularReference(*FString::Printf(TEXT("Bag_%s"), *Item.ItemID.ToString())))
        {
            OutReason = NSLOCTEXT("BagComponent", "CircularRef", "Cannot create circular bag reference");
            return false;
        }

        if (!IsBagEmpty())
        {
            OutReason = NSLOCTEXT("BagComponent", "NotEmpty", "Can only put bags into empty bags");
            return false;
        }
    }

    return true;
}

bool UBagComponent::TryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot)
{
    FText FailReason;
    if (!CanAcceptItem(Item, TargetSlot, FailReason) || !ValidateItemOperation(Item))
    {
        OnBagOperationFailed.Broadcast(FailReason);
        return false;
    }

    BagState.SlotStates[TargetSlot].ItemInfo = Item;
    BagState.SlotStates[TargetSlot].Quantity = Quantity;

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }

    NotifySlotUpdated(TargetSlot);
    RequestWeightUpdate();

    return true;
}

bool UBagComponent::TryRemoveItem(int32 SlotIndex)
{
    if (!BagState.SlotStates.IsValidIndex(SlotIndex))
        return false;

    BagState.SlotStates[SlotIndex].Clear();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SaveState();
    }

    NotifySlotUpdated(SlotIndex);
    RequestWeightUpdate();
    return true;
}

void UBagComponent::LoadState(const FBagState& State)
{
    UE_LOG(LogTemp, Warning, TEXT("Loading state for bag %s"), *State.BagKey.ToString());
    
    BagState = State;

    if (BagState.SlotStates.Num() != BagState.BagInfo.BagSlots)
    {
        BagState.SlotStates.SetNum(BagState.BagInfo.BagSlots);
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        MarkPackageDirty();
    }

    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        NotifySlotUpdated(i);
    }

    RequestWeightUpdate();
}

void UBagComponent::SaveState()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !BagState.BagKey.IsValid())
    {
        return;
    }

    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
    {
        Character->SaveBagState(this);
        MarkPackageDirty();
    }
}

void UBagComponent::ServerTryAddItem_Implementation(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
        return;

    if (TryAddItem(Item, Quantity, TargetSlot))
    {
        SaveState();
    }
}

void UBagComponent::ServerTryRemoveItem_Implementation(int32 SlotIndex)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
        return;

    if (TryRemoveItem(SlotIndex))
    {
        SaveState();
    }
}

void UBagComponent::RequestWeightUpdate()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WeightUpdateTimer);
        World->GetTimerManager().SetTimer(WeightUpdateTimer,
            [this]() { UpdateWeight(); },
            0.1f, false);
    }
}

void UBagComponent::UpdateWeight()
{
    if (bIsUpdatingWeight)
    {
        bPendingWeightUpdate = true;
        return;
    }

    bIsUpdatingWeight = true;
    bSuppressSave = true;

    float NewWeight = GetRecursiveWeight();

    if (!FMath::IsNearlyEqual(LastCalculatedWeight, NewWeight))
    {
        LastCalculatedWeight = NewWeight;
        OnWeightChanged.Broadcast(NewWeight);
    }

    bSuppressSave = false;
    bIsUpdatingWeight = false;

    if (bPendingWeightUpdate)
    {
        bPendingWeightUpdate = false;
        RequestWeightUpdate();
    }
}

float UBagComponent::GetRecursiveWeight() const
{
    float TotalContentsWeight = 0.0f;
    TSet<FName> VisitedKeys;
    
    for (const FBagSlotState& Slot : BagState.SlotStates)
    {
        if (!Slot.IsEmpty())
        {
            if (Slot.ItemInfo.ItemType == EItemType::Bag)
            {
                FName NestedBagKey = *FString::Printf(TEXT("Bag_%s"), *Slot.ItemInfo.ItemID.ToString());
                if (!VisitedKeys.Contains(NestedBagKey))
                {
                    VisitedKeys.Add(NestedBagKey);
                    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
                    {
                        if (UBagComponent* NestedBag = Character->FindBagByKey(NestedBagKey))
                        {
                            TotalContentsWeight += NestedBag->GetRecursiveWeight();
                        }
                    }
                }
            }
            else
            {
                TotalContentsWeight += (Slot.ItemInfo.Weight * Slot.Quantity);
            }
        }
    }

    return BagState.BagInfo.Weight + TotalContentsWeight;
}

bool UBagComponent::HasCircularReference(const FName& BagKey) const
{
    if (BagKey == BagState.BagKey)
        return true;

    TSet<FName> VisitedKeys;
    return CheckCircularReference(BagKey, VisitedKeys);
}

bool UBagComponent::IsBagEmpty() const
{
    for (const FBagSlotState& Slot : BagState.SlotStates)
    {
        if (!Slot.IsEmpty())
            return false;
    }
    return true;
}

void UBagComponent::NotifySlotUpdated(int32 SlotIndex)
{
    if (BagState.SlotStates.IsValidIndex(SlotIndex))
    {
        const FBagSlotState& State = BagState.SlotStates[SlotIndex];
        OnSlotUpdated.Broadcast(SlotIndex, State.ItemInfo, State.Quantity);
        RequestWeightUpdate();
    }
}

void UBagComponent::OnRep_BagState()
{
    for (int32 i = 0; i < BagState.SlotStates.Num(); ++i)
    {
        NotifySlotUpdated(i);
    }
    RequestWeightUpdate();
}

void UBagComponent::OnRep_IsOpen()
{
    if (bIsOpen)
        OnBagOpened.Broadcast(this);
    else
        OnBagClosed.Broadcast(this);
}

void UBagComponent::CleanupTimers()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SaveDebounceTimer);
        World->GetTimerManager().ClearTimer(WeightUpdateTimer);
    }
}

bool UBagComponent::CheckCircularReference(const FName& TargetBagKey, TSet<FName>& VisitedKeys) const
{
    if (VisitedKeys.Contains(BagState.BagKey))
        return false;

    VisitedKeys.Add(BagState.BagKey);

    for (const FBagSlotState& Slot : BagState.SlotStates)
    {
        if (!Slot.IsEmpty() && Slot.ItemInfo.ItemType == EItemType::Bag)
        {
            FName NestedBagKey = *FString::Printf(TEXT("Bag_%s"), *Slot.ItemInfo.ItemID.ToString());
            
            if (NestedBagKey == TargetBagKey)
                return true;

            if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwner()))
            {
                if (UBagComponent* NestedBag = Character->FindBagByKey(NestedBagKey))
                {
                    if (NestedBag->CheckCircularReference(TargetBagKey, VisitedKeys))
                        return true;
                }
            }
        }
    }

    return false;
}

bool UBagComponent::ValidateItemOperation(const FS_ItemInfo& Item) const
{
    return Item.ItemID.IsValid() && Item.MaxStackSize > 0;
}