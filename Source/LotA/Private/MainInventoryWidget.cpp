// MainInventoryWidget.cpp
#include "MainInventoryWidget.h"
#include "InventoryWidget.h"
#include "S_ItemInfo.h"
#include "LotA/LotACharacter.h"
#include "InventorySlotWidget.h"
#include "CharacterStatsComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TextBlock.h"

void UMainInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Get stats component from owning character
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
        UpdateWeightDisplay();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidget binding failed in MainInventoryWidget!"));
    }
}

void UMainInventoryWidget::AddTestItems()
{
    if (!WBP_Inventory)
    {
        UE_LOG(LogTemp, Error, TEXT("WBP_Inventory is null"));
        return;
    }

    // Create test health potion
    FS_ItemInfo HealthPotion;
    HealthPotion.ItemID = FName("Item_HealthPotion");
    HealthPotion.ItemName = FText::FromString("Health Potion");
    HealthPotion.ItemType = EItemType::Consumable;
    HealthPotion.Weight = 0.5f;
    HealthPotion.MaxStackSize = 20;
    HealthPotion.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_HealthPotion")));

    // Create test bag
    FS_ItemInfo TestBag;
    TestBag.ItemID = FName("Item_SmallBag");
    TestBag.ItemName = FText::FromString("Small Bag");
    TestBag.ItemType = EItemType::Bag;
    TestBag.Weight = 1.0f;
    TestBag.MaxStackSize = 1;
    TestBag.BagSlots = 6;  // 6 slot bag
    TestBag.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_StandardBagIcon")));

    if (WBP_Inventory)
    {
        WBP_Inventory->AddTestItem(0, HealthPotion, 20);  // Health potion in first slot
        WBP_Inventory->AddTestItem(1, TestBag, 1);      // Bag in second slot
        UE_LOG(LogTemp, Warning, TEXT("Added potion and bag to inventory"));
    }
}

float UMainInventoryWidget::CalculateTotalInventoryWeight() const
{
    float TotalWeight = 0.0f;

    if (WBP_Inventory)
    {
        const TArray<UInventorySlotWidget*>& InventorySlots = WBP_Inventory->GetInventorySlots();
        
        UE_LOG(LogTemp, Warning, TEXT("Calculating total weight - Number of slots: %d"), InventorySlots.Num());
        
        for (const UInventorySlotWidget* SlotWidget : InventorySlots)
        {
            if (SlotWidget && SlotWidget->GetQuantity() > 0)
            {
                const FS_ItemInfo& ItemInfo = SlotWidget->GetItemInfo();
                float ItemWeight = ItemInfo.Weight * SlotWidget->GetQuantity();
                TotalWeight += ItemWeight;
                
                UE_LOG(LogTemp, Warning, TEXT("Slot weight: %.1f (Item: %s, Quantity: %d)"), 
                    ItemWeight, *ItemInfo.ItemName.ToString(), SlotWidget->GetQuantity());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Total calculated weight: %.1f"), TotalWeight);
    return TotalWeight;
}

void UMainInventoryWidget::UpdateInventoryWeight()
{
    if (!WeightText || !StatsComponent) return;

    float CurrentWeight = CalculateTotalInventoryWeight();
    float MaxWeight = StatsComponent->GetBaseCarryWeight();
    
    // Update Weight Text
    FString WeightString = FString::Printf(TEXT("%.1f/%.1f"), CurrentWeight, MaxWeight);
    WeightText->SetText(FText::FromString(WeightString));

    // Set text color based on weight
    if (CurrentWeight > MaxWeight)
    {
        WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f))); // Red color
    }
    else
    {
        WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))); // White color
    }

    // Update character movement speed
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
    {
        UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
        if (MovementComp)
        {
            float SpeedMultiplier = 1.0f;
            
            if (CurrentWeight > MaxWeight)
            {
                // Calculate how overweight we are
                float OverweightRatio = CurrentWeight / MaxWeight;
                
                // Apply a more aggressive speed reduction
                SpeedMultiplier = FMath::Max(0.25f, 1.0f - ((OverweightRatio - 1.0f) * 1.0f));

                // Ensure there's always some penalty when overweight
                SpeedMultiplier = FMath::Min(SpeedMultiplier, 0.9f);
                
                UE_LOG(LogTemp, Warning, TEXT("Applying speed penalty - Current Weight: %.1f, Max Weight: %.1f, Ratio: %.2f, Multiplier: %.2f"), 
                    CurrentWeight, MaxWeight, OverweightRatio, SpeedMultiplier);

                // Store base values if we haven't already
                if (!Character->IsValidLowLevel()) return;
                
                // Apply the speed changes directly to the movement component
                Character->GetCharacterMovement()->MaxWalkSpeed = Character->BaseWalkSpeed * SpeedMultiplier;
                Character->GetCharacterMovement()->GroundFriction = 2.0f * SpeedMultiplier; // Reduce ground friction too
                Character->GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f * SpeedMultiplier;
                
                UE_LOG(LogTemp, Warning, TEXT("Speed reduced from %.1f to %.1f"), 
                    Character->BaseWalkSpeed, MovementComp->MaxWalkSpeed);
            }
            else
            {
                // Reset to normal values
                Character->GetCharacterMovement()->MaxWalkSpeed = Character->BaseWalkSpeed;
                Character->GetCharacterMovement()->GroundFriction = 8.0f; // Default value
                Character->GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f; // Default value
                
                UE_LOG(LogTemp, Warning, TEXT("Speed reset to normal: %.1f"), MovementComp->MaxWalkSpeed);
            }

            // Force movement update
            Character->GetCharacterMovement()->UpdateComponentVelocity();
        }
    }
}

void UMainInventoryWidget::UpdateWeightDisplay()
{
    UpdateInventoryWeight();
}

void UMainInventoryWidget::UpdateStatsDisplay()
{
    if (!StatsComponent || !StrengthText) return;

    // Update STR Text
    FString StrValue = FString::Printf(TEXT("%02d"), FMath::RoundToInt(StatsComponent->STR));
    StrengthText->SetText(FText::FromString(StrValue));
}