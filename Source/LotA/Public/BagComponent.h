// BagComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BagTypes.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "BagComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UBagComponent*, Bag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UBagComponent*, Bag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSlotUpdated, int32, SlotIndex, const FS_ItemInfo&, ItemInfo, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOperationFailed, const FText&, FailureReason);

USTRUCT()
struct FBagSaveStateInfo
{
    GENERATED_BODY()

    bool bIsSaving;
    uint32 SaveId;
    FDateTime LastSaveTime;
    
    FBagSaveStateInfo() : bIsSaving(false), SaveId(0) {}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOTA_API UBagComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UBagComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // === Basic Operations ===
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool OpenBag();
    
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void CloseBag();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void ForceClose();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void InitializeBag(const FS_ItemInfo& BagItemInfo);

    // === Item Management ===
    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool CanAcceptItem(const FS_ItemInfo& Item, int32 TargetSlot, FText& OutReason) const;

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool TryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot);

    UFUNCTION(BlueprintCallable, Category = "Bag")
    bool TryRemoveItem(int32 SlotIndex);

    // === State Management ===
    UFUNCTION(BlueprintCallable, Category = "Bag")
    void LoadState(const FBagState& State);

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void SaveState();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void RequestWeightUpdate();

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void SetSuppressSave(bool bSuppress) { bSuppressSave = bSuppress; }

    UFUNCTION(BlueprintCallable, Category = "Bag")
    void UpdateWeight();
    
    // === Getters ===
    UFUNCTION(BlueprintPure, Category = "Bag")
    float GetLastCalculatedWeight() const { return LastCalculatedWeight; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    float GetRecursiveWeight() const;

    UFUNCTION(BlueprintPure, Category = "Bag")
    bool HasCircularReference(const FName& BagKey) const;

    UFUNCTION(BlueprintPure, Category = "Bag")
    const FS_ItemInfo& GetBagInfo() const { return BagState.BagInfo; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    bool IsBagOpen() const { return bIsOpen; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    int32 GetBagSlots() const { return BagState.SlotStates.Num(); }

    UFUNCTION(BlueprintPure, Category = "Bag")
    const TArray<FBagSlotState>& GetSlotStates() const { return BagState.SlotStates; }

    UFUNCTION(BlueprintPure, Category = "Bag")
    bool IsBagEmpty() const;

    // === Server RPCs ===
    UFUNCTION(Server, Reliable)
    void ServerTryAddItem(const FS_ItemInfo& Item, int32 Quantity, int32 TargetSlot);

    UFUNCTION(Server, Reliable)
    void ServerTryRemoveItem(int32 SlotIndex);

    // === Events ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBagOpened OnBagOpened;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBagClosed OnBagClosed;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSlotUpdated OnSlotUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWeightChanged OnWeightChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBagOperationFailed OnBagOperationFailed;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_BagState)
    FBagState BagState;

    UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
    bool bIsOpen;

    UFUNCTION()
    void OnRep_BagState();

    UFUNCTION()
    void OnRep_IsOpen();

    void NotifySlotUpdated(int32 SlotIndex);

private:
    bool bPendingRemoval = false;
    bool bIsClosing = false;
    bool bIsUpdatingWeight = false;
    bool bSuppressSave = false;
    bool bPendingWeightUpdate = false;
    float LastCalculatedWeight = 0.0f;
    
    FTimerHandle SaveDebounceTimer;
    FTimerHandle WeightUpdateTimer;
    
    mutable TSet<FName> VisitedBags;
    
    FBagSaveStateInfo SaveStateInfo;
    static constexpr float SaveDebounceTime = 0.1f;
    static constexpr float MinSaveInterval = 0.05f;

    void CleanupTimers();
    bool ValidateItemOperation(const FS_ItemInfo& Item) const;
    bool CheckCircularReference(const FName& TargetBagKey, TSet<FName>& VisitedKeys) const;
    void LogValidationError(const FString& Message) const;
};