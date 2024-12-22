#include "CharacterStatsComponent.h"

UCharacterStatsComponent::UCharacterStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	STR = 10.0f;  // Default STR value
}

void UCharacterStatsComponent::BeginPlay()
{
	Super::BeginPlay();
}

// Define the function here
float UCharacterStatsComponent::GetBaseCarryWeight() const
{
	return STR;
}

//float UCharacterStatsComponent::GetModifiedCarryWeight() const
//{
	//float BaseWeight = GetBaseCarryWeight();
	// TODO: Apply modifiers from magic bags, buffs, etc.
	//return BaseWeight;
//}