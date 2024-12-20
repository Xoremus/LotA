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
       WBP_Inventory->AddTestItem(0, HealthPotion,20);  // Health potion in first slot
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

void UMainInventoryWidget::UpdateInventoryWeight()
{
    if (WBP_Inventory)
    {
        float TotalWeight = 0.0f;
        const TArray<UInventorySlotWidget*>& InventorySlots = WBP_Inventory->GetInventorySlots();
        
        // Calculate total weight
        for (const UInventorySlotWidget* SlotWidget : InventorySlots)
        {
            if (SlotWidget && SlotWidget->GetQuantity() > 0)
            {
                const FS_ItemInfo& ItemInfo = SlotWidget->GetItemInfo();
                TotalWeight += ItemInfo.Weight * SlotWidget->GetQuantity();
            }
        }

        // Update weight display
        UpdateWeightDisplay();
    }
}

void UMainInventoryWidget::UpdateWeightDisplay()
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

    // Apply movement penalties if overweight
    if (ALotACharacter* Character = Cast<ALotACharacter>(GetOwningPlayerPawn()))
    {
        UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
        if (MovementComp)
        {
            if (CurrentWeight > MaxWeight)
            {
                // Make the penalty more noticeable
                float WeightRatio = FMath::Clamp(CurrentWeight / MaxWeight, 1.0f, 2.0f);
                float SpeedMultiplier = 0.5f;  // Reduce to 50% speed when overweight
                MovementComp->MaxWalkSpeed = Character->BaseWalkSpeed * SpeedMultiplier;
    
                UE_LOG(LogTemp, Warning, TEXT("Overweight! Speed reduced from %f to %f"), 
                    Character->BaseWalkSpeed, MovementComp->MaxWalkSpeed);
            }
            else
            {
                // Reset to normal speed
                MovementComp->MaxWalkSpeed = Character->BaseWalkSpeed;
                UE_LOG(LogTemp, Warning, TEXT("Normal weight, speed reset to: %f"), MovementComp->MaxWalkSpeed);
            }
        }
    }
}

void UMainInventoryWidget::UpdateStatsDisplay()
{
   if (!StatsComponent || !StrengthText) return;

   // Update STR Text
   FString StrValue = FString::Printf(TEXT("%02d"), FMath::RoundToInt(StatsComponent->STR));
   StrengthText->SetText(FText::FromString(StrValue));
}