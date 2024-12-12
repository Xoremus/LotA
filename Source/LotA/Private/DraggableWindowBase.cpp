#include "DraggableWindowBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UDraggableWindowBase::UDraggableWindowBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDraggableWindowBase::NativeConstruct()
{
	Super::NativeConstruct();
}

FReply UDraggableWindowBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		// Use TakeWidget() to pass a TSharedRef<SWidget> to DetectDrag
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UDraggableWindowBase::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* DragOperation = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	if (DragOperation)
	{
		DragOperation->DefaultDragVisual = this; // Use the current widget as the drag visual
		OutOperation = DragOperation;
	}
}
