// MainInventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.h"
#include "MainInventoryWidget.generated.h"

UCLASS()
class LOTA_API UMainInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddTestItems();

	UPROPERTY(meta = (BindWidget))
	UInventoryWidget* WBP_Inventory;
};