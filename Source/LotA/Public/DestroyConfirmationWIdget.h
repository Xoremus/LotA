#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "S_ItemInfo.h"
#include "DestroyConfirmationWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroyConfirmed, const FS_ItemInfo&, ItemToDestroy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDestroyCancelled);

UCLASS()
class LOTA_API UDestroyConfirmationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItemToDestroy(const FS_ItemInfo& Item, int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Destroy")
	FOnDestroyConfirmed OnDestroyConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Destroy")
	FOnDestroyCancelled OnDestroyCancelled;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* DestroyButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CancelButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ConfirmationText;

	UFUNCTION()
	void OnDestroyClicked();

	UFUNCTION()
	void OnCancelClicked();

private:
	FS_ItemInfo ItemInfo;
	int32 ItemAmount;
};