// BagComponent.cpp
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

    // Create inventory slots if we're the server
    if (GetOwnerRole() == ROLE_Authority)
    {
        CreateInventorySlots();
    }
}

void UBagComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    CloseBag();

    // Clean up inventory slots
    for (auto* Slot : InventorySlots)
    {
        if (Slot)
        {
            Slot->DestroyComponent();
        }
    }
    InventorySlots.Empty();
}

void UBagComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBagComponent, bIsOpen);
    DOREPLIFETIME(UBagComponent, BagInfo);
}

bool UBagComponent::OpenBag()
{
    if (bIsOpen || !BagInfo.ItemID.IsValid())
        return false;

    bIsOpen = true;
    OnBagOpened.Broadcast(this);

    // Only create widget on local client
    if (GetOwner()->HasLocalNetOwner())
    {
        // Widget creation will be handled by the inventory system
    }

    return true;
}

void UBagComponent::CloseBag()
{
    if (!bIsOpen)
        return;

    bIsOpen = false;
    OnBagClosed.Broadcast(this);

    // Widget removal will be handled by the inventory system
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

bool UBagComponent::HasItems() const
{
    for (const auto* Slot : InventorySlots)
    {
        if (Slot && !Slot->IsEmpty())
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
    for (const auto* Slot : InventorySlots)
    {
        if (Slot && !Slot->IsEmpty())
        {
            TotalWeight += Slot->ItemData.Weight * Slot->StackCount;
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
        CreateInventorySlots();
    }
}

void UBagComponent::CreateInventorySlots()
{
    // Clear existing slots
    for (auto* Slot : InventorySlots)
    {
        if (Slot)
        {
            Slot->DestroyComponent();
        }
    }
    InventorySlots.Empty();

    // Create new slots based on BagInfo.BagSlots
    for (int32 i = 0; i < BagInfo.BagSlots; ++i)
    {
        UInventorySlotDataComponent* NewSlot = NewObject<UInventorySlotDataComponent>(GetOwner());
        if (NewSlot)
        {
            NewSlot->RegisterComponent();
            InventorySlots.Add(NewSlot);
        }
    }
}