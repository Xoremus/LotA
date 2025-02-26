// Fill out your copyright notice in the Description page of Project Settings.

// ChatTabSettingsWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "ChatMessage.h"
#include "ChatTab.h"
#include "ChatTabSettingsWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsApplied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTabDeleted);

/**
 * Widget for configuring chat tab settings
 */
UCLASS()
class LOTA_API UChatTabSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Set the tab to configure
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SetTab(UChatTab* InTab);
    
    // Apply settings and close
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void ApplySettings();
    
    // Cancel and close without applying
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void CancelSettings();
    
    // Delete the current tab
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void DeleteTab();
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Chat")
    FOnSettingsApplied OnSettingsApplied;
    
    UPROPERTY(BlueprintAssignable, Category = "Chat")
    FOnSettingsCancelled OnSettingsCancelled;
    
    UPROPERTY(BlueprintAssignable, Category = "Chat")
    FOnTabDeleted OnTabDeleted;
    
protected:
    virtual void NativeConstruct() override;
    
    // UI Elements
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* TabNameInput;
    
    UPROPERTY(meta = (BindWidget))
    UButton* ApplyButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* CancelButton;
    
    UPROPERTY(meta = (BindWidget))
    UButton* DeleteButton;
    
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* ChannelOptionsBox;
    
    // Channel checkboxes
    UPROPERTY()
    TMap<EChatChannel, UCheckBox*> ChannelCheckboxes;
    
private:
    // The tab being configured
    UPROPERTY()
    UChatTab* CurrentTab;
    
    // Button handlers
    UFUNCTION()
    void OnApplyButtonClicked();
    
    UFUNCTION()
    void OnCancelButtonClicked();
    
    UFUNCTION()
    void OnDeleteButtonClicked();
    
    // Create channel filter options
    void CreateChannelOptions();
    
    // Update channel checkbox states
    void UpdateCheckboxStates();
    
    // Channel checkbox change handler
    UFUNCTION()
    void OnChannelCheckboxChanged(bool bIsChecked);
};