#include "LotA/LotACharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"
#include "ItemBase.h"
#include "BagComponent.h"
#include "CharacterStatsComponent.h"
#include "InteractionComponent.h"

ALotACharacter::ALotACharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    BaseWalkSpeed = 600.f;
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    bUseControllerRotationYaw = false;    
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;     
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 12.0f;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 15.0f;
    CameraBoom->bEnableCameraRotationLag = false;
    CameraBoom->bInheritPitch = true;
    CameraBoom->bInheritYaw = true;
    CameraBoom->bInheritRoll = false;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    StatsComponent = CreateDefaultSubobject<UCharacterStatsComponent>(TEXT("StatsComponent"));
    InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

    CameraPitchMin = -80.0f;
    CameraPitchMax = 0.0f;

    bIsAutoRunning = false;
    bIsRightMouseDown = false;
}

void ALotACharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    RestoreAllBagStates();
    ValidateBagHierarchy();  // NEW: Validate hierarchy after restore
}

void ALotACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsAutoRunning)
    {
        AddMovementInput(AutoRunDirection);
    }
}

void ALotACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALotACharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALotACharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        if (IA_Interact)
        {
            UE_LOG(LogTemp, Warning, TEXT("Binding Interact action"));
            EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, 
                InteractionComponent, &UInteractionComponent::TryInteract);
        }

        if (IA_RightMouse)
        {
            EnhancedInputComponent->BindAction(IA_RightMouse, ETriggerEvent::Started, this, &ALotACharacter::OnRightMousePressed);
            EnhancedInputComponent->BindAction(IA_RightMouse, ETriggerEvent::Completed, this, &ALotACharacter::OnRightMouseReleased);
        }
    }
}

void ALotACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALotACharacter, BagSaveData);
}

void ALotACharacter::Move(const FInputActionValue& Value)
{
    if (Controller == nullptr)
        return;

    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);

    if (bIsAutoRunning && !FMath::IsNearlyZero(MovementVector.Size()))
    {
        bIsAutoRunning = false;
    }
}

void ALotACharacter::Look(const FInputActionValue& Value)
{
    if (!bIsRightMouseDown)
        return;

    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    
    AddControllerYawInput(LookAxisVector.X);
    AddControllerPitchInput(LookAxisVector.Y);
}

void ALotACharacter::ValidateBagHierarchy()
{
    TArray<FName> BagKeysToRemove;
    
    // Check each active bag
    for (const auto& Pair : ActiveBagComponents)
    {
        const FName& BagKey = Pair.Key;
        UBagComponent* BagComp = Pair.Value;
        
        if (!BagComp || !IsValid(BagComp))
        {
            BagKeysToRemove.Add(BagKey);
            continue;
        }

        // Check for circular references
        TSet<FName> VisitedKeys;
        if (CheckCircularReference(BagKey, BagKey, VisitedKeys))
        {
            UE_LOG(LogTemp, Warning, TEXT("Found circular reference in bag: %s"), *BagKey.ToString());
            BagKeysToRemove.Add(BagKey);
        }
    }

    // Remove any invalid bags
    for (const FName& KeyToRemove : BagKeysToRemove)
    {
        if (UBagComponent* BagToRemove = FindBagComponent(KeyToRemove))
        {
            UE_LOG(LogTemp, Warning, TEXT("Removing invalid bag: %s"), *KeyToRemove.ToString());
            BagToRemove->ForceClose();
            RemoveBagComponent(BagToRemove);
        }
    }
}

void ALotACharacter::OnInteract()
{
    if (InteractionComponent)
    {
        InteractionComponent->TryInteract();
    }
}

void ALotACharacter::OnRightMousePressed()
{
    bIsRightMouseDown = true;
    
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        PC->SetShowMouseCursor(false);
    }
}

void ALotACharacter::OnRightMouseReleased()
{
    bIsRightMouseDown = false;
    
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        PC->SetShowMouseCursor(true);
    }
}

UBagComponent* ALotACharacter::AddBagComponent(const FS_ItemInfo& BagInfo)
{
    if (BagInfo.ItemType != EItemType::Bag)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Tried to add non-bag => %s"), *BagInfo.ItemName.ToString());
        return nullptr;
    }

    FName BagKey = GenerateBagKey(BagInfo);
    if (UBagComponent* Existing = FindBagComponent(BagKey))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Found existing => %s"), *BagKey.ToString());
        int32 SavedIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& BS){
            return BS.BagKey == BagKey;
        });
        if (SavedIndex != INDEX_NONE)
        {
            Existing->LoadState(BagSaveData.SavedBags[SavedIndex]);
        }
        return Existing;
    }

    UBagComponent* NewBag = NewObject<UBagComponent>(this);
    if (NewBag)
    {
        NewBag->RegisterComponent();
        NewBag->InitializeBag(BagInfo);
        
        FBagSavedState S;
        if (GetSavedBagState(BagKey, S))
        {
            FBagState Rebuild;
            Rebuild.BagKey = BagKey;
            Rebuild.BagInfo = S.BagInfo;
            Rebuild.SlotStates = S.SlotStates;
            NewBag->LoadState(Rebuild);
        }

        NewBag->OnWeightChanged.AddDynamic(this, &ALotACharacter::OnBagWeightChanged);
        ActiveBagComponents.Add(BagKey, NewBag);
        
        UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Created => %s"), *BagKey.ToString());
    }
    return NewBag;
}

UBagComponent* ALotACharacter::FindBagByKey(const FName& BagKey) const
{
    return FindBagComponent(BagKey);
}

UBagComponent* ALotACharacter::FindBagComponent(const FName& BagKey) const
{
    if (const UBagComponent* const* Found = ActiveBagComponents.Find(BagKey))
    {
        return const_cast<UBagComponent*>(*Found);
    }
    return nullptr;
}

void ALotACharacter::SaveBagState_Implementation(UBagComponent* BagComp)
{
    if (!BagComp || !BagComp->GetBagInfo().ItemID.IsValid())
        return;

    FName BagKey = GenerateBagKey(BagComp->GetBagInfo());
    
    // Create new state
    FBagState NewState;
    NewState.BagKey = BagKey;
    NewState.BagInfo = BagComp->GetBagInfo();
    NewState.SlotStates = BagComp->GetSlotStates();

    // Log current state
    UE_LOG(LogTemp, Warning, TEXT("Saving bag state: %s"), *BagKey.ToString());
    for (int32 i = 0; i < NewState.SlotStates.Num(); ++i)
    {
        if (!NewState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s x%d"), 
                i, 
                *NewState.SlotStates[i].ItemInfo.ItemName.ToString(),
                NewState.SlotStates[i].Quantity);
        }
    }

    // Find and update or add
    int32 ExistingIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& State){
        return State.BagKey == BagKey;
    });

    if (ExistingIndex != INDEX_NONE)
    {
        BagSaveData.SavedBags[ExistingIndex] = NewState;
        UE_LOG(LogTemp, Warning, TEXT("SaveBagState: Updated => %s"), *BagKey.ToString());
    }
    else
    {
        BagSaveData.SavedBags.Add(NewState);
        UE_LOG(LogTemp, Warning, TEXT("SaveBagState: Added => %s"), *BagKey.ToString());
    }

    if (HasAuthority())
    {
        MarkPackageDirty();
    }
}

bool ALotACharacter::GetSavedBagState(const FName& BagKey, FBagSavedState& OutState) const
{
    if (!IsBagKeyValid(BagKey))
        return false;

    int32 FoundIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& St){
        return St.BagKey == BagKey;
    });

    if (FoundIndex != INDEX_NONE)
    {
        const FBagState& S = BagSaveData.SavedBags[FoundIndex];
        OutState.BagInfo = S.BagInfo;
        OutState.SlotStates = S.SlotStates;
        
        // Log what we're loading
        UE_LOG(LogTemp, Warning, TEXT("Loading bag state: %s"), *BagKey.ToString());
        for (int32 i = 0; i < OutState.SlotStates.Num(); ++i)
        {
            if (!OutState.SlotStates[i].IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s x%d"), 
                    i, 
                    *OutState.SlotStates[i].ItemInfo.ItemName.ToString(),
                    OutState.SlotStates[i].Quantity);
            }
        }
        return true;
    }
    return false;
}

float ALotACharacter::GetTotalBagsWeight() const
{
    return BagSaveData.GetTotalTopLevelWeight();
}

void ALotACharacter::RestoreAllBagStates()
{
    UE_LOG(LogTemp, Warning, TEXT("RestoreAllBagStates called"));
    
    TArray<FName> TopBags = BagSaveData.GetBagsAtDepth(0);
    for (const FName& BagK : TopBags)
    {
        int32 FoundIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagK](const FBagState& BS){
            return BS.BagKey == BagK;
        });
        
        if (FoundIndex != INDEX_NONE)
        {
            const FBagState& ST = BagSaveData.SavedBags[FoundIndex];
            AddBagComponent(ST.BagInfo);
        }
    }
    
    UpdateBagWeights();
}

void ALotACharacter::OnTotalWeightChanged(float NewTotalWeight)
{
    float Ratio = (NewTotalWeight / MaxCarryWeight);
    float SpeedMult = 1.f;
    
    if (Ratio > 1.f)
    {
        SpeedMult = FMath::Max(0.2f, 1.f - ((Ratio - 1.f) * 0.5f));
    }
    
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SpeedMult;
}

bool ALotACharacter::ValidateBagOperation(const FS_ItemInfo& BagInfo, FText& OutErrorMessage) const
{
    if (BagInfo.ItemType != EItemType::Bag)
    {
        OutErrorMessage = NSLOCTEXT("Inventory", "NotABag", "Item is not a bag");
        return false;
    }

    // Check if bag already exists
    FName BagKey = GenerateBagKey(BagInfo);
    if (UBagComponent* ExistingBag = FindBagComponent(BagKey))
    {
        // Only allow if bag is empty
        if (!ExistingBag->IsBagEmpty())
        {
            OutErrorMessage = NSLOCTEXT("Inventory", "BagNotEmpty", "Cannot move a bag that contains items");
            return false;
        }
    }

    // Check nesting depth
    TArray<FName> BagKeys;
    GetAllActiveBagKeys(BagKeys);
    if (BagKeys.Num() >= MaxBagNestingDepth)
    {
        OutErrorMessage = FText::Format(
            NSLOCTEXT("Inventory", "MaxNestingDepth", "Cannot nest more than {0} bags"),
            FText::AsNumber(MaxBagNestingDepth));
        return false;
    }

    return true;
}

bool ALotACharacter::HasCircularBagReference(const FName& BagKey, const FName& TargetBagKey) const
{
    if (!IsBagKeyValid(BagKey) || !IsBagKeyValid(TargetBagKey))
    {
        return false;
    }

    TSet<FName> VisitedKeys;
    return CheckCircularReference(BagKey, TargetBagKey, VisitedKeys);
}

void ALotACharacter::GetAllActiveBagKeys(TArray<FName>& OutBagKeys) const
{
    OutBagKeys.Empty();
    for (const auto& Pair : ActiveBagComponents)
    {
        OutBagKeys.Add(Pair.Key);
    }
}

void ALotACharacter::SaveAllBagsAndClose()
{
    UE_LOG(LogTemp, Warning, TEXT("Saving and closing all bags"));
    
    // Make a copy of the map keys since we'll be modifying the map
    TArray<FName> BagKeys;
    ActiveBagComponents.GenerateKeyArray(BagKeys);

    // Save and close each bag
    for (const FName& BagKey : BagKeys)
    {
        if (UBagComponent* BagComp = FindBagComponent(BagKey))
        {
            UE_LOG(LogTemp, Warning, TEXT("Saving and closing bag: %s"), *BagKey.ToString());
            BagComp->SaveState();
            BagComp->CloseBag();
        }
    }

    // Final save of overall bag state
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("Final save of bag state data"));
        SaveAllBagStates();
    }
}

void ALotACharacter::RemoveBagComponent(UBagComponent* BagComp)
{
    if (!BagComp) return;

    FName BagKey = GenerateBagKey(BagComp->GetBagInfo());
    if (UBagComponent** Found = ActiveBagComponents.Find(BagKey))
    {
        if (*Found == BagComp)
        {
            ActiveBagComponents.Remove(BagKey);
            UE_LOG(LogTemp, Warning, TEXT("RemoveBagComponent => %s"), *BagKey.ToString());
        }
    }
}

void ALotACharacter::ServerPickupItem_Implementation(AItemBase* ItemActor)
{
    if (InteractionComponent)
    {
        InteractionComponent->TryInteract();
    }
}

void ALotACharacter::OnBagWeightChanged(float NewWeight)
{
    UpdateBagWeights();
}

int32 ALotACharacter::FindOrCreateSlotIndex(UBagComponent* Bag, const FS_ItemInfo& Item, int32 Quantity)
{
    if (!Bag) return INDEX_NONE;

    const TArray<FBagSlotState>& Slots = Bag->GetSlotStates();
    int32 FirstEmpty = INDEX_NONE;

    // First look for partial stacks
    for (int32 i = 0; i < Slots.Num(); i++)
    {
        const FBagSlotState& S = Slots[i];
        if (!S.IsEmpty() && S.ItemInfo.ItemID == Item.ItemID)
        {
            int32 Space = Item.MaxStackSize - S.Quantity;
            if (Space > 0)
            {
                return i;
            }
        }
        else if (S.IsEmpty() && FirstEmpty == INDEX_NONE)
        {
            FirstEmpty = i;
        }
    }

    return FirstEmpty;
}

bool ALotACharacter::CheckCircularReference(const FName& StartBagKey, const FName& TargetBagKey, TSet<FName>& VisitedKeys) const
{
    // Guard against infinite recursion
    if (VisitedKeys.Contains(StartBagKey))
    {
        return false;
    }

    VisitedKeys.Add(StartBagKey);

    // Check if this is the target bag
    if (StartBagKey == TargetBagKey)
    {
        return true;
    }

    // Get the bag component
    if (UBagComponent* BagComp = FindBagComponent(StartBagKey))
    {
        // Check each bag in this bag's slots
        const TArray<FBagSlotState>& Slots = BagComp->GetSlotStates();
        for (const FBagSlotState& Slot : Slots)
        {
            if (!Slot.IsEmpty() && Slot.ItemInfo.ItemType == EItemType::Bag)
            {
                FName NestedBagKey = GenerateBagKey(Slot.ItemInfo);
                if (CheckCircularReference(NestedBagKey, TargetBagKey, VisitedKeys))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void ALotACharacter::UpdateBagWeights()
{
    for (auto& Pair : ActiveBagComponents)
    {
        if (UBagComponent* Bag = Pair.Value)
        {
            Bag->UpdateWeight();
        }
    }
    
    float TotalWeight = GetTotalBagsWeight();
    OnTotalWeightChanged(TotalWeight);
}

FName ALotACharacter::GenerateBagKey(const FS_ItemInfo& BagInfo) const
{
    return *FString::Printf(TEXT("Bag_%s"), *BagInfo.ItemID.ToString());
}

bool ALotACharacter::IsBagKeyValid(const FName& BagKey) const
{
    return !BagKey.IsNone() && BagKey.ToString().StartsWith(TEXT("Bag_"));
}

void ALotACharacter::SaveAllBagStates_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Server: Saving all bag states."));
	
    for (const auto& Pair : ActiveBagComponents)
    {
        if (UBagComponent* Bag = Pair.Value)
        {
            Bag->SaveState();
        }
    }
}