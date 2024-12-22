// MainInventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.h"
#include "Components/TextBlock.h"
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
	void UpdateInventoryWeight();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateStatsDisplay();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateWeightDisplay();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float CalculateTotalInventoryWeight() const;

	UPROPERTY(meta = (BindWidget))
	UInventoryWidget* WBP_Inventory;

	// UI Elements
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeightText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StrengthText;

private:
	UPROPERTY()
	class UCharacterStatsComponent* StatsComponent;
};