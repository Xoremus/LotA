// LotAPlayerController.cpp
#include "LotAPlayerController.h"
#include "LotA/LotACharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ALotAPlayerController::ALotAPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ALotAPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Only create UI for local player
    if (IsLocalPlayerController())
    {
        CreatePlayerUI();
    }

    // Add Input Mapping Context
    if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                InputSystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

void ALotAPlayerController::CreatePlayerUI()
{
    // Create Main Inventory Widget
    if (MainInventoryWidgetClass)
    {
        MainInventoryWidget = CreateWidget<UMainInventoryWidget>(this, MainInventoryWidgetClass);
        if (MainInventoryWidget)
        {
            MainInventoryWidget->AddToViewport();
            MainInventoryWidget->SetVisibility(ESlateVisibility::Hidden);

            // Set the reference on the character
            if (ALotACharacter* LotAChar = Cast<ALotACharacter>(GetPawn()))
            {
                LotAChar->SetMainInventoryWidget(MainInventoryWidget);
            }
        }
    }

    // Create Chat Widget
    if (ChatWidgetClass)
    {
        ChatWidget = CreateWidget<UChatWidget>(this, ChatWidgetClass);
        if (ChatWidget)
        {
            ChatWidget->AddToViewport();
        }
    }
}

void ALotAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (IA_Inventory)
        {
            EnhancedInput->BindAction(IA_Inventory, ETriggerEvent::Started, this, &ALotAPlayerController::ToggleMainInventory);
        }

        if (IA_RightMouse)
        {
            EnhancedInput->BindAction(IA_RightMouse, ETriggerEvent::Started, this, &ALotAPlayerController::OnRightMousePressed);
            EnhancedInput->BindAction(IA_RightMouse, ETriggerEvent::Completed, this, &ALotAPlayerController::OnRightMouseReleased);
        }
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

        // Close all open bags
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

void ALotAPlayerController::OnRightMousePressed()
{
    bIsRightMouseDown = true;
    SetShowMouseCursor(false);
}

void ALotAPlayerController::OnRightMouseReleased()
{
    bIsRightMouseDown = false;
    SetShowMouseCursor(true);
}