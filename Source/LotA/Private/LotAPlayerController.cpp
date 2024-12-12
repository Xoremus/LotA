// LotAPlayerController.cpp
#include "LotAPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"

ALotAPlayerController::ALotAPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ALotAPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Add Input Mapping Context
    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                InputSystem->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext added"));
            }
        }
    }

    // Create and Add Main Inventory Widget
    if (MainInventoryWidgetClass)
    {
        MainInventoryWidget = CreateWidget<UMainInventoryWidget>(this, MainInventoryWidgetClass);
        if (MainInventoryWidget)
        {
            MainInventoryWidget->AddToViewport();
            MainInventoryWidget->SetVisibility(ESlateVisibility::Hidden);
            UE_LOG(LogTemp, Warning, TEXT("MainInventoryWidget created and added to viewport"));
        }
    }
}

void ALotAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UE_LOG(LogTemp, Warning, TEXT("SetupInputComponent called"));

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent found"));
        
        if (IA_Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("IA_Inventory is valid"));
            EnhancedInput->BindAction(IA_Inventory, ETriggerEvent::Started, this, &ALotAPlayerController::ToggleMainInventory);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("IA_Inventory is null"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("EnhancedInputComponent not found"));
    }
}

void ALotAPlayerController::ToggleMainInventory()
{
    if (!MainInventoryWidget) return;

    if (MainInventoryWidget->IsVisible())
    {
        MainInventoryWidget->SetVisibility(ESlateVisibility::Hidden);
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;

        // Close all open bags when closing inventory
        TArray<UBagComponent*> BagsToClose = OpenBags;
        for (UBagComponent* Bag : BagsToClose)
        {
            if (Bag)
            {
                Bag->CloseBag();
            }
        }
    }
    else
    {
        MainInventoryWidget->SetVisibility(ESlateVisibility::Visible);
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
}

void ALotAPlayerController::OpenAllBags()
{
    // Get all bag components from the player's inventory
    if (APawn* PlayerPawn = GetPawn())
    {
        TArray<UBagComponent*> AllBags;
        PlayerPawn->GetComponents<UBagComponent>(AllBags);

        for (UBagComponent* Bag : AllBags)
        {
            if (Bag && !Bag->IsBagOpen())
            {
                Bag->OpenBag();
            }
        }
    }
}

void ALotAPlayerController::OnBagOpened(UBagComponent* Bag)
{
    if (Bag)
    {
        OpenBags.AddUnique(Bag);
    }
}

void ALotAPlayerController::OnBagClosed(UBagComponent* Bag)
{
    if (Bag)
    {
        OpenBags.Remove(Bag);
    }
}