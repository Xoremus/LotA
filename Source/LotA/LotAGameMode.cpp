// Copyright Epic Games, Inc. All Rights Reserved.

#include "LotAGameMode.h"
#include "LotACharacter.h"
#include "UObject/ConstructorHelpers.h"

ALotAGameMode::ALotAGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
