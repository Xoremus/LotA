// DraggableWindowBase.cpp
#include "DraggableWindowBase.h"

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
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsDragging = true;
        
        // Get current widget position
        FVector2D CurrentPos = GetCachedGeometry().GetAbsolutePosition();
        // Get mouse position relative to widget
        FVector2D LocalMousePos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        
        // Store the offset from widget origin to click position
        DragOffset = LocalMousePos;
        
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
        // Get current mouse position
        FVector2D MousePos = InMouseEvent.GetScreenSpacePosition();
        
        // Calculate new position by subtracting the stored offset
        FVector2D NewPos = MousePos - DragOffset;
        
        SetPositionInViewport(NewPos, false);
        return FReply::Handled();
    }
    return FReply::Unhandled();
}