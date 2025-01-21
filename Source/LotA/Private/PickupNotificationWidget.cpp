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

	// Start fade timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FadeTimer, this, &UPickupNotificationWidget::StartFadeOut, FadeOutDelay);
	}
}

void UPickupNotificationWidget::StartFadeOut()
{
	// Create a lambda for the fade animation
	if (UWorld* World = GetWorld())
	{
		// Store ElapsedTime as a shared pointer so it persists between timer callbacks
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
					RemoveFromParent();
					delete ElapsedTimePtr;
				}
			}
		});

		World->GetTimerManager().SetTimer(FadeTimer, TimerDelegate, World->GetDeltaSeconds(), true);
	}
}