// ChatWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "ChatMessage.h"
#include "LotAPlayerState.h"
#include "ChatTab.h"
#include "ChatTabButtonWidget.h"
#include "ChatWidget.generated.h"

UCLASS()
class LOTA_API UChatWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void AddMessage(const FChatMessage& Message);

    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SetActiveChannel(EChatChannel NewChannel);

    UFUNCTION(BlueprintCallable, Category = "Chat")
    void ToggleChannelVisibility(EChatChannel Channel, bool bVisible);
    
    // Basic tab functions
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void CreateNewTab(const FString& TabName);
    
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SwitchToTab(int32 TabIndex);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* ChatScrollBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UEditableTextBox* InputTextBox;
    
    // New tab UI elements
    UPROPERTY(meta = (BindWidget))
    UWidgetSwitcher* TabContentSwitcher;
    
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* TabBar;
    
    UPROPERTY(meta = (BindWidget))
    UButton* NewTabButton;
    
    // Tab button widget class
    UPROPERTY(EditDefaultsOnly, Category = "Chat")
    TSubclassOf<UChatTabButtonWidget> TabButtonWidgetClass;

    UFUNCTION()
    void OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    
    UFUNCTION()
    void OnNewTabButtonClicked();
    
    UFUNCTION()
    void OnTabButtonClicked(int32 TabIndex);

private:
    // All chat tabs
    UPROPERTY()
    TArray<UChatTab*> ChatTabs;
    
    // Current active tab index
    int32 ActiveTabIndex;
    
    // Active channel for sending messages
    UPROPERTY()
    EChatChannel ActiveChannel;
    
    // Create tab content
    UScrollBox* CreateTabContent();
    
    // Create a tab button
    UChatTabButtonWidget* CreateTabButton(const FString& TabName, int32 TabIndex);
    
    // Update the input hint text
    void UpdateInputBoxHintText();
    
    // Process chat commands
    bool ProcessChatCommand(const FString& Input, FString& OutMessage, EChatChannel& OutChannel, FString& OutTarget);
};