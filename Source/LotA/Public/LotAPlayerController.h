// LotAPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LotA/LotACharacter.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MainInventoryWidget.h"
#include "ChatWidget.h"
#include "LotAPlayerController.generated.h"

UCLASS()
class LOTA_API ALotAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:    
    ALotAPlayerController();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Inventory;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_RightMouse;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_AutoRun;

    // UI Classes
    UPROPERTY(EditDefaultsOnly, Category = "UI|Inventory")
    TSubclassOf<UMainInventoryWidget> MainInventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Chat")
    TSubclassOf<UChatWidget> ChatWidgetClass;

protected:
    virtual void BeginPlay() override;
    // Changed this line to fix the override error
    virtual void SetupInputComponent() override;

    // UI References
    UPROPERTY()
    TObjectPtr<UMainInventoryWidget> MainInventoryWidget;

    UPROPERTY()
    TObjectPtr<UChatWidget> ChatWidget;

    // UI Toggle Functions
    UFUNCTION()
    void ToggleMainInventory();

    UFUNCTION()
    void OpenAllBags();

    // Bag Management
    UFUNCTION()
    void OnBagOpened(UBagComponent* Bag);

    UFUNCTION()
    void OnBagClosed(UBagComponent* Bag);

private:
    bool bIsRightMouseDown;
    bool bIsAutoRunning;
    FVector AutoRunDirection;

    UPROPERTY()
    TArray<UBagComponent*> OpenBags;

    // Input Handlers
    void OnRightMousePressed();
    void OnRightMouseReleased();

    // UI Creation
    void CreatePlayerUI();
};