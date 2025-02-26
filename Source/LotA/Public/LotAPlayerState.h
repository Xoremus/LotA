// Fill out your copyright notice in the Description page of Project Settings.

// LotAPlayerState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LotAPlayerState.generated.h"

UCLASS()
class LOTA_API ALotAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ALotAPlayerState();

	// Player's display name
	UPROPERTY(Replicated)
	FString PlayerDisplayName;

	// Set player's display name (called from server)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
	void SetPlayerDisplayName(const FString& NewName);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called when player state is initialized
	virtual void BeginPlay() override;
};