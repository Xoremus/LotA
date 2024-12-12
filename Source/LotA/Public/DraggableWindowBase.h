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

	// Handles dragging of the window
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
};

