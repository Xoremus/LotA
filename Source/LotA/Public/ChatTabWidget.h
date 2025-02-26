// Fill out your copyright notice in the Description page of Project Settings.

// ChatTabWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "ChatMessage.h"
#include "ChatTab.h"
#include "ChatTabWidget.generated.h"

/**
 * Blueprint-friendly wrapper for ChatTab that can be used directly in UMG
 */
UCLASS()
class LOTA_API UChatTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Initialize with a tab object
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetTab(UChatTab* InTab);
    
	// Get the tab
	UFUNCTION(BlueprintPure, Category = "Chat")
	UChatTab* GetTab() const { return Tab; }
    
	// Add a message to this tab
	UFUNCTION(BlueprintCallable, Category = "Chat")
	bool AddMessage(const FChatMessage& Message);
    
	// Clear all messages
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ClearMessages();
    
	// Set channel visibility
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SetChannelVisibility(EChatChannel Channel, bool bVisible);
    
	// Get channel visibility
	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsChannelVisible(EChatChannel Channel) const;
    
protected:
	virtual void NativeConstruct() override;
    
	// The scroll box container for messages
	UPROPERTY(meta = (BindWidget))
	UScrollBox* MessageScrollBox;
    
	// Tab info
	UPROPERTY()
	UChatTab* Tab;
};