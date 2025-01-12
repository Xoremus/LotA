#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BagTypes.h"
#include "BagComponent.h"
#include "InputActionValue.h"
#include "ItemBase.h"
#include "LotACharacter.generated.h"

// Forward declarations for your input assets, etc.
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
 * - Move with WASD (no auto-rotation)
 * - Camera orbits around character by default
 * - Hold Right Mouse => character rotates with mouse movement
 * - Press E => line trace to pick up items
 * - Full bag system (AddBagComponent, SaveBagState, etc.)
 */
UCLASS(config=Game)
class LOTA_API ALotACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // -----------------------------
    //  Constructor
    // -----------------------------
    ALotACharacter();

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

    /** Right Mouse to rotate the character with mouse */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_RightMouse;

    // -----------------------------
    //  Camera
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    UCameraComponent* FollowCamera;

    // -----------------------------
    //  Character Stats (optional)
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
    UCharacterStatsComponent* StatsComponent;

    // Movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float BaseWalkSpeed;

    // -----------------------------
    //  Bag System
    // -----------------------------
    /** Holds all bag states, replicated so the server is authoritative */
    UPROPERTY(Replicated)
    FBagSaveData BagSaveData;

    /** Map of active bag components keyed by BagKey */
    UPROPERTY()
    TMap<FName, UBagComponent*> ActiveBagComponents;

    // Bag functions
    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* AddBagComponent(const FS_ItemInfo& BagInfo);

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

    /** Press E => line trace to pick up item */
    UFUNCTION()
    void OnInteract();

    /** Right Mouse pressed => let character rotate with mouse */
    UFUNCTION()
    void OnRightMousePressed(const FInputActionValue& Value);

    /** Right Mouse released => revert to orbit mode */
    UFUNCTION()
    void OnRightMouseReleased(const FInputActionValue& Value);

    // Bag / weight
    void UpdateBagWeights();
    FName GenerateBagKey(const FS_ItemInfo& BagInfo) const;
    bool IsBagKeyValid(const FName& BagKey) const;

    UFUNCTION()
    void OnBagWeightChanged(float NewWeight);

    /** Helper for picking up items => find a free/stackable slot */
    UFUNCTION()
    int32 FindOrCreateSlotIndex(UBagComponent* Bag, const FS_ItemInfo& Item, int32 Quantity);

private:
    /** True if right mouse is down => we rotate the character with the mouse */
    bool bRightMouseDown;

    UPROPERTY(EditAnywhere, Category="Inventory")
    float MaxCarryWeight = 100.0f;
};
