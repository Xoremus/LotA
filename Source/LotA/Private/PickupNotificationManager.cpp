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
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("ShowPickupNotification: Invalid PlayerController"));
		return;
	}
    // ... existing validation code ...

    // Create new notification
    UPickupNotificationWidget* NewNotification = CreateWidget<UPickupNotificationWidget>(PC, NotificationWidgetClass);
    if (!NewNotification)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create new notification widget"));
        return;
    }

    // Add to viewport first
    NewNotification->AddToViewport(100);
    
    // Setup the notification
    NewNotification->SetupNotification(Item, Quantity);

    // Remove oldest if at max
    while (ActiveNotifications.Num() >= MaxNotifications)
    {
        if (ActiveNotifications.Num() > 0 && IsValid(ActiveNotifications[0]))
        {
            ActiveNotifications[0]->RemoveFromParent();
            ActiveNotifications.RemoveAt(0);
        }
    }

    // Add to active list and update positions
    ActiveNotifications.Add(NewNotification);
    UpdateNotificationPositions();
	
	// Bind the fade complete event for the new notification
	NewNotification->OnFadeComplete.AddDynamic(this, &APickupNotificationManager::OnNotificationFadeComplete);
}

void APickupNotificationManager::UpdateNotificationPositions()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateNotificationPositions: Invalid PlayerController"));
		return;
	}
    float CurrentY = 100.0f;

    for (int32 i = 0; i < ActiveNotifications.Num(); ++i)
    {
        UPickupNotificationWidget* Notification = ActiveNotifications[i];
        if (!IsValid(Notification))
        {
            continue;
        }

        // Get the widget's desired size
        FVector2D Size = Notification->GetDesiredSize();
        if (Size.Y <= 0)
        {
            // If size isn't available yet, use a default
            Size.Y = 50.0f;
        }
        
		// Position from the right edge
		int32 ViewportX = 0, ViewportY = 0;
		PC->GetViewportSize(ViewportX, ViewportY);
		FVector2D ViewportSize(ViewportX, ViewportY);
        FVector2D Position = FVector2D(ViewportSize.X - Size.X - 20.0f, CurrentY);

		// Set Position
        Notification->SetPositionInViewport(Position);

        // Move down by widget height plus spacing
        CurrentY += Size.Y + NotificationSpacing;
            
        // Log successful positioning
        UE_LOG(LogTemp, Warning, TEXT("Successfully positioned notification %d at Y=%f"), i, CurrentY - Size.Y);
    }
}

void APickupNotificationManager::OnNotificationFadeComplete(UPickupNotificationWidget* Widget)
{
    if (Widget)
    {
        ActiveNotifications.Remove(Widget);
        UpdateNotificationPositions();
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