#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BagComponent.h"
#include "BagTypes.h"
#include "InputActionValue.h"
#include "LotACharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UCharacterStatsComponent;
class UBagComponent;

// Replicatable structure for active bags
USTRUCT()
struct FActiveBagEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FName BagKey;

    UPROPERTY()
    UBagComponent* BagComponent;

    FActiveBagEntry() : BagComponent(nullptr) {}
};

UCLASS(config=Game)
class LOTA_API ALotACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALotACharacter();

    // Enhanced Input
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    // Camera components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Character components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    UCharacterStatsComponent* StatsComponent;

    // Movement properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float BaseWalkSpeed;

    // === Bag System ===
    // Save data for all bags
    UPROPERTY(Replicated)
    FBagSaveData BagSaveData;

    // Active bag components
    UPROPERTY()
    TMap<FName, UBagComponent*> ActiveBagComponents;

    // Bag Functions
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UBagComponent* AddBagComponent(const FS_ItemInfo& BagInfo);

    // For compatibility with existing code
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UBagComponent* FindBagByKey(const FName& BagKey) const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UBagComponent* FindBagComponent(const FName& BagKey) const;

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
    void SaveBagState(UBagComponent* BagComp);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool GetSavedBagState(const FName& BagKey, FBagSavedState& OutState) const;

    // Weight calculations
    UFUNCTION(BlueprintPure, Category = "Inventory")
    float GetTotalBagsWeight() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RestoreAllBagStates();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void OnTotalWeightChanged(float NewTotalWeight);

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Enhanced Input functions
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    // Helper functions
    void UpdateBagWeights();
    FName GenerateBagKey(const FS_ItemInfo& BagInfo) const;
    bool IsBagKeyValid(const FName& BagKey) const;
    
    UFUNCTION()
    void OnBagWeightChanged(float NewWeight);

private:
    UPROPERTY(EditAnywhere, Category = "Inventory")
    float MaxCarryWeight = 100.0f;
};