// CharacterStatsComponent.cpp
#include "CharacterStatsComponent.h"

UCharacterStatsComponent::UCharacterStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	STR = 10.0f;  // Default strength
}

void UCharacterStatsComponent::BeginPlay()
{
	Super::BeginPlay();
}

float UCharacterStatsComponent::GetBaseCarryWeight() const 
{
	return STR * WEIGHT_PER_STRENGTH;
}

float UCharacterStatsComponent::GetModifiedCarryWeight() const
{
	float BaseWeight = GetBaseCarryWeight();
	// TODO: Apply modifiers from magic bags, buffs, etc.
	return BaseWeight;
}