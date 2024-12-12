// LotAPlayerController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MainInventoryWidget.h"
#include "BagComponent.h"
#include "LotAPlayerController.generated.h"

UCLASS()
class LOTA_API ALotAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALotAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Inventory;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_OpenAllBags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UMainInventoryWidget> MainInventoryWidgetClass;

private:
    UPROPERTY()
    TObjectPtr<UMainInventoryWidget> MainInventoryWidget;

    // Track open bags
    UPROPERTY()
    TArray<UBagComponent*> OpenBags;

    UFUNCTION()
    void ToggleMainInventory();

    UFUNCTION()
    void OpenAllBags();

    UFUNCTION()
    void OnBagOpened(UBagComponent* Bag);

    UFUNCTION()
    void OnBagClosed(UBagComponent* Bag);
};