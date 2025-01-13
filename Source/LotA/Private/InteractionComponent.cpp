#include "InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "LotA/LotACharacter.h"
#include "DrawDebugHelpers.h"

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
        UE_LOG(LogTemp, Warning, TEXT("Executing OnInteract for %s"), 
            *FocusedInteractable->GetName());

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
        UE_LOG(LogTemp, Warning, TEXT("%s does not implement IInteractable"), 
            *FocusedInteractable->GetName());
        OnInteractionFailed.Broadcast(FText::FromString("Invalid interaction target"));
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

void UInteractionComponent::ServerHandleItemPickup_Implementation(AItemBase* ItemActor)
{
    if (!IsValid(ItemActor) || ItemActor->IsActorBeingDestroyed())
    {
        OnInteractionFailed.Broadcast(FText::FromString("Invalid item"));
        return;
    }

    ALotACharacter* Character = GetOwningLotACharacter();
    if (!Character)
    {
        OnInteractionFailed.Broadcast(FText::FromString("Invalid character"));
        return;
    }

    const FS_ItemInfo& ItemInfo = ItemActor->ItemDetails;
    const int32 PickupQuantity = ItemActor->StackCount;

    UE_LOG(LogTemp, Warning, TEXT("Attempting to pick up: %s (x%d)"), 
        *ItemInfo.ItemName.ToString(), PickupQuantity);

    if (TryFindSpaceForItem(ItemInfo, PickupQuantity))
    {
        ItemActor->OnPickedUp();
        Character->UpdateBagWeights();
    }
    else
    {
        OnInteractionFailed.Broadcast(FText::FromString("No space available"));
    }
}

bool UInteractionComponent::TryFindSpaceForItem(const FS_ItemInfo& ItemInfo, int32 Quantity)
{
    ALotACharacter* Character = GetOwningLotACharacter();
    if (!Character) return false;

    // Check for stacking in all bags first
    for (const auto& BagPair : Character->ActiveBagComponents)
    {
        if (UBagComponent* Bag = BagPair.Value)
        {
            const TArray<FBagSlotState>& BagSlots = Bag->GetSlotStates();

            // Look for partial stacks first
            for (int32 i = 0; i < BagSlots.Num(); i++)
            {
                const FBagSlotState& Slot = BagSlots[i];
                if (!Slot.IsEmpty() && Slot.ItemInfo.ItemID == ItemInfo.ItemID)
                {
                    int32 SpaceAvailable = ItemInfo.MaxStackSize - Slot.Quantity;
                    if (SpaceAvailable > 0)
                    {
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

    return false;
}

ALotACharacter* UInteractionComponent::GetOwningLotACharacter() const
{
    return Cast<ALotACharacter>(GetOwner());
}