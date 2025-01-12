#include "InteractionComponent.h"
#include "InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

UInteractionComponent::UInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    TimeSinceLastTrace = 0.0f;
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
    if (FocusedInteractable && FocusedInteractable->Implements<UInteractable>())
    {
        IInteractable::Execute_OnInteract(FocusedInteractable, GetOwner());
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
        return;

    FVector Start = CameraComponent->GetComponentLocation();
    FVector End = Start + (CameraComponent->GetForwardVector() * InteractionDistance);
    
    FCollisionShape Shape = FCollisionShape::MakeSphere(TraceRadius);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    
    FHitResult Hit;
    bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, TraceChannel, Shape, Params);

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

    #if WITH_EDITOR
    // Debug visualization
    DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.1f);
    if (bHit)
    {
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Yellow, false, 0.1f);
    }
    #endif
}