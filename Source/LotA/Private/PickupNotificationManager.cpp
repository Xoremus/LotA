// PickupNotificationManager.cpp
#include "PickupNotificationManager.h"
#include "PickupNotificationWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

APickupNotificationManager::APickupNotificationManager()
{
    PrimaryActorTick.bCanEverTick = false;

    // Try to load the widget class directly
    static ConstructorHelpers::FClassFinder<UPickupNotificationWidget> WidgetClassFinder(TEXT("/Game/Inventory/Widgets/WBP_PickupNotification"));
    if (WidgetClassFinder.Succeeded())
    {
        NotificationWidgetClass = WidgetClassFinder.Class;
        UE_LOG(LogTemp, Warning, TEXT("Constructor: Found widget class: %s"), *NotificationWidgetClass->GetName());
    }
}

void APickupNotificationManager::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("PickupNotificationManager BeginPlay"));
    
    if (!NotificationWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("NotificationWidgetClass is NULL in BeginPlay"));
        LogDebugState();
        return;
    }

    if (!NotificationWidgetClass->IsChildOf(UPickupNotificationWidget::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("NotificationWidgetClass '%s' is not a child of UPickupNotificationWidget"), 
            *NotificationWidgetClass->GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("NotificationWidgetClass validated successfully: %s"), 
        *NotificationWidgetClass->GetName());
}

void APickupNotificationManager::ShowPickupNotification(const FS_ItemInfo& Item, int32 Quantity)
{
    if (!NotificationWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowPickupNotification: NotificationWidgetClass is NULL"));
        LogDebugState();
        return;
    }

    // Get the local player controller
    APlayerController* PC = nullptr;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* TestPC = Iterator->Get();
        if (TestPC && TestPC->IsLocalController())
        {
            UE_LOG(LogTemp, Warning, TEXT("Found local player controller: %s"), *TestPC->GetName());
            PC = TestPC;
            break;
        }
    }

    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowPickupNotification: No local player controller found"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== ShowPickupNotification Start ==="));
    UE_LOG(LogTemp, Warning, TEXT("Current Active Notifications: %d"), ActiveNotifications.Num());

    // Cleanup old or invalid notifications
    int32 PreCleanupCount = ActiveNotifications.Num();
    ActiveNotifications.RemoveAll([](UPickupNotificationWidget* Widget) {
        return !IsValid(Widget) || !Widget->IsVisible();
    });
    int32 PostCleanupCount = ActiveNotifications.Num();
    
    UE_LOG(LogTemp, Warning, TEXT("Cleanup: Before=%d, After=%d"), PreCleanupCount, PostCleanupCount);

    // Create new notification
    UPickupNotificationWidget* NewNotification = CreateWidget<UPickupNotificationWidget>(PC, NotificationWidgetClass);
    if (!NewNotification)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create new notification widget"));
        return;
    }

    // Add to viewport before setting up
    NewNotification->AddToViewport(100);
    NewNotification->SetupNotification(Item, Quantity);

    // Defer positioning to next frame
    GetWorld()->GetTimerManager().SetTimerForNextTick([this, NewNotification]() {
        if (!IsValid(NewNotification))
        {
            UE_LOG(LogTemp, Error, TEXT("Notification became invalid before positioning"));
            return;
        }

        // Add to our list first
        ActiveNotifications.Add(NewNotification);
        
        UE_LOG(LogTemp, Warning, TEXT("=== Positioning Notifications ==="));
        UE_LOG(LogTemp, Warning, TEXT("Active Notifications Count: %d"), ActiveNotifications.Num());

        float CurrentY = 100.0f;
        for (int32 i = 0; i < ActiveNotifications.Num(); ++i)
        {
            UPickupNotificationWidget* Notification = ActiveNotifications[i];
            if (!IsValid(Notification))
            {
                UE_LOG(LogTemp, Warning, TEXT("Notification at index %d is invalid"), i);
                continue;
            }

            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Notification->Slot))
            {
                FVector2D Size = Notification->GetDesiredSize();
                CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f));
                CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
                CanvasSlot->SetPosition(FVector2D(-20.0f, CurrentY));
                CanvasSlot->SetAutoSize(true);

                CurrentY += Size.Y + NotificationSpacing;

                UE_LOG(LogTemp, Warning, TEXT("Notification %d positioned at Y=%f, Height=%f"), 
                    i, CurrentY - Size.Y, Size.Y);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to get canvas slot for notification %d"), i);
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("=== Positioning Complete ==="));
    });

    UE_LOG(LogTemp, Warning, TEXT("=== ShowPickupNotification End ==="));
}

void APickupNotificationManager::UpdateNotificationPositions()
{
    float CurrentY = 100.0f;

    for (int32 i = 0; i < ActiveNotifications.Num(); ++i)
    {
        UPickupNotificationWidget* Notification = ActiveNotifications[i];
        if (!IsValid(Notification))
            continue;

        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Notification->Slot))
        {
            CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f));
            CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
            CanvasSlot->SetPosition(FVector2D(-20.0f, CurrentY));

            CurrentY += Notification->GetDesiredSize().Y + NotificationSpacing;
        }
    }
}

void APickupNotificationManager::LogDebugState() const
{
    UE_LOG(LogTemp, Warning, TEXT("=== PickupNotificationManager Debug State ==="));
    UE_LOG(LogTemp, Warning, TEXT("NotificationWidgetClass: %s"), 
        NotificationWidgetClass ? *NotificationWidgetClass->GetName() : TEXT("None"));
    UE_LOG(LogTemp, Warning, TEXT("Active Notifications: %d"), ActiveNotifications.Num());
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}