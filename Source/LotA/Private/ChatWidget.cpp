// ChatWidget.cpp
#include "ChatWidget.h"
#include "ChatComponent.h"
#include "Blueprint/WidgetTree.h"

void UChatWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("ChatWidget::NativeConstruct - Start"));

    // Initialize defaults
    ActiveChannel = EChatChannel::Say;
    ActiveTabIndex = 0;
    ChatTabs.Empty();

    // Find and bind to the Chat Component
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UChatComponent* ChatComp = Pawn->FindComponentByClass<UChatComponent>())
            {
                UE_LOG(LogTemp, Warning, TEXT("Found ChatComponent, binding to OnChatMessageReceived"));
                ChatComp->OnChatMessageReceived.AddDynamic(this, &UChatWidget::AddMessage);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No ChatComponent found on Pawn"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No Pawn found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No PlayerController found"));
    }

    // Bind input text event
    if (InputTextBox)
    {
        InputTextBox->OnTextCommitted.AddDynamic(this, &UChatWidget::OnInputTextCommitted);
    }

    // Bind new tab button if available
    if (NewTabButton)
    {
        NewTabButton->OnClicked.AddDynamic(this, &UChatWidget::OnNewTabButtonClicked);
    }

    // Create initial tab if none exist
    if (TabContentSwitcher && TabBar)
    {
        // Create "General" tab
        CreateNewTab(TEXT("General"));
    }
    else if (ChatScrollBox)
    {
        // If no tab system is available, just create a simple tab using the existing scroll box
        UChatTab* DefaultTab = NewObject<UChatTab>(this);
        DefaultTab->Initialize(TEXT("General"), ChatScrollBox);
        ChatTabs.Add(DefaultTab);
    }

    UpdateInputBoxHintText();

    UE_LOG(LogTemp, Warning, TEXT("ChatWidget::NativeConstruct - End"));
}

void UChatWidget::AddMessage(const FChatMessage& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("ChatWidget::AddMessage called with message: %s"), *Message.Message);

    // Check if we're using the tab system
    if (ChatTabs.Num() > 0)
    {
        bool bMessageAdded = false;
        
        // Legacy code for backward compatibility with the old scroll box
        if (ChatTabs.Num() == 1 && ChatTabs[0]->MessageContainer == ChatScrollBox)
        {
            // Only one tab using the main scroll box, use original code
            if (!ChatScrollBox)
            {
                UE_LOG(LogTemp, Error, TEXT("ChatWidget::AddMessage - No ChatScrollBox found!"));
                return;
            }
            
            // Create a new text block for the message
            UTextBlock* MessageBlock = NewObject<UTextBlock>(this);
            if (!MessageBlock)
            {
                UE_LOG(LogTemp, Error, TEXT("ChatWidget::AddMessage - Failed to create TextBlock!"));
                return;
            }
            
            // Get the local player's state
            ALotAPlayerState* LocalPlayerState = nullptr;
            if (APlayerController* PC = GetOwningPlayer())
            {
                LocalPlayerState = PC->GetPlayerState<ALotAPlayerState>();
                // Update local player name if needed
                if (LocalPlayerState && LocalPlayerState->PlayerDisplayName.Equals(TEXT("Player")))
                {
                    LocalPlayerState->PlayerDisplayName = FString::Printf(TEXT("Player_%d"), LocalPlayerState->GetPlayerId());
                }
            }
            
            // Compare using the same format
            const bool IsSenderLocalPlayer = LocalPlayerState && 
                Message.SenderName.Equals(FString::Printf(TEXT("Player_%d"), LocalPlayerState->GetPlayerId()));
            
            // Format message based on channel
            FString FormattedMessage;
            switch (Message.Channel)
            {
                case EChatChannel::Tell:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("You Whisper to %s: %s"), *Message.ReceiverName, *Message.Message) :
                        FString::Printf(TEXT("%s Whispers: %s"), *Message.SenderName, *Message.Message);
                    break;
                case EChatChannel::Group:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("[Group] You Say: %s"), *Message.Message) :
                        FString::Printf(TEXT("[Group] %s Says: %s"), *Message.SenderName, *Message.Message);
                    break;
                case EChatChannel::Guild:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("[Guild] You Say: %s"), *Message.Message) :
                        FString::Printf(TEXT("[Guild] %s Says: %s"), *Message.SenderName, *Message.Message);
                    break;
                case EChatChannel::Shout:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("You Shout: %s"), *Message.Message) :
                        FString::Printf(TEXT("%s Shouts: %s"), *Message.SenderName, *Message.Message);
                    break;
                case EChatChannel::OOC:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("[OOC] You: %s"), *Message.Message) :
                        FString::Printf(TEXT("[OOC] %s: %s"), *Message.SenderName, *Message.Message);
                    break;
                case EChatChannel::Trade:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("[Trade] You: %s"), *Message.Message) :
                        FString::Printf(TEXT("[Trade] %s: %s"), *Message.SenderName, *Message.Message);
                    break;
                default:
                    FormattedMessage = IsSenderLocalPlayer ?
                        FString::Printf(TEXT("You Say: %s"), *Message.Message) :
                        FString::Printf(TEXT("%s Says: %s"), *Message.SenderName, *Message.Message);
                    break;
            }
            
            MessageBlock->SetText(FText::FromString(FormattedMessage));
            
            // Set color based on channel
            FLinearColor ChannelColor;
            switch (Message.Channel)
            {
                case EChatChannel::Say:
                    ChannelColor = FLinearColor::White;
                    break;
                case EChatChannel::Shout:
                    ChannelColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
                    break;
                case EChatChannel::Tell:
                    ChannelColor = FLinearColor(1.0f, 0.75f, 0.8f); // Pink
                    break;
                case EChatChannel::Group:
                    ChannelColor = FLinearColor(0.0f, 1.0f, 0.0f); // Green
                    break;
                case EChatChannel::Guild:
                    ChannelColor = FLinearColor(0.0f, 1.0f, 1.0f); // Cyan
                    break;
                case EChatChannel::Trade:
                    ChannelColor = FLinearColor(1.0f, 1.0f, 0.0f); // Yellow
                    break;
                case EChatChannel::OOC:
                    ChannelColor = FLinearColor(0.5f, 0.5f, 0.5f); // Gray
                    break;
                case EChatChannel::General:
                    ChannelColor = FLinearColor::White;
                    break;
                case EChatChannel::Region:
                    ChannelColor = FLinearColor(0.8f, 0.8f, 1.0f); // Light Blue
                    break;
                default:
                    ChannelColor = FLinearColor::White;
                    break;
            }
            MessageBlock->SetColorAndOpacity(FSlateColor(ChannelColor));
            
            ChatScrollBox->AddChild(MessageBlock);
            ChatScrollBox->ScrollToEnd();
            bMessageAdded = true;
        }
        else
        {
            // Add message to each tab that wants to display this channel
            for (UChatTab* Tab : ChatTabs)
            {
                if (Tab && Tab->IsChannelVisible(Message.Channel))
                {
                    if (Tab->MessageContainer)
                    {
                        // Create a new text block for the message
                        UTextBlock* MessageBlock = NewObject<UTextBlock>(Tab->MessageContainer);
                        if (!MessageBlock)
                        {
                            continue;
                        }
                        
                        // Get the local player's state
                        ALotAPlayerState* LocalPlayerState = nullptr;
                        if (APlayerController* PC = GetOwningPlayer())
                        {
                            LocalPlayerState = PC->GetPlayerState<ALotAPlayerState>();
                        }
                        
                        // Compare using the same format
                        const bool IsSenderLocalPlayer = LocalPlayerState && 
                            Message.SenderName.Equals(LocalPlayerState->PlayerDisplayName);
                        
                        // Format message based on channel
                        FString FormattedMessage;
                        switch (Message.Channel)
                        {
                            case EChatChannel::Tell:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("You Whisper to %s: %s"), *Message.ReceiverName, *Message.Message) :
                                    FString::Printf(TEXT("%s Whispers: %s"), *Message.SenderName, *Message.Message);
                                break;
                            case EChatChannel::Group:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("[Group] You Say: %s"), *Message.Message) :
                                    FString::Printf(TEXT("[Group] %s Says: %s"), *Message.SenderName, *Message.Message);
                                break;
                            case EChatChannel::Guild:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("[Guild] You Say: %s"), *Message.Message) :
                                    FString::Printf(TEXT("[Guild] %s Says: %s"), *Message.SenderName, *Message.Message);
                                break;
                            case EChatChannel::Shout:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("You Shout: %s"), *Message.Message) :
                                    FString::Printf(TEXT("%s Shouts: %s"), *Message.SenderName, *Message.Message);
                                break;
                            case EChatChannel::OOC:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("[OOC] You: %s"), *Message.Message) :
                                    FString::Printf(TEXT("[OOC] %s: %s"), *Message.SenderName, *Message.Message);
                                break;
                            case EChatChannel::Trade:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("[Trade] You: %s"), *Message.Message) :
                                    FString::Printf(TEXT("[Trade] %s: %s"), *Message.SenderName, *Message.Message);
                                break;
                            default:
                                FormattedMessage = IsSenderLocalPlayer ?
                                    FString::Printf(TEXT("You Say: %s"), *Message.Message) :
                                    FString::Printf(TEXT("%s Says: %s"), *Message.SenderName, *Message.Message);
                                break;
                        }
                        
                        MessageBlock->SetText(FText::FromString(FormattedMessage));
                        
                        // Set color based on channel
                        FLinearColor ChannelColor;
                        switch (Message.Channel)
                        {
                            case EChatChannel::Say:
                                ChannelColor = FLinearColor::White;
                                break;
                            case EChatChannel::Shout:
                                ChannelColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
                                break;
                            case EChatChannel::Tell:
                                ChannelColor = FLinearColor(1.0f, 0.75f, 0.8f); // Pink
                                break;
                            case EChatChannel::Group:
                                ChannelColor = FLinearColor(0.0f, 1.0f, 0.0f); // Green
                                break;
                            case EChatChannel::Guild:
                                ChannelColor = FLinearColor(0.0f, 1.0f, 1.0f); // Cyan
                                break;
                            case EChatChannel::Trade:
                                ChannelColor = FLinearColor(1.0f, 1.0f, 0.0f); // Yellow
                                break;
                            case EChatChannel::OOC:
                                ChannelColor = FLinearColor(0.5f, 0.5f, 0.5f); // Gray
                                break;
                            case EChatChannel::General:
                                ChannelColor = FLinearColor::White;
                                break;
                            case EChatChannel::Region:
                                ChannelColor = FLinearColor(0.8f, 0.8f, 1.0f); // Light Blue
                                break;
                            default:
                                ChannelColor = FLinearColor::White;
                                break;
                        }
                        
                        MessageBlock->SetColorAndOpacity(FSlateColor(ChannelColor));
                        
                        Tab->MessageContainer->AddChild(MessageBlock);
                        Tab->MessageContainer->ScrollToEnd();
                        bMessageAdded = true;
                    }
                }
            }
        }
        
        if (!bMessageAdded)
        {
            UE_LOG(LogTemp, Warning, TEXT("Message was not added to any tab. Channel: %d"), static_cast<int32>(Message.Channel));
        }
    }
}

void UChatWidget::SetActiveChannel(EChatChannel NewChannel)
{
    ActiveChannel = NewChannel;
    UpdateInputBoxHintText();
}

void UChatWidget::ToggleChannelVisibility(EChatChannel Channel, bool bVisible)
{
    // Apply to the active tab
    if (ChatTabs.IsValidIndex(ActiveTabIndex))
    {
        ChatTabs[ActiveTabIndex]->SetChannelVisibility(Channel, bVisible);
    }
}

void UChatWidget::CreateNewTab(const FString& TabName)
{
    // Early out if we don't have the required widgets
    if (!TabContentSwitcher || !TabBar)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateNewTab: Missing TabContentSwitcher or TabBar"));
        return;
    }
    
    // Create a new scroll box for this tab's content
    UScrollBox* NewScrollBox = CreateTabContent();
    if (!NewScrollBox)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create scroll box for new tab"));
        return;
    }
    
    // Add the scroll box to the tab switcher
    int32 NewContentIndex = TabContentSwitcher->AddChild(NewScrollBox);
    
    // Create the actual tab object
    UChatTab* NewTab = NewObject<UChatTab>(this);
    if (!NewTab)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create new chat tab object"));
        return;
    }
    
    // Initialize the tab
    NewTab->Initialize(TabName, NewScrollBox);
    
    // Add to our array
    int32 NewTabIndex = ChatTabs.Add(NewTab);
    
    // Create and add tab button to tab bar if we have a class
    if (TabButtonWidgetClass)
    {
        UChatTabButtonWidget* TabButton = CreateTabButton(TabName, NewTabIndex);
        if (TabButton)
        {
            TabBar->AddChild(TabButton);
        }
    }
    
    // Switch to the new tab
    SwitchToTab(NewTabIndex);
}

void UChatWidget::SwitchToTab(int32 TabIndex)
{
    if (ChatTabs.IsValidIndex(TabIndex) && TabContentSwitcher)
    {
        TabContentSwitcher->SetActiveWidgetIndex(TabIndex);
        ActiveTabIndex = TabIndex;
        
        // Update tab button visuals
        for (int32 i = 0; i < TabBar->GetChildrenCount(); i++)
        {
            if (UChatTabButtonWidget* TabButton = Cast<UChatTabButtonWidget>(TabBar->GetChildAt(i)))
            {
                TabButton->SetIsActive(i == ActiveTabIndex);
            }
        }
    }
}

void UChatWidget::OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod != ETextCommit::OnEnter) return;
    if (Text.IsEmpty()) return;

    // Process the input text
    FString Message = Text.ToString();
    FString OutMessage;
    EChatChannel OutChannel;
    FString OutTarget;

    // Get the owning player's ChatComponent
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UChatComponent* ChatComp = Pawn->FindComponentByClass<UChatComponent>())
            {
                if (ProcessChatCommand(Message, OutMessage, OutChannel, OutTarget))
                {
                    ChatComp->ServerSendMessage(OutMessage, OutChannel, OutTarget);
                }
            }
        }
    }

    // Clear the input box
    InputTextBox->SetText(FText::GetEmpty());
}

void UChatWidget::OnNewTabButtonClicked()
{
    // For simplicity, just create a numbered tab
    FString NewTabName = FString::Printf(TEXT("Tab %d"), ChatTabs.Num() + 1);
    CreateNewTab(NewTabName);
}

void UChatWidget::OnTabButtonClicked(int32 TabIndex)
{
    SwitchToTab(TabIndex);
}

UScrollBox* UChatWidget::CreateTabContent()
{
    UScrollBox* NewScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    if (NewScrollBox)
    {
        // Configure scroll box settings
        NewScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
        NewScrollBox->SetAlwaysShowScrollbar(true);
    }
    return NewScrollBox;
}

UChatTabButtonWidget* UChatWidget::CreateTabButton(const FString& TabName, int32 TabIndex)
{
    if (!TabButtonWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("No TabButtonWidgetClass set"));
        return nullptr;
    }
    
    UChatTabButtonWidget* TabButton = CreateWidget<UChatTabButtonWidget>(this, TabButtonWidgetClass);
    if (TabButton)
    {
        TabButton->SetTabName(TabName);
        TabButton->SetTabIndex(TabIndex);
        TabButton->OnTabButtonClicked.AddDynamic(this, &UChatWidget::OnTabButtonClicked);
        
        // Mark as active if this is the active tab
        TabButton->SetIsActive(TabIndex == ActiveTabIndex);
    }
    
    return TabButton;
}

void UChatWidget::UpdateInputBoxHintText()
{
    if (!InputTextBox) return;

    FString ChannelName;
    switch (ActiveChannel)
    {
        case EChatChannel::Say:     ChannelName = TEXT("Say"); break;
        case EChatChannel::Tell:    ChannelName = TEXT("Tell"); break;
        case EChatChannel::Group:   ChannelName = TEXT("Group"); break;
        case EChatChannel::Guild:   ChannelName = TEXT("Guild"); break;
        case EChatChannel::Trade:   ChannelName = TEXT("Trade"); break;
        case EChatChannel::Shout:   ChannelName = TEXT("Shout"); break;
        case EChatChannel::OOC:     ChannelName = TEXT("OOC"); break;
        default:                    ChannelName = TEXT("General"); break;
    }

    InputTextBox->SetHintText(FText::FromString(FString::Printf(TEXT("Enter message... [%s]"), *ChannelName)));
}

bool UChatWidget::ProcessChatCommand(const FString& Input, FString& OutMessage, EChatChannel& OutChannel, FString& OutTarget)
{
    if (Input.IsEmpty()) return false;

    // Default to current active channel if no command
    if (!Input.StartsWith("/"))
    {
        OutMessage = Input;
        OutChannel = ActiveChannel;
        return true;
    }

    TArray<FString> Args;
    Input.ParseIntoArray(Args, TEXT(" "), true);
    if (Args.Num() == 0) return false;

    FString Command = Args[0].ToLower();
    Args.RemoveAt(0);  // Remove the command, leaving just the message parts

    // Join the remaining arguments back into the message
    OutMessage = FString::Join(Args, TEXT(" "));

    if (Command == TEXT("/s") || Command == TEXT("/say"))
    {
        OutChannel = EChatChannel::Say;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/sh") || Command == TEXT("/shout"))
    {
        OutChannel = EChatChannel::Shout;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/t") || Command == TEXT("/tell") || Command == TEXT("/w") || Command == TEXT("/whisper"))
    {
        if (Args.Num() < 2) return false;  // Need target and message
        OutTarget = Args[0];
        Args.RemoveAt(0);  // Remove target name
        OutMessage = FString::Join(Args, TEXT(" "));
        OutChannel = EChatChannel::Tell;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/g") || Command == TEXT("/group"))
    {
        OutChannel = EChatChannel::Group;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/gu") || Command == TEXT("/guild"))
    {
        OutChannel = EChatChannel::Guild;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/ooc"))
    {
        OutChannel = EChatChannel::OOC;
        SetActiveChannel(OutChannel);
    }
    else if (Command == TEXT("/tr") || Command == TEXT("/trade"))
    {
        OutChannel = EChatChannel::Trade;
        SetActiveChannel(OutChannel);
    }
    // Simple tab commands
    else if (Command == TEXT("/tab"))
    {
        if (Args.Num() > 0)
        {
            FString TabName = FString::Join(Args, TEXT(" "));
            CreateNewTab(TabName);
            OutMessage = FString::Printf(TEXT("Created new tab: %s"), *TabName);
            OutChannel = EChatChannel::General;
            return true;
        }
    }
    else if (Command == TEXT("/filter"))
    {
        if (Args.Num() < 2)
        {
            OutMessage = TEXT("Usage: /filter [channel] [on|off]");
            OutChannel = EChatChannel::General;
            return true;
        }
        
        FString ChannelStr = Args[0].ToLower();
        FString StateStr = Args[1].ToLower();
        bool bShowChannel = StateStr == TEXT("on");
        
        EChatChannel FilterChannel;
        if (ChannelStr == TEXT("say")) FilterChannel = EChatChannel::Say;
        else if (ChannelStr == TEXT("shout")) FilterChannel = EChatChannel::Shout;
        else if (ChannelStr == TEXT("tell")) FilterChannel = EChatChannel::Tell;
        else if (ChannelStr == TEXT("group")) FilterChannel = EChatChannel::Group;
        else if (ChannelStr == TEXT("guild")) FilterChannel = EChatChannel::Guild;
        else if (ChannelStr == TEXT("trade")) FilterChannel = EChatChannel::Trade;
        else if (ChannelStr == TEXT("ooc")) FilterChannel = EChatChannel::OOC;
        else if (ChannelStr == TEXT("general")) FilterChannel = EChatChannel::General;
        else if (ChannelStr == TEXT("region")) FilterChannel = EChatChannel::Region;
        else
        {
            OutMessage = FString::Printf(TEXT("Unknown channel: %s"), *ChannelStr);
            OutChannel = EChatChannel::General;
            return true;
        }
        
        ToggleChannelVisibility(FilterChannel, bShowChannel);
        
        if (ChatTabs.IsValidIndex(ActiveTabIndex))
        {
            OutMessage = FString::Printf(TEXT("Channel %s is now %s in tab %s"), 
                *ChannelStr, bShowChannel ? TEXT("shown") : TEXT("hidden"), *ChatTabs[ActiveTabIndex]->TabName);
        }
        else
        {
            OutMessage = FString::Printf(TEXT("Channel %s is now %s"), 
                *ChannelStr, bShowChannel ? TEXT("shown") : TEXT("hidden"));
        }
        
        OutChannel = EChatChannel::General;
        return true;
    }
    else
    {
        // Unknown command, default to active channel
        OutMessage = Input;
        OutChannel = ActiveChannel;
    }

    return true;
}