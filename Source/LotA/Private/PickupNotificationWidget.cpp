// PickupNotificationWidget.cpp
#include "PickupNotificationWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Animation/UMGSequencePlayer.h"
#include "Components/CanvasPanelSlot.h"

void UPickupNotificationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetRenderOpacity(1.0f);
}

void UPickupNotificationWidget::SetupNotification(const FS_ItemInfo& Item, int32 Quantity)
{
    if (ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(Item.ItemIcon);
    }

    if (ItemName)
    {
        ItemName->SetText(Item.ItemName);
    }

    if (QuantityText)
    {
        QuantityText->SetText(Quantity > 1 ? 
            FText::Format(NSLOCTEXT("PickupNotification", "QuantityFormat", "x{0}"), Quantity) : 
            FText::GetEmpty());
    }
	
	// Store start position
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		StartPosition = CanvasSlot->GetPosition();
	}
	
    // Start fade timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(FadeTimer, this, &UPickupNotificationWidget::StartFadeOut, FadeOutDelay);
		
		// Initialize scrolling
		CurrentScrollTime = 0.0f;
        World->GetTimerManager().SetTimer(ScrollTimer, this, &UPickupNotificationWidget::UpdatePosition, World->GetDeltaSeconds(), true);

    }
}

void UPickupNotificationWidget::StartFadeOut()
{
    if (UWorld* World = GetWorld())
    {
        float* ElapsedTimePtr = new float(0.0f);
        
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([this, ElapsedTimePtr]()
        {
            if (!IsValid(this))
            {
                delete ElapsedTimePtr;
                return;
            }

            if (UWorld* TimerWorld = GetWorld())
            {
                *ElapsedTimePtr += TimerWorld->GetDeltaSeconds();
                float Alpha = FMath::Min(*ElapsedTimePtr / FadeOutDuration, 1.0f);
                SetRenderOpacity(1.0f - Alpha);

                if (Alpha >= 1.0f)
                {
                    if (TimerWorld && FadeTimer.IsValid())
                    {
                        TimerWorld->GetTimerManager().ClearTimer(FadeTimer);
                    }
					if (TimerWorld && ScrollTimer.IsValid())
					{
						TimerWorld->GetTimerManager().ClearTimer(ScrollTimer);
					}
                    // Broadcast before removal
                    OnFadeComplete.Broadcast(this);
                    RemoveFromParent();
                    delete ElapsedTimePtr;
                }
            }
        });

        World->GetTimerManager().SetTimer(FadeTimer, TimerDelegate, World->GetDeltaSeconds(), true);
    }
}

void UPickupNotificationWidget::UpdatePosition()
{
	if (!IsValid(this))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ScrollTimer);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		CurrentScrollTime += World->GetDeltaSeconds();
		float Alpha = FMath::Min(CurrentScrollTime / ScrollDuration, 1.0f);
	    float VerticalOffset = Alpha * -80.f;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			CanvasSlot->SetPosition(StartPosition + FVector2D(0, VerticalOffset));
		}
	}
}