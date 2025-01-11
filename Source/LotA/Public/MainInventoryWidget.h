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

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* WBP_Inventory;

    // UI Elements
    UPROPERTY(meta = (BindWidget))
    UTextBlock* WeightText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StrengthText;

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
};