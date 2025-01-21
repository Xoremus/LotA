// PickupNotificationManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S_ItemInfo.h"
#include "PickupNotificationManager.generated.h"

UCLASS()
class LOTA_API APickupNotificationManager : public AActor
{
	GENERATED_BODY()

public:    
	APickupNotificationManager();

	virtual void BeginPlay() override;

	/** Show a new pickup notification */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ShowPickupNotification(const FS_ItemInfo& Item, int32 Quantity);

protected:
	// Widget class for notifications
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UPickupNotificationWidget> NotificationWidgetClass;

	UPROPERTY()
	TArray<class UPickupNotificationWidget*> ActiveNotifications;

	/** Maximum number of notifications to show at once */
	UPROPERTY(EditAnywhere, Category = "UI")
	int32 MaxNotifications = 3;

	/** Space between notifications */
	UPROPERTY(EditAnywhere, Category = "UI")
	float NotificationSpacing = 10.0f;

private:
	/** Update positions of all active notifications */
	void UpdateNotificationPositions();

	/** Log the current state for debugging */
	void LogDebugState() const;
};