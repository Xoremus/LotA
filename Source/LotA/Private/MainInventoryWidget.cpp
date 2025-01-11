// MainInventoryWidget.cpp
#include "MainInventoryWidget.h"
#include "GameFramework/PlayerController.h"
#include "LotA/LotACharacter.h"
#include "InventorySlotWidget.h"

void UMainInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LastTotalWeight = 0.0f;
    bWeightUpdatePending = false;

    if (ExitButton)
    {
        ExitButton->OnClicked.AddDynamic(this, &UMainInventoryWidget::OnExitButtonClicked);
    }

    if (DoneButton)
    {
        DoneButton->OnClicked.AddDynamic(this, &UMainInventoryWidget::OnDoneButtonClicked);
    }

    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (ALotACharacter* Character = Cast<ALotACharacter>(OwningPawn))
        {
            StatsComponent = Character->StatsComponent;
            UpdateStatsDisplay();
        }
    }

    if (WBP_Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryWidget successfully bound in MainInventoryWidget."));
        AddTestItems();
        UpdateInventoryWeight();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidget binding failed in MainInventoryWidget!"));
    }
}

void UMainInventoryWidget::OnDoneButtonClicked()
{
    SetGameOnlyMode();
}

void UMainInventoryWidget::OnExitButtonClicked()
{
    SetGameOnlyMode();
}

void UMainInventoryWidget::SetGameOnlyMode()
{
    SetVisibility(ESlateVisibility::Hidden);
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void UMainInventoryWidget::RequestWeightUpdate()
{
    if (bWeightUpdatePending)
        return;

    bWeightUpdatePending = true;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(WeightUpdateTimer,
            [this]()
            {
                UpdateInventoryWeight();
                bWeightUpdatePending = false;
            },
            0.1f, false);
    }
}

void UMainInventoryWidget::UpdateInventoryWeight()
{
    if (!WeightText || !StatsComponent) return;

    float CurrentWeight = CalculateTotalInventoryWeight();
    float MaxWeight = StatsComponent->GetBaseCarryWeight();

    // Only update if weight actually changed
    if (!FMath::IsNearlyEqual(LastTotalWeight, CurrentWeight))
    {
        LastTotalWeight = CurrentWeight;
        FString WeightString = FString::Printf(TEXT("%.1f/%.1f"), CurrentWeight, MaxWeight);
        WeightText->SetText(FText::FromString(WeightString));

        if (CurrentWeight > MaxWeight)
        {
            WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f)));

            if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
            {
                if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
                {
                    float OverweightRatio = CurrentWeight / MaxWeight;
                    float SpeedMultiplier = FMath::Clamp(1.0f - ((OverweightRatio - 1.0f) * 1.0f), 0.25f, 0.9f);
                    MovementComp->MaxWalkSpeed = Character->BaseWalkSpeed * SpeedMultiplier;
                }
            }
        }
        else
        {
            WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));

            if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
            {
                if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
                {
                    MovementComp->MaxWalkSpeed = Character->BaseWalkSpeed;
                }
            }
        }
    }
}

void UMainInventoryWidget::UpdateStatsDisplay()
{
    if (!StatsComponent || !StrengthText) return;
    FString StrValue = FString::Printf(TEXT("%02d"), FMath::RoundToInt(StatsComponent->STR));
    StrengthText->SetText(FText::FromString(StrValue));
}

float UMainInventoryWidget::CalculateTotalInventoryWeight() const
{
    float TotalWeight = 0.0f;

    if (WBP_Inventory)
    {
        const TArray<UInventorySlotWidget*>& InventorySlots = WBP_Inventory->GetInventorySlots();
        
        for (const UInventorySlotWidget* SlotWidget : InventorySlots)
        {
            if (SlotWidget && SlotWidget->GetQuantity() > 0)
            {
                const FS_ItemInfo& ItemInfo = SlotWidget->GetItemInfo();
                TotalWeight += ItemInfo.Weight * SlotWidget->GetQuantity();
            }
        }
    }

    return TotalWeight;
}

void UMainInventoryWidget::AddTestItems()
{
    if (!WBP_Inventory)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Inventory is null"));
        return;
    }

    FS_ItemInfo HealthPotion;
    HealthPotion.ItemID = FName("Item_HealthPotion");
    HealthPotion.ItemName = FText::FromString("Health Potion");
    HealthPotion.ItemType = EItemType::Consumable;
    HealthPotion.Weight = 0.5f;
    HealthPotion.MaxStackSize = 20;
    HealthPotion.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_HealthPotion")));

    FS_ItemInfo TestBag;
    TestBag.ItemID = FName("Item_SmallBag");
    TestBag.ItemName = FText::FromString("Small Bag");
    TestBag.ItemType = EItemType::Bag;
    TestBag.Weight = 1.0f;
    TestBag.MaxStackSize = 1;
    TestBag.BagSlots = 6;
    TestBag.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_StandardBagIcon")));

    WBP_Inventory->AddTestItem(0, HealthPotion, 20);
    WBP_Inventory->AddTestItem(1, TestBag, 1);
    UE_LOG(LogTemp, Warning, TEXT("Added potion and bag to inventory"));
}