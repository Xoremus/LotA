// Fill out your copyright notice in the Description page of Project Settings.

// LotAPlayerState.cpp
#include "LotAPlayerState.h"
#include "Net/UnrealNetwork.h"

ALotAPlayerState::ALotAPlayerState()
{
	// Initialize any variables here
	PlayerDisplayName = TEXT("Player");
}

void ALotAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALotAPlayerState, PlayerDisplayName);
}

void ALotAPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Set initial display name if not already set
	if (PlayerDisplayName.IsEmpty() && HasAuthority())
	{
		PlayerDisplayName = FString::Printf(TEXT("Player_%d"), GetPlayerId());
		// Force the replication
		ForceNetUpdate();
	}
}
void ALotAPlayerState::SetPlayerDisplayName_Implementation(const FString& NewName)
{
	PlayerDisplayName = NewName;
	// Log the name change for debugging
	UE_LOG(LogTemp, Warning, TEXT("Player name set to: %s"), *PlayerDisplayName);
}