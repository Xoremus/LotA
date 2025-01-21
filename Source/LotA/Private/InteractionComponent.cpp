#include "InteractionComponent.h"
#include "LotA/LotACharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "ItemBase.h"
#include "MainInventoryWidget.h"
#include "InventoryWidget.h"
#include "InventorySlotWidget.h"
#include "PickupNotificationManager.h"
#include "Kismet/GameplayStatics.h"

UInteractionComponent::UInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    TimeSinceLastTrace = 0.0f;
    
    UE_LOG(LogTemp, Warning, TEXT("InteractionComponent constructed"));
}

void UInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache camera reference
    if (APawn* Owner = Cast<APawn>(GetOwner()))
    {
        TArray<UCameraComponent*> Cameras;
        Owner->GetComponents<UCameraComponent>(Cameras);
        if (Cameras.Num() > 0)
        {
            CameraComponent = Cameras[0];
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("InteractionComponent BeginPlay on %s"), 
        *GetOwner()->GetName());
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TimeSinceLastTrace += DeltaTime;
    if (TimeSinceLastTrace >= TraceInterval)
    {
        PerformInteractionTrace();
        TimeSinceLastTrace = 0.0f;
    }
}

void UInteractionComponent::TryInteract()
{
    UE_LOG(LogTemp, Warning, TEXT("TryInteract called, FocusedInteractable: %s"), 
        FocusedInteractable ? *FocusedInteractable->GetName() : TEXT("None"));

    if (!FocusedInteractable)
    {
        OnInteractionFailed.Broadcast(FText::FromString("No valid target"));
        return;
    }

    if (FocusedInteractable->Implements<UInteractable>())
    {
        // Special handling for items
        if (AItemBase* Item = Cast<AItemBase>(FocusedInteractable))
        {
            ServerHandleItemPickup(Item);
        }
        else
        {
            // Generic interaction
            IInteractable::Execute_OnInteract(FocusedInteractable, GetOwner());
        }
    }
    else
    {
        OnInteractionFailed.Broadcast(FText::FromString("Invalid interaction target"));
    }
}

void UInteractionComponent::ServerHandleItemPickup_Implementation(AItemBase* ItemActor)
{
    if (!IsValid(ItemActor) || ItemActor->IsActorBeingDestroyed())
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerHandleItemPickup: Invalid item actor"));
        OnInteractionFailed.Broadcast(FText::FromString("Invalid item"));
        return;
    }

    const FS_ItemInfo& ItemInfo = ItemActor->ItemDetails;
    const int32 PickupQuantity = ItemActor->StackCount;

    UE_LOG(LogTemp, Warning, TEXT("ServerHandleItemPickup: Attempting pickup of %s (x%d)"), 
        *ItemInfo.ItemName.ToString(), PickupQuantity);

    if (TryFindSpaceForItem(ItemInfo, PickupQuantity))
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerHandleItemPickup: Found space for item, showing notification"));
        ShowPickupNotification(ItemInfo, PickupQuantity);
        ItemActor->OnPickedUp();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerHandleItemPickup: No space found for item"));
        OnInteractionFailed.Broadcast(FText::FromString("Inventory is full"));
    }
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
    if (FocusedInteractable && FocusedInteractable->Implements<UInteractable>())
    {
        return IInteractable::Execute_GetInteractionText(FocusedInteractable, GetOwner());
    }
    return FText::GetEmpty();
}

void UInteractionComponent::PerformInteractionTrace()
{
    if (!CameraComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("No camera component found!"));
        return;
    }

    FVector Start = CameraComponent->GetComponentLocation();
    FVector End = Start + (CameraComponent->GetForwardVector() * InteractionDistance);
    
    FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    
    FHitResult Hit;
    bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, 
        TraceChannel, Shape, Params);

    if (bShowDebugTrace)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, 
            false, -1.0f, 0, 2.0f);
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), Hit.ImpactPoint, TraceRadius, 12, 
                FColor::Yellow, false, -1.0f, 0, 2.0f);
        }
    }

    AActor* NewFocus = nullptr;
    if (bHit)
    {
        if (Hit.GetActor()->Implements<UInteractable>())
        {
            if (IInteractable::Execute_CanInteract(Hit.GetActor(), GetOwner()))
            {
                NewFocus = Hit.GetActor();
            }
        }
    }

    // Handle focus changes
    if (NewFocus != FocusedInteractable)
    {
        if (FocusedInteractable)
        {
            OnInteractableLost.Broadcast();
        }

        FocusedInteractable = NewFocus;

        if (FocusedInteractable)
        {
            OnInteractableFound.Broadcast(FocusedInteractable);
        }
    }
}

bool UInteractionComponent::TryFindSpaceForItem(const FS_ItemInfo& ItemInfo, int32 Quantity)
{
    ALotACharacter* Character = GetOwningLotACharacter();
    if (!Character) 
    {
        UE_LOG(LogTemp, Warning, TEXT("TryFindSpaceForItem: No valid character"));
        return false;
    }

    // First check main inventory slots
    if (UMainInventoryWidget* MainInv = Cast<UMainInventoryWidget>(Character->MainInventoryWidget))
    {
        if (UInventoryWidget* InvWidget = MainInv->WBP_Inventory)
        {
            const TArray<UInventorySlotWidget*>& Slots = InvWidget->GetInventorySlots();
            UE_LOG(LogTemp, Warning, TEXT("Checking main inventory with %d slots"), Slots.Num());

            // First try stacking
            for (UInventorySlotWidget* Slot : Slots)
            {
                if (Slot && !Slot->IsEmpty() && Slot->GetItemInfo().ItemID == ItemInfo.ItemID)
                {
                    int32 SpaceAvailable = ItemInfo.MaxStackSize - Slot->GetQuantity();
                    if (SpaceAvailable > 0)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Found partial stack in main inventory with %d space"), SpaceAvailable);
                        int32 AmountToAdd = FMath::Min(Quantity, SpaceAvailable);
                        Slot->SetItemDetails(ItemInfo, Slot->GetQuantity() + AmountToAdd);
                        return true;
                    }
                }
            }

            // Then try empty slots
            for (UInventorySlotWidget* Slot : Slots)
            {
                if (Slot && Slot->IsEmpty())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Found empty slot in main inventory"));
                    Slot->SetItemDetails(ItemInfo, Quantity);
                    return true;
                }
            }
        }
    }

    // Then check bags
    UE_LOG(LogTemp, Warning, TEXT("Checking %d bag components for space"), Character->ActiveBagComponents.Num());

    for (const auto& BagPair : Character->ActiveBagComponents)
    {
        if (UBagComponent* Bag = BagPair.Value)
        {
            const TArray<FBagSlotState>& BagSlots = Bag->GetSlotStates();
            UE_LOG(LogTemp, Warning, TEXT("Checking bag %s with %d slots"), *BagPair.Key.ToString(), BagSlots.Num());

            // Look for partial stacks first
            for (int32 i = 0; i < BagSlots.Num(); i++)
            {
                const FBagSlotState& Slot = BagSlots[i];
                if (!Slot.IsEmpty() && Slot.ItemInfo.ItemID == ItemInfo.ItemID)
                {
                    int32 SpaceAvailable = ItemInfo.MaxStackSize - Slot.Quantity;
                    if (SpaceAvailable > 0)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Found partial stack in slot %d with %d space"), i, SpaceAvailable);
                        int32 AmountToAdd = FMath::Min(Quantity, SpaceAvailable);
                        if (Bag->TryAddItem(ItemInfo, Slot.Quantity + AmountToAdd, i))
                        {
                            return true;
                        }
                    }
                }
            }

            // Then look for empty slots
            for (int32 i = 0; i < BagSlots.Num(); i++)
            {
                if (BagSlots[i].IsEmpty())
                {
                    if (Bag->TryAddItem(ItemInfo, Quantity, i))
                    {
                        return true;
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No suitable slot found for item"));
    return false;
}

ALotACharacter* UInteractionComponent::GetOwningLotACharacter() const
{
    return Cast<ALotACharacter>(GetOwner());
}

APickupNotificationManager* UInteractionComponent::GetNotificationManager() const
{
    auto Manager = Cast<APickupNotificationManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), APickupNotificationManager::StaticClass()));
    
    if (Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetNotificationManager: Found manager: %s"), *Manager->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GetNotificationManager: Failed to find manager"));
    }
    
    return Manager;
}

void UInteractionComponent::ShowPickupNotification(const FS_ItemInfo& ItemInfo, int32 Quantity) const
{
    if (APickupNotificationManager* NotificationManager = GetNotificationManager())
    {
        NotificationManager->ShowPickupNotification(ItemInfo, Quantity);
    }
}