// DraggableWindowBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DraggableWindowBase.generated.h"

UCLASS()
class LOTA_API UDraggableWindowBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UDraggableWindowBase(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	bool bIsDragging;
	FVector2D DragOffset;
};