#include "LotACharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"

ALotACharacter::ALotACharacter()
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // Set movement defaults
    BaseWalkSpeed = 600.f;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->JumpZVelocity = 600.f;
    GetCharacterMovement()->AirControl = 0.2f;
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

    // Configure character rotation
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Create camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bInheritPitch = true;
    CameraBoom->bInheritYaw = true;
    CameraBoom->bInheritRoll = false;

    // Create follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ALotACharacter::BeginPlay()
{
    Super::BeginPlay();

    // Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    RestoreAllBagStates();
}

void ALotACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALotACharacter, BagSaveData);
}

UBagComponent* ALotACharacter::AddBagComponent(const FS_ItemInfo& BagInfo)
{
    if (BagInfo.ItemType != EItemType::Bag)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Invalid item type for %s"), *BagInfo.ItemName.ToString());
        return nullptr;
    }

    FName BagKey = GenerateBagKey(BagInfo);
    
    // Check for existing bag component
    UBagComponent* Existing = FindBagComponent(BagKey);
    if (Existing && IsValid(Existing))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Using existing bag %s"), *BagKey.ToString());
        return Existing;
    }

    // Create new bag component
    UBagComponent* NewBag = NewObject<UBagComponent>(this);
    if (NewBag)
    {
        NewBag->RegisterComponent();
        NewBag->InitializeBag(BagInfo);

        // Look for saved state
        int32 FoundIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& State) {
            return State.BagKey == BagKey;
        });

        if (FoundIndex != INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("AddBagComponent: Loading saved state for %s"), *BagKey.ToString());
            NewBag->LoadState(BagSaveData.SavedBags[FoundIndex]);
        }

        ActiveBagComponents.Add(BagKey, NewBag);
        NewBag->OnWeightChanged.AddDynamic(this, &ALotACharacter::OnBagWeightChanged);
    }
    return NewBag;
}

bool ALotACharacter::GetSavedBagState(const FName& BagKey, FBagSavedState& OutState) const
{
    if (!IsBagKeyValid(BagKey))
    {
        UE_LOG(LogTemp, Warning, TEXT("GetSavedBagState: Invalid bag key %s"), *BagKey.ToString());
        return false;
    }

    int32 FoundIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& State) {
        return State.BagKey == BagKey;
    });

    if (FoundIndex != INDEX_NONE)
    {
        // Convert FBagState to legacy FBagSavedState
        const FBagState& State = BagSaveData.SavedBags[FoundIndex];
        OutState.SlotStates = State.SlotStates;
        OutState.BagInfo = State.BagInfo;
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("GetSavedBagState: No saved state found for %s"), *BagKey.ToString());
    return false;
}

void ALotACharacter::SaveBagState_Implementation(UBagComponent* BagComp)
{
    if (!BagComp || !BagComp->GetBagInfo().ItemID.IsValid())
        return;

    FName BagKey = GenerateBagKey(BagComp->GetBagInfo());
    
    FBagState NewState;
    NewState.BagKey = BagKey;
    NewState.BagInfo = BagComp->GetBagInfo();
    NewState.SlotStates = BagComp->GetSlotStates();  // Get current slot states

    // Log contents for debugging
    for (int32 i = 0; i < NewState.SlotStates.Num(); ++i)
    {
        if (!NewState.SlotStates[i].IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Saving item in slot %d: %s (x%d)"), 
                i, 
                *NewState.SlotStates[i].ItemInfo.ItemName.ToString(),
                NewState.SlotStates[i].Quantity);
        }
    }
    
    int32 ExistingIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& State) {
        return State.BagKey == BagKey;
    });

    if (ExistingIndex != INDEX_NONE)
    {
        BagSaveData.SavedBags[ExistingIndex] = NewState;
        UE_LOG(LogTemp, Warning, TEXT("Updated existing bag state: %s"), *BagKey.ToString());
    }
    else
    {
        BagSaveData.SavedBags.Add(NewState);
        UE_LOG(LogTemp, Warning, TEXT("Added new bag state: %s"), *BagKey.ToString());
    }
}

void ALotACharacter::UpdateBagWeights()
{
    for (const auto& Pair : ActiveBagComponents)
    {
        if (UBagComponent* Bag = Pair.Value)
        {
            Bag->UpdateWeight();
        }
    }

    float TotalWeight = GetTotalBagsWeight();
    OnTotalWeightChanged(TotalWeight);
}

void ALotACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALotACharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALotACharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    }
}

void ALotACharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void ALotACharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxisVector.X);
    AddControllerPitchInput(LookAxisVector.Y);
}

void ALotACharacter::OnBagWeightChanged(float NewWeight)
{
    UpdateBagWeights();
}

void ALotACharacter::OnTotalWeightChanged(float NewTotalWeight)
{
    float WeightRatio = NewTotalWeight / MaxCarryWeight;
    float SpeedMultiplier = 1.0f;

    if (WeightRatio > 1.0f)
    {
        SpeedMultiplier = FMath::Max(0.2f, 1.0f - ((WeightRatio - 1.0f) * 0.5f));
    }

    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SpeedMultiplier;
}

float ALotACharacter::GetTotalBagsWeight() const
{
    return BagSaveData.GetTotalTopLevelWeight();
}

FName ALotACharacter::GenerateBagKey(const FS_ItemInfo& BagInfo) const
{
    return *FString::Printf(TEXT("Bag_%s"), *BagInfo.ItemID.ToString());
}

bool ALotACharacter::IsBagKeyValid(const FName& BagKey) const
{
    return !BagKey.IsNone() && BagKey.ToString().StartsWith(TEXT("Bag_"));
}

UBagComponent* ALotACharacter::FindBagComponent(const FName& BagKey) const
{
    const UBagComponent* const* Found = ActiveBagComponents.Find(BagKey);
    return Found ? const_cast<UBagComponent*>(*Found) : nullptr;
}

UBagComponent* ALotACharacter::FindBagByKey(const FName& BagKey) const
{
    return FindBagComponent(BagKey);
}

void ALotACharacter::RestoreAllBagStates()
{
    UE_LOG(LogTemp, Warning, TEXT("Restoring all bag states..."));

    TArray<FName> TopLevelBags = BagSaveData.GetBagsAtDepth(0);
    for (const FName& BagKey : TopLevelBags)
    {
        int32 FoundIndex = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& State) {
            return State.BagKey == BagKey;
        });

        if (FoundIndex != INDEX_NONE)
        {
            const FBagState& State = BagSaveData.SavedBags[FoundIndex];
            AddBagComponent(State.BagInfo);
        }
    }

    UpdateBagWeights();
}