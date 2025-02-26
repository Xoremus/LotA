// ChatTab.cpp
#include "ChatTab.h"

UChatTab::UChatTab()
{
    TabName = TEXT("General");
    ResetToDefaults();
}

void UChatTab::Initialize(const FString& InTabName, UScrollBox* InMessageContainer)
{
    TabName = InTabName;
    MessageContainer = InMessageContainer;
    ResetToDefaults();
}

bool UChatTab::IsChannelVisible(EChatChannel Channel) const
{
    const bool* Value = ChannelVisibility.Find(Channel);
    return Value ? *Value : false;
}

void UChatTab::SetChannelVisibility(EChatChannel Channel, bool bVisible)
{
    ChannelVisibility.Add(Channel, bVisible);
}

void UChatTab::ResetToDefaults()
{
    // By default, show all channels
    ChannelVisibility.Empty();
    
    ChannelVisibility.Add(EChatChannel::Say, true);
    ChannelVisibility.Add(EChatChannel::Shout, true);
    ChannelVisibility.Add(EChatChannel::Tell, true);
    ChannelVisibility.Add(EChatChannel::Group, true);
    ChannelVisibility.Add(EChatChannel::Guild, true);
    ChannelVisibility.Add(EChatChannel::Trade, true);
    ChannelVisibility.Add(EChatChannel::OOC, true);
    ChannelVisibility.Add(EChatChannel::General, true);
    ChannelVisibility.Add(EChatChannel::Region, true);
}

void UChatTab::ClearMessages()
{
    if (MessageContainer)
    {
        MessageContainer->ClearChildren();
    }
}