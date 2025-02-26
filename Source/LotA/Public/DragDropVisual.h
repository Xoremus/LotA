#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "DragDropVisual.generated.h"

UCLASS()
class LOTA_API UDragDropVisual : public UUserWidget
{
	GENERATED_BODY()

public:
	UDragDropVisual(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DragDrop Visual")
	void SetItemIcon(UTexture2D* Icon);

	UFUNCTION(BlueprintCallable, Category = "DragDrop Visual")
	void SetQuantityText(int32 Quantity);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText;
};