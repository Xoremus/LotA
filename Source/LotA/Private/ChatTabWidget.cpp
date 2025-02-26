// Fill out your copyright notice in the Description page of Project Settings.

// ChatTabWidget.cpp
#include "ChatTabWidget.h"

void UChatTabWidget::NativeConstruct()
{
	Super::NativeConstruct();
    
	// Create a new tab if we don't have one yet
	if (!Tab)
	{
		Tab = NewObject<UChatTab>(this);
		if (Tab)
		{
			Tab->Initialize(TEXT("Tab"), MessageScrollBox);
		}
	}
}

void UChatTabWidget::SetTab(UChatTab* InTab)
{
	Tab = InTab;
    
	if (Tab && MessageScrollBox)
	{
		// Update the tab's message container to point to our scroll box
		Tab->MessageContainer = MessageScrollBox;
	}
}

bool UChatTabWidget::AddMessage(const FChatMessage& Message)
{
	if (Tab)
	{
		return Tab->AddMessage(Message, GetOwningPlayer());
	}
	return false;
}

void UChatTabWidget::ClearMessages()
{
	if (Tab)
	{
		Tab->ClearMessages();
	}
}

void UChatTabWidget::SetChannelVisibility(EChatChannel Channel, bool bVisible)
{
	if (Tab)
	{
		Tab->SetChannelVisibility(Channel, bVisible);
	}
}

bool UChatTabWidget::IsChannelVisible(EChatChannel Channel) const
{
	if (Tab)
	{
		return Tab->IsChannelVisible(Channel);
	}
	return false;
}