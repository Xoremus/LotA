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
}

void UDragDropVisual::SetItemIcon(UTexture2D* Icon)
{
	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(Icon);
		ItemIcon->SetVisibility(Icon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
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