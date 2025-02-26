// ChatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChatMessage.h"
#include "ChatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&, Message);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UChatComponent();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Chat")
	void ServerSendMessage(const FString& Message, EChatChannel Channel, const FString& TargetPlayer = "");

	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FChatMessage& Message);

	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FOnChatMessageReceived OnChatMessageReceived;

	// Command processing
	UFUNCTION(BlueprintCallable, Category = "Chat")
	bool ProcessChatCommand(const FString& Input, FString& OutMessage, EChatChannel& OutChannel, FString& OutTarget);

	// Chat range checks
	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsInRange(const AActor* Target, EChatChannel Channel) const;

protected:
	virtual void BeginPlay() override;

private:
	static const float SAY_RANGE;
	static const float SHOUT_RANGE;
	static const float OOC_RANGE;

	// Chat history for each channel
	TMap<EChatChannel, TArray<FChatMessage>> ChatHistory;

	// Command handlers
	UFUNCTION()
	bool HandleTellCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget);
	
	bool HandleGroupCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget);
	bool HandleGuildCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget);
	bool HandleEmoteCommand(const TArray<FString>& Args, FString& OutMessage, FString& OutTarget);
};