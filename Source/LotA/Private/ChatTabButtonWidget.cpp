// ChatTabButtonWidget.cpp
#include "ChatTabButtonWidget.h"

void UChatTabButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (TabButton)
	{
		TabButton->OnClicked.AddDynamic(this, &UChatTabButtonWidget::OnButtonClicked);
	}
    
	// Set default values
	TabName = TEXT("Tab");
	TabIndex = 0;
	bIsActive = false;
    
	// Update visuals
	SetTabName(TabName);
	SetIsActive(bIsActive);
}

void UChatTabButtonWidget::SetTabName(const FString& InTabName)
{
	TabName = InTabName;
    
	if (TabNameText)
	{
		TabNameText->SetText(FText::FromString(TabName));
	}
}

void UChatTabButtonWidget::SetTabIndex(int32 InTabIndex)
{
	TabIndex = InTabIndex;
}

void UChatTabButtonWidget::SetIsActive(bool bInIsActive)
{
	bIsActive = bInIsActive;
    
	// Update visual appearance based on active state
	if (TabButton)
	{
		FLinearColor ButtonColor = bIsActive ? 
			FLinearColor(0.2f, 0.2f, 0.3f, 1.0f) : 
			FLinearColor(0.1f, 0.1f, 0.15f, 1.0f);
            
		TabButton->SetBackgroundColor(ButtonColor);
	}
    
	if (TabNameText)
	{
		FSlateColor TextColor = bIsActive ? 
			FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)) : 
			FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
            
		TabNameText->SetColorAndOpacity(TextColor);
	}
}

void UChatTabButtonWidget::OnButtonClicked()
{
	// Broadcast the event
	OnTabButtonClicked.Broadcast(TabIndex);
}