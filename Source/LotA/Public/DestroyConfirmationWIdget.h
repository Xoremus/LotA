// DestroyConfirmationWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "S_ItemInfo.h"
#include "DestroyConfirmationWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroyConfirmed, const FS_ItemInfo&, ItemToDestroy);

UCLASS()
class LOTA_API UDestroyConfirmationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItemToDestroy(const FS_ItemInfo& Item, int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Destroy")
	FOnDestroyConfirmed OnDestroyConfirmed;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* DestroyButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CancelButton;

	UFUNCTION()
	void OnDestroyClicked();

	UFUNCTION()
	void OnCancelClicked();

private:
	FS_ItemInfo ItemInfo;
	int32 ItemAmount;
};