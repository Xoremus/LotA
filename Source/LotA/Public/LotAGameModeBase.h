#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LotAGameModeBase.generated.h"

UCLASS()
class LOTA_API ALotAGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALotAGameModeBase();

protected:
	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

private:
	// Inventory widget reference
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UInventoryWidget> DefaultInventoryWidgetClass;
};
