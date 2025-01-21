#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S_ItemInfo.h"
#include "PickupNotificationWidget.generated.h"

UCLASS()
class LOTA_API UPickupNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
    
	/** Setup the notification with item info */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void SetupNotification(const FS_ItemInfo& Item, int32 Quantity);

	/** Start the fade out animation */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void StartFadeOut();

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* QuantityText;

	// Animation properties
	UPROPERTY(EditAnywhere, Category = "Animation")
	float FadeOutDelay = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Animation")
	float FadeOutDuration = 0.5f;

private:
	FTimerHandle FadeTimer;
};