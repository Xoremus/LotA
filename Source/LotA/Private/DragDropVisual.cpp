// DragDropVisual.cpp
#include "DragDropVisual.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UDragDropVisual::UDragDropVisual(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDragDropVisual::NativeConstruct()
{
	Super::NativeConstruct();
    
	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	}

	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UDragDropVisual::SetItemIcon(UTexture2D* Icon)
{
	if (ItemIcon && Icon)
	{
		ItemIcon->SetBrushFromTexture(Icon);
		ItemIcon->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Warning, TEXT("DragDropVisual: Setting icon image"));
	}
}

void UDragDropVisual::SetQuantityText(int32 Quantity)
{
	if (QuantityText)
	{
		if (Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(Quantity));
			QuantityText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}