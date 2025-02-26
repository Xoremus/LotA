// LotACharacter.h
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

UCLASS(config=Game)
class LOTA_API ALotACharacter : public ACharacter
{
    GENERATED_BODY()

public:    
    ALotACharacter();

    virtual void Tick(float DeltaTime) override;

    // Input Actions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_Interact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_RightMouse;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* IA_AutoRun;

    // Camera
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
    UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, Category="Camera")
    float CameraPitchMin;

    UPROPERTY(EditAnywhere, Category="Camera")
    float CameraPitchMax;

    // Stats
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats")
    UCharacterStatsComponent* StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
    UInteractionComponent* InteractionComponent;

    // Movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float BaseWalkSpeed;

    // Bag System
    UPROPERTY(Replicated)
    FBagSaveData BagSaveData;

    UPROPERTY()
    TMap<FName, UBagComponent*> ActiveBagComponents;

    UPROPERTY()
    class UMainInventoryWidget* MainInventoryWidget;

    void HandleBagPickup(AItemBase* BagActor);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* AddBagComponent(const FS_ItemInfo& BagInfo);

    UFUNCTION()
    void SetMainInventoryWidget(UMainInventoryWidget* Widget) { MainInventoryWidget = Widget; }

    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* FindBagByKey(const FName& BagKey) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    UBagComponent* FindBagComponent(const FName& BagKey) const;

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
    void SaveAllBagStates();

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

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool ValidateBagOperation(const FS_ItemInfo& BagInfo, FText& OutErrorMessage) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool HasCircularBagReference(const FName& BagKey, const FName& TargetBagKey) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    void GetAllActiveBagKeys(TArray<FName>& OutBagKeys) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SaveAllBagsAndClose();

    void RemoveBagComponent(UBagComponent* BagComp);

    // Pickup
    UFUNCTION(Server, Reliable)
    void ServerPickupItem(AItemBase* ItemActor);

    void UpdateBagWeights();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ToggleAutoRun();

    void ValidateBagHierarchy();

    UFUNCTION()
    void OnInteract();

    UFUNCTION()
    void OnRightMousePressed();

    UFUNCTION()
    void OnRightMouseReleased();

    UFUNCTION()
    void OnBagWeightChanged(float NewWeight);

    UFUNCTION()
    int32 FindOrCreateSlotIndex(UBagComponent* Bag, const FS_ItemInfo& Item, int32 Quantity);

private:
    bool bIsRightMouseDown;
    bool bIsAutoRunning;
    FVector AutoRunDirection;

    bool CheckCircularReference(const FName& StartBagKey, const FName& TargetBagKey, TSet<FName>& VisitedKeys) const;
    
    static const int32 MaxBagNestingDepth = 5;

    UPROPERTY(Config, EditAnywhere, Category="Camera|Controls")
    bool bInvertMouseY;

    UPROPERTY(EditAnywhere, Category="Inventory")
    float MaxCarryWeight = 100.0f;

    FName GenerateBagKey(const FS_ItemInfo& BagInfo) const;
    bool IsBagKeyValid(const FName& BagKey) const;
    
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