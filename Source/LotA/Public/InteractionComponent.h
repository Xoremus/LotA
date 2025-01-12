#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IInteractable.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractableLost);

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

protected:
    virtual void BeginPlay() override;

    // Trace settings
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractionDistance = 300.f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float TraceRadius = 25.f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    // How often to check for interactable objects (in seconds)
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float TraceInterval = 0.1f;

private:
    // Perform the interaction trace
    void PerformInteractionTrace();

    // Currently focused interactable actor
    UPROPERTY()
    AActor* FocusedInteractable;

    // Camera component reference (cached on begin play)
    UPROPERTY()
    class UCameraComponent* CameraComponent;

    float TimeSinceLastTrace;
};