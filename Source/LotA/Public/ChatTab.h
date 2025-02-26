// ChatTab.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChatMessage.h"
#include "Components/ScrollBox.h"
#include "ChatTab.generated.h"

/**
 * Represents a single chat tab with filtering settings
 */
UCLASS(BlueprintType, Blueprintable)
class LOTA_API UChatTab : public UObject
{
    GENERATED_BODY()

public:
    UChatTab();
    
    // Tab name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    FString TabName;
    
    // Chat channel visibility settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
    TMap<EChatChannel, bool> ChannelVisibility;
    
    // The scroll box containing messages for this tab
    UPROPERTY()
    UScrollBox* MessageContainer;
    
    // Initialize default settings
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void Initialize(const FString& InTabName, UScrollBox* InMessageContainer);
    
    // Check if channel is visible in this tab
    UFUNCTION(BlueprintCallable, Category = "Chat")
    bool IsChannelVisible(EChatChannel Channel) const;
    
    // Set channel visibility
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void SetChannelVisibility(EChatChannel Channel, bool bVisible);
    
    // Reset to default visibility settings
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void ResetToDefaults();
    
    // Clear all messages
    UFUNCTION(BlueprintCallable, Category = "Chat")
    void ClearMessages();
};