#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S_ItemInfo.h"
#include "ItemBase.generated.h"

UCLASS()
class LOTA_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItemBase();

	// Item information
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FS_ItemInfo ItemDetails;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Get item weight (including weight reduction if bag)
	UFUNCTION(BlueprintCallable, Category = "Item")
	float GetEffectiveWeight() const;
};