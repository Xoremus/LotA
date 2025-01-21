#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BagTypes.h"
#include "BagComponent.h"
#include "InputActionValue.h"
#include "ItemBase.h"
#include "MainInventoryWidget.h"
#include "InventorySlotWidget.h"
#include "LotACharacter.generated.h"

class UInteractionComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UCharacterStatsComponent;
class UBagComponent;

/**
 * ALotACharacter
 * 
 * Features:
 * - MMO-style WASD movement relative to camera
 * - Right Mouse Button camera control
 * - Auto-rotation to movement direction
 * - Optional auto-run with Left Mouse Button
 * - Full bag/inventory system
 * - Press E to pickup items
 */
UCLASS(config=Game)
class LOTA_API ALotACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALotACharacter();

    virtual void Tick(float DeltaTime) override;

    // -----------------------------
    //  Input Actions
    // -----------------------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* JumpAction;

    /** Press E to pick up items */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_Interact;

    /** Right Mouse to control camera */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_RightMouse;

    /** Left Mouse for auto-run */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_AutoRun;

    // -----------------------------
    //  Camera
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, Category="Camera")
    float CameraPitchMin;

    UPROPERTY(EditAnywhere, Category="Camera")
    float CameraPitchMax;

    // -----------------------------
    //  Character Stats
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
    UCharacterStatsComponent* StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
    UInteractionComponent* InteractionComponent;

    // Movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float BaseWalkSpeed;

    // -----------------------------
    //  Bag System
    // -----------------------------
    /** Holds all bag states, replicated so server is authoritative */
    UPROPERTY(Replicated)
    FBagSaveData BagSaveData;

    /** Map of active bag components keyed by BagKey */
    UPROPERTY()
    TMap<FName, UBagComponent*> ActiveBagComponents;

    UPROPERTY()
    class UMainInventoryWidget* MainInventoryWidget;

    void HandleBagPickup(AItemBase* BagActor);
    // Bag functions
    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* AddBagComponent(const FS_ItemInfo& BagInfo);

    UFUNCTION()
    void SetMainInventoryWidget(UMainInventoryWidget* Widget) { MainInventoryWidget = Widget; }

    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* FindBagByKey(const FName& BagKey) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* FindBagComponent(const FName& BagKey) const;

    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
    void SaveBagState(UBagComponent* BagComp);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool GetSavedBagState(const FName& BagKey, FBagSavedState& OutState) const;

    UFUNCTION(BlueprintPure, Category="Inventory")
    float GetTotalBagsWeight() const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    void RestoreAllBagStates();

    UFUNCTION(BlueprintCallable, Category="Inventory")
    void OnTotalWeightChanged(float NewTotalWeight);

    void RemoveBagComponent(UBagComponent* BagComp);

    // -------------
    //  Pickup
    // -------------
    UFUNCTION(Server, Reliable)
    void ServerPickupItem(AItemBase* ItemActor);

    void UpdateBagWeights();

protected:
    // -----------------------------
    //  ACharacter overrides
    // -----------------------------
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Movement & Look
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ToggleAutoRun();

    /** Press E => line trace to pick up item */
    UFUNCTION()
    void OnInteract();

    /** Right Mouse pressed => activate camera control */
    UFUNCTION()
    void OnRightMousePressed();

    /** Right Mouse released => deactivate camera control */
    UFUNCTION()
    void OnRightMouseReleased();

    // Bag / weight
    //void UpdateBagWeights();
    FName GenerateBagKey(const FS_ItemInfo& BagInfo) const;
    bool IsBagKeyValid(const FName& BagKey) const;

    UFUNCTION()
    void OnBagWeightChanged(float NewWeight);

    /** Helper for picking up items => find a free/stackable slot */
    UFUNCTION()
    int32 FindOrCreateSlotIndex(UBagComponent* Bag, const FS_ItemInfo& Item, int32 Quantity);

private:
    /** Movement state */
    bool bIsRightMouseDown;
    bool bIsAutoRunning;
    FVector AutoRunDirection;

    /** When true, moving mouse up will look down */
    UPROPERTY(Config, EditAnywhere, Category="Camera|Controls")
    bool bInvertMouseY;

    UPROPERTY(EditAnywhere, Category="Inventory")
    float MaxCarryWeight = 100.0f;

    struct FBagSlotInfo
    {
        UBagComponent* Bag;
        int32 SlotIndex;
        bool bIsPartialStack;
        int32 AvailableSpace;

        FBagSlotInfo() : Bag(nullptr), SlotIndex(INDEX_NONE), bIsPartialStack(false), AvailableSpace(0) {}
    };
    
    bool FindAvailableSlotForItem(const FS_ItemInfo& Item, int32 Quantity, FBagSlotInfo& OutSlotInfo);

    bool AddItemToSlot(const FBagSlotInfo& SlotInfo, const FS_ItemInfo& Item, int32 Quantity);
};