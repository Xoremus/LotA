// BagComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BagTypes.h"
#include "BagComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UBagComponent*, Bag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UBagComponent*, Bag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSlotUpdated, int32, SlotIndex, const FS_ItemInfo&, ItemInfo, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UBagComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UBagComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    // Basic Operations
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool OpenBag();
    
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void CloseBag();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void ForceClose();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void InitializeBag(const FS_ItemInfo& BagItemInfo);

    // Item Management
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool CanAcceptItem(const FS_ItemInfo& Item, int32 TargetSlot) const;

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool TryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot);

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool TryRemoveItem(int32 SlotIndex);

    // State Management
    void LoadState(const FBagState& State);
    void SaveState();
    void RequestWeightUpdate();
    void SetSuppressSave(bool bSuppress) { bSuppressSave = bSuppress; }
    float GetLastCalculatedWeight() const { return LastCalculatedWeight; }

    // Getters
    UFUNCTION(BlueprintPure, Category = "Bag")
    const FS_ItemInfo& GetBagInfo() const { return BagState.BagInfo; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    bool IsBagOpen() const { return bIsOpen; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    int32 GetBagSlots() const { return BagState.SlotStates.Num(); }

    UFUNCTION(BlueprintPure, Category = "Bag")
    const TArray<FBagSlotState>& GetSlotStates() const { return BagState.SlotStates; }

public:
    void UpdateWeight();
    void NotifySlotUpdated(int32 SlotIndex);

    // Delegates
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBagOpened OnBagOpened;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBagClosed OnBagClosed;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSlotUpdated OnSlotUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeightChanged OnWeightChanged;

protected:
    UPROPERTY(Replicated)
    FBagState BagState;

    UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
    bool bIsOpen;

    UFUNCTION()
    void OnRep_IsOpen();

private:
    bool bPendingRemoval;
    bool bIsSaving;
    bool bIsClosing;
    bool bIsUpdatingWeight;
    bool bSuppressSave;
    bool bPendingWeightUpdate;
    float LastCalculatedWeight;
    FTimerHandle SaveDebounceTimer;
    FTimerHandle WeightUpdateTimer;
};