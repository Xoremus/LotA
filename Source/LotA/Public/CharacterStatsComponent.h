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

	// Only declare the function here
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetBaseCarryWeight() const;

protected:
	virtual void BeginPlay() override;
};