// CharacterStatsComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStatsComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UCharacterStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UCharacterStatsComponent();

	// Basic Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float STR;

	// Weight Calculations
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBaseCarryWeight() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetModifiedCarryWeight() const;

protected:
	virtual void BeginPlay() override;

private:
	// Formula: Base carry weight = Strength * 10 (this gives a reasonable starting point)
	const float WEIGHT_PER_STRENGTH = 10.0f;
};