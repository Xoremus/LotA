// DestroyConfirmationWidget.cpp
#include "DestroyConfirmationWidget.h"
#include "Components/Button.h"

void UDestroyConfirmationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (DestroyButton)
	{
		DestroyButton->OnClicked.AddDynamic(this, &UDestroyConfirmationWidget::OnDestroyClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UDestroyConfirmationWidget::OnCancelClicked);
	}
}

void UDestroyConfirmationWidget::SetItemToDestroy(const FS_ItemInfo& Item, int32 Amount)
{
	ItemInfo = Item;
	ItemAmount = Amount;
}

void UDestroyConfirmationWidget::OnDestroyClicked()
{
	// Broadcast the item being destroyed
	OnDestroyConfirmed.Broadcast(ItemInfo);
	RemoveFromParent();
}

void UDestroyConfirmationWidget::OnCancelClicked()
{
	// Just close the widget
	RemoveFromParent();
}