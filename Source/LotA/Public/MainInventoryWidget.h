// MainInventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CharacterStatsComponent.h"
#include "MainInventoryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryOperationFailed, const FText&, FailureReason);

UCLASS()
class LOTA_API UMainInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddTestItems();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RequestWeightUpdate();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateStatsDisplay();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    float CalculateTotalInventoryWeight() const;

    // NEW: Added function to validate bag operations
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool ValidateBagOperation(const FS_ItemInfo& BagInfo, FText& OutErrorMessage) const;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* WBP_Inventory;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WeightText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StrengthText;

    // NEW: Added delegate for operation failures
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryOperationFailed OnInventoryOperationFailed;

protected:
    UFUNCTION()
    void OnDoneButtonClicked();

    UFUNCTION()
    void OnExitButtonClicked();

    UPROPERTY(meta = (BindWidget))
    UButton* ExitButton;

    UPROPERTY(meta = (BindWidget))
    UButton* DoneButton;

private:
    UPROPERTY()
    UCharacterStatsComponent* StatsComponent;

    float LastTotalWeight;
    bool bWeightUpdatePending;
    FTimerHandle WeightUpdateTimer;

    void UpdateInventoryWeight();
    void SetGameOnlyMode();

    // NEW: Track active bags for validation
    TSet<FName> GetActiveBagKeys() const;
};