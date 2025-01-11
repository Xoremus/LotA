// DraggableWindowBase.cpp
#include "DraggableWindowBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UDraggableWindowBase::UDraggableWindowBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bIsDragging(false)
{
}

void UDraggableWindowBase::NativeConstruct()
{
    Super::NativeConstruct();
}

FReply UDraggableWindowBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TitleBar)
    {
        bIsDragging = true;
        DragOffset = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return FReply::Unhandled();
}

FReply UDraggableWindowBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
    {
        bIsDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply UDraggableWindowBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDragging)
    {
        FVector2D NewPosition = InMouseEvent.GetScreenSpacePosition() - DragOffset;
        SetPositionInViewport(NewPosition, false);
        return FReply::Handled();
    }
    return FReply::Unhandled();
}