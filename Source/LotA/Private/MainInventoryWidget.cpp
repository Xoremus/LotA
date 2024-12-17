// MainInventoryWidget.cpp
#include "MainInventoryWidget.h"
#include "InventoryWidget.h"
#include "InventorySlotWidget.h"
#include "S_ItemInfo.h"
#include "LotA/LotACharacter.h"
#include "CharacterStatsComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMainInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Get stats component from owning character
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (ALotACharacter* Character = Cast<ALotACharacter>(OwningPawn))
		{
			StatsComponent = Character->StatsComponent;
			UpdateStatsDisplay();  // Update stats when widget is constructed
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

float UMainInventoryWidget::CalculateTotalInventoryWeight() const
{
	float TotalWeight = 0.0f;

	if (WBP_Inventory)
	{
		const TArray<UInventorySlotWidget*>& InventorySlots = WBP_Inventory->GetInventorySlots();
		for (const UInventorySlotWidget* SlotWidget : InventorySlots)  // Changed variable names here
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

void UMainInventoryWidget::UpdateWeightDisplay()
{
    if (!WeightText || !StatsComponent) return;

    float CurrentWeight = CalculateTotalInventoryWeight();
    float MaxWeight = StatsComponent->GetBaseCarryWeight();
    
    // Update Weight Text
    FString WeightString = FString::Printf(TEXT("%.1f/%.1f"), CurrentWeight, MaxWeight);
    WeightText->SetText(FText::FromString(WeightString));

    // Apply movement penalties if overweight
    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (ACharacter* Character = Cast<ACharacter>(OwningPawn))
        {
            UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
            if (MovementComp)
            {
                if (CurrentWeight > MaxWeight)
                {
                    // Reduce movement speed when overweight
                    float WeightRatio = FMath::Clamp(CurrentWeight / MaxWeight, 1.0f, 2.0f);
                    float SpeedMultiplier = 1.0f / WeightRatio;
                    MovementComp->MaxWalkSpeed = 600.0f * SpeedMultiplier;  // Adjust base speed as needed
                }
                else
                {
                    // Reset to normal speed
                    MovementComp->MaxWalkSpeed = 600.0f;
                }
            }
        }
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
	TestBag.ItemIcon = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Inventory/Textures/T_StandardBagIcon"))); // Using same icon for now

	if (WBP_Inventory)
	{
		WBP_Inventory->AddTestItem(0, HealthPotion, 5);  // Health potion in first slot
		WBP_Inventory->AddTestItem(1, TestBag, 1);      // Bag in second slot
		UE_LOG(LogTemp, Warning, TEXT("Added potion and bag to inventory"));
	}
}

void UMainInventoryWidget::UpdateStatsDisplay()
{
	if (!StatsComponent || !StrengthText) return;

	// Update STR Text
	FString StrValue = FString::Printf(TEXT("%02d"), FMath::RoundToInt(StatsComponent->STR));
	StrengthText->SetText(FText::FromString(StrValue));
}