// ChatTabButtonWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ChatTabButtonWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabButtonClicked, int32, TabIndex);

/**
 * Widget for a single chat tab button
 */
UCLASS()
class LOTA_API UChatTabButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set the tab name
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetTabName(const FString& InTabName);
    
	// Set the tab index
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetTabIndex(int32 InTabIndex);
    
	// Set active state
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetIsActive(bool bInIsActive);
    
	// Event when tab is clicked
	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FOnTabButtonClicked OnTabButtonClicked;
    
protected:
	virtual void NativeConstruct() override;
    
	UPROPERTY(meta = (BindWidget))
	UButton* TabButton;
    
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TabNameText;
    
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	FString TabName;
    
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	int32 TabIndex;
    
	UPROPERTY(BlueprintReadOnly, Category = "Chat")
	bool bIsActive;
    
	UFUNCTION()
	void OnButtonClicked();
};