#include "ItemBase.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AItemBase::BeginPlay()
{
	Super::BeginPlay();
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float AItemBase::GetEffectiveWeight() const
{
	if (ItemDetails.ItemType == EItemType::Bag && ItemDetails.WeightReductionPercentage > 0.0f)
	{
		return ItemDetails.Weight * (1.0f - (ItemDetails.WeightReductionPercentage / 100.0f));
	}

	return ItemDetails.Weight;
}
