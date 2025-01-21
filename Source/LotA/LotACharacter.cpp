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
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // Movement settings
    BaseWalkSpeed = 600.f;
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    // Configure character rotation
    bUseControllerRotationYaw = false;    // Don't rotate with controller
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;     // Rotate character to movement direction
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // Smooth rotation speed
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;

    // Create camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;          // Further back for better view
    CameraBoom->bUsePawnControlRotation = true;    // Rotate arm based on controller
    CameraBoom->bDoCollisionTest = true;           // Camera boom collision
    CameraBoom->ProbeSize = 12.0f;
    CameraBoom->bEnableCameraLag = true;          // Smooth camera movement
    CameraBoom->CameraLagSpeed = 15.0f;
    CameraBoom->bEnableCameraRotationLag = false; // Disable rotation lag for responsive camera
    CameraBoom->bInheritPitch = true;             // Allow pitch inheritance
    CameraBoom->bInheritYaw = true;               // Allow yaw inheritance
    CameraBoom->bInheritRoll = false;             // No roll needed for MMO-style

    // Create follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false; // Camera doesn't rotate relative to arm

    // Create components
    StatsComponent = CreateDefaultSubobject<UCharacterStatsComponent>(TEXT("StatsComponent"));
    InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

    // Default camera pitch limits
    CameraPitchMin = -80.0f;
    CameraPitchMax = 0.0f;

    // Initialize movement state
    bIsAutoRunning = false;
    bIsRightMouseDown = false;
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

    // Restore saved bag states
    RestoreAllBagStates();
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
        // Movement
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALotACharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALotACharacter::Look);

        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Interaction - bind to InteractionComponent
        if (IA_Interact)
        {
            UE_LOG(LogTemp, Warning, TEXT("Binding Interact action"));
            EnhancedInputComponent->BindAction(IA_Interact, ETriggerEvent::Started, 
                InteractionComponent, &UInteractionComponent::TryInteract);
        }

        // Camera control
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
    
    // Get camera forward direction (ignoring pitch)
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    
    // Get forward and right vectors
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // Add movement inputs
    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);

    // Cancel auto-run if moving manually
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

void ALotACharacter::OnInteract()
{
    // This is kept for compatibility but delegates to InteractionComponent
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
    FBagState NewState;
    NewState.BagKey = BagKey;
    NewState.BagInfo = BagComp->GetBagInfo();
    NewState.SlotStates = BagComp->GetSlotStates();

    int32 Existing = BagSaveData.SavedBags.IndexOfByPredicate([BagKey](const FBagState& St){
        return St.BagKey == BagKey;
    });

    if (Existing != INDEX_NONE)
    {
        BagSaveData.SavedBags[Existing] = NewState;
        UE_LOG(LogTemp, Warning, TEXT("SaveBagState: Updated => %s"), *BagKey.ToString());
    }
    else
    {
        BagSaveData.SavedBags.Add(NewState);
        UE_LOG(LogTemp, Warning, TEXT("SaveBagState: Added => %s"), *BagKey.ToString());
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
    // This is kept for compatibility but delegates to InteractionComponent
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