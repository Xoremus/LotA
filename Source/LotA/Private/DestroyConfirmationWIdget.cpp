// DestroyConfirmationWidget.cpp
#include "DestroyConfirmationWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

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

	if (ConfirmationText)
	{
		FString Message;
		if (Item.ItemType == EItemType::Bag)
		{
			Message = FString::Printf(TEXT("Are you sure you want to destroy this %s? Any contents will be lost."), 
				*Item.ItemName.ToString());
		}
		else
		{
			Message = FString::Printf(TEXT("Are you sure you want to destroy %d %s?"), 
				Amount, *Item.ItemName.ToString());
		}
		ConfirmationText->SetText(FText::FromString(Message));
	}
}

void UDestroyConfirmationWidget::OnDestroyClicked()
{
	OnDestroyConfirmed.Broadcast(ItemInfo);
	RemoveFromParent();
}

void UDestroyConfirmationWidget::OnCancelClicked()
{
	OnDestroyCancelled.Broadcast();
	RemoveFromParent();
}