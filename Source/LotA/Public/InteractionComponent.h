#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IInteractable.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractableLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionFailed, const FText&, FailureReason);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UInteractionComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Try to interact with whatever we're looking at
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryInteract();

    // Get the currently focused interactable actor
    UFUNCTION(BlueprintPure, Category = "Interaction")
    AActor* GetFocusedInteractable() const { return FocusedInteractable; }

    // Get current interaction text (if any)
    UFUNCTION(BlueprintPure, Category = "Interaction")
    FText GetCurrentInteractionText() const;

    // Events for UI feedback
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractableFound OnInteractableFound;

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractableLost OnInteractableLost;

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractionFailed OnInteractionFailed;

protected:
    virtual void BeginPlay() override;

    // Trace settings
    UPROPERTY(EditAnywhere, Category = "Interaction|Trace")
    float InteractionDistance = 300.f;

    UPROPERTY(EditAnywhere, Category = "Interaction|Trace")
    float TraceRadius = 25.f;

    UPROPERTY(EditAnywhere, Category = "Interaction|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    // How often to check for interactable objects (in seconds)
    UPROPERTY(EditAnywhere, Category = "Interaction|Trace")
    float TraceInterval = 0.1f;

    // Debug visualization
    UPROPERTY(EditAnywhere, Category = "Interaction|Debug")
    bool bShowDebugTrace = false;

private:
    // Perform the interaction trace
    void PerformInteractionTrace();

    // Handle item pickup
    UFUNCTION(Server, Reliable)
    void ServerHandleItemPickup(class AItemBase* ItemActor);

    // Try to find space for an item
    bool TryFindSpaceForItem(const struct FS_ItemInfo& ItemInfo, int32 Quantity);

    // Currently focused interactable actor
    UPROPERTY()
    AActor* FocusedInteractable;

    // Camera component reference (cached on begin play)
    UPROPERTY()
    class UCameraComponent* CameraComponent;

    float TimeSinceLastTrace;

    // Helper function to get owning character
    class ALotACharacter* GetOwningLotACharacter() const;

    // Helper to get the notification manager
    class APickupNotificationManager* GetNotificationManager() const;

    // Show pickup success notification
    void ShowPickupNotification(const FS_ItemInfo& ItemInfo, int32 Quantity) const;
};