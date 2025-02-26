// ChatComponent.cpp
#include "ChatComponent.h"

#include "LotAPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Actor.h"

const float UChatComponent::SAY_RANGE = 1000.0f;    // 10 meters
const float UChatComponent::SHOUT_RANGE = 3000.0f;  // 30 meters
const float UChatComponent::OOC_RANGE = 2000.0f;    // 20 meters

UChatComponent::UChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UChatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UChatComponent::ServerSendMessage_Implementation(const FString& Message, EChatChannel Channel, const FString& TargetPlayer)
{
    ALotAPlayerState* SenderState = GetOwner()->GetInstigatorController()->GetPlayerState<ALotAPlayerState>();
    if (!SenderState)
    {
        UE_LOG(LogTemp, Error, TEXT("ServerSendMessage: No PlayerState found"));
        return;
    }

    FChatMessage ChatMessage;
    ChatMessage.SenderName = FString::Printf(TEXT("Player_%d"), SenderState->GetPlayerId());  // Use PlayerId instead
    ChatMessage.Message = Message;
    ChatMessage.Channel = Channel;
    ChatMessage.ReceiverName = TargetPlayer;
    ChatMessage.Timestamp = FDateTime::Now();

    UE_LOG(LogTemp, Warning, TEXT("Created ChatMessage: From=%s, Message=%s"), *ChatMessage.SenderName, *ChatMessage.Message);

    // Find recipients based on channel and range
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC) continue;

        if (APawn* ReceiverPawn = PC->GetPawn())
        {
            if (UChatComponent* ReceiverChat = ReceiverPawn->FindComponentByClass<UChatComponent>())
            {
                ReceiverChat->ClientReceiveMessage(ChatMessage);
            }
        }
    }
}

void UChatComponent::ClientReceiveMessage_Implementation(const FChatMessage& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("ClientReceiveMessage: Message=%s"), *Message.Message);
    UE_LOG(LogTemp, Warning, TEXT("Is OnChatMessageReceived bound: %s"), 
        OnChatMessageReceived.IsBound() ? TEXT("Yes") : TEXT("No"));
    OnChatMessageReceived.Broadcast(Message);
}

bool UChatComponent::ProcessChatCommand(const FString& Input, FString& OutMessage, EChatChannel& OutChannel, FString& OutTarget)
{
    if (Input.IsEmpty()) return false;

    // Default to Say channel if no command
    if (!Input.StartsWith("/"))
    {
        OutMessage = Input;
        OutChannel = EChatChannel::Say;
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
    }
    else if (Command == TEXT("/sh") || Command == TEXT("/shout"))
    {
        OutChannel = EChatChannel::Shout;
    }
    else if (Command == TEXT("/t") || Command == TEXT("/tell") || Command == TEXT("/w") || Command == TEXT("/whisper"))
    {
        if (Args.Num() < 2) return false;  // Need target and message
        OutTarget = Args[0];
        Args.RemoveAt(0);  // Remove target name
        OutMessage = FString::Join(Args, TEXT(" "));
        OutChannel = EChatChannel::Tell;
    }
    else if (Command == TEXT("/g") || Command == TEXT("/group"))
    {
        OutChannel = EChatChannel::Group;
    }
    else if (Command == TEXT("/gu") || Command == TEXT("/guild"))
    {
        OutChannel = EChatChannel::Guild;
    }
    else if (Command == TEXT("/ooc"))
    {
        OutChannel = EChatChannel::OOC;
    }
    else if (Command == TEXT("/tr") || Command == TEXT("/trade"))
    {
        OutChannel = EChatChannel::Trade;
    }
    else
    {
        // Unknown command, default to Say
        OutMessage = Input;
        OutChannel = EChatChannel::Say;
    }

    return true;
}

bool UChatComponent::IsInRange(const AActor* Target, EChatChannel Channel) const
{
    if (!Target || !GetOwner()) return false;

    float Range;
    switch (Channel)
    {
        case EChatChannel::Say:
            Range = SAY_RANGE;
            break;
        case EChatChannel::Shout:
            Range = SHOUT_RANGE;
            break;
        case EChatChannel::OOC:
            Range = OOC_RANGE;
            break;
        default:
            return true;
    }

    return FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation()) <= Range;
}

bool UChatComponent::HandleTellCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget)
{
    if (Args.Num() < 2) return false;

    OutTarget = Args[0];
    // Create a new array starting from the second element
    TArray<FString> MessageParts;
    for (int32 i = 1; i < Args.Num(); i++)
    {
        MessageParts.Add(Args[i]);
    }
    OutMessage = FString::Join(MessageParts, TEXT(" "));
    return true;
}

bool UChatComponent::HandleGroupCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget)
{
    if (Args.Num() < 1) return false;

    OutMessage = FString::Join(Args, TEXT(" "));
    OutTarget = ""; // Group messages don't need a target
    return true;
}

bool UChatComponent::HandleGuildCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget)
{
    if (Args.Num() < 1) return false;

    OutMessage = FString::Join(Args, TEXT(" "));
    OutTarget = ""; // Guild messages don't need a target
    return true;
}

bool UChatComponent::HandleEmoteCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget)
{
    if (Args.Num() < 1) return false;

    // Get the player's name
    APlayerState* SenderState = GetOwner()->GetInstigatorController()->PlayerState;
    if (!SenderState) return false;

    // Format: "PlayerName <emote text>"
    OutMessage = FString::Printf(TEXT("%s %s"), *SenderState->GetPlayerName(), *FString::Join(Args, TEXT(" ")));
    OutTarget = ""; // Emotes don't need a target
    return true;
}