// MainInventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.h"
#include "Components/TextBlock.h"
#include "CharacterStatsComponent.h"
#include "MainInventoryWidget.generated.h"

// MainInventoryWidget.h - Add the text binding
UCLASS()
class LOTA_API UMainInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddTestItems();

	UFUNCTION()
	void UpdateWeightDisplay();

	UFUNCTION()
	void UpdateStatsDisplay();  // New function for updating stats

	UPROPERTY(meta = (BindWidget))
	UInventoryWidget* WBP_Inventory;

	// UI Elements
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeightText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StrengthText;  // Add binding for STR text

private:
	UPROPERTY()
	class UCharacterStatsComponent* StatsComponent;

	float CalculateTotalInventoryWeight() const;
};