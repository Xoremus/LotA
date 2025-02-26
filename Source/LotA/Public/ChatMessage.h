// ChatMessage.h
#pragma once

#include "CoreMinimal.h"
#include "ChatMessage.generated.h"

UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	Say         UMETA(DisplayName = "Say"),
	Shout       UMETA(DisplayName = "Shout"),
	Tell        UMETA(DisplayName = "Tell"),
	Group       UMETA(DisplayName = "Group"),
	Guild       UMETA(DisplayName = "Guild"),
	Trade       UMETA(DisplayName = "Trade"),
	OOC         UMETA(DisplayName = "OOC"),
	General     UMETA(DisplayName = "General"),
	Region      UMETA(DisplayName = "Region")
};

USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString SenderName;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString Message;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	EChatChannel Channel;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadWrite, Category = "Chat")
	FString ReceiverName;  // For private messages

	FChatMessage()
		: Channel(EChatChannel::Say)
		, Timestamp(FDateTime::Now())
	{}
};