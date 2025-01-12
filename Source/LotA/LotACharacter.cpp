#include "LotACharacter.h"
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

//////////////////////////////////////////////////////////////////////////
// Constructor

ALotACharacter::ALotACharacter()
{
    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    BaseWalkSpeed = 600.f;
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    // We do NOT automatically rotate with camera yaw
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll  = false;

    // We do NOT orient to movement
    GetCharacterMovement()->bOrientRotationToMovement = false;

    // Camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.f;
    CameraBoom->bUsePawnControlRotation = true; // let the camera orbit

    // Follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Stats (optional)
    // StatsComponent = CreateDefaultSubobject<UCharacterStatsComponent>(TEXT("StatsComponent"));

    bRightMouseDown = false; // start false
}

void ALotACharacter::BeginPlay()
{
    Super::BeginPlay();

    // Add default mapping
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    RestoreAllBagStates();
}

//////////////////////////////////////////////////////////////////////////
// Input

void ALotACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Movement
        if (MoveAction)
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALotACharacter::Move);
        }
        // Look
        if (LookAction)
        {
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALotACharacter::Look);
        }
        // Jump
        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
        // Interact => E
        if (IA_Interact)
        {
            EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ALotACharacter::OnInteract);
        }
        // Right Mouse press/release
        if (IA_RightMouse)
        {
            EIC->BindAction(IA_RightMouse, ETriggerEvent::Started, this, &ALotACharacter::OnRightMousePressed);
            EIC->BindAction(IA_RightMouse, ETriggerEvent::Completed, this, &ALotACharacter::OnRightMouseReleased);
        }
    }
}

void ALotACharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALotACharacter, BagSaveData);
}

//////////////////////////////////////////////////////////////////////////
// Movement / Look

void ALotACharacter::Move(const FInputActionValue& Value)
{
    FVector2D Axis = Value.Get<FVector2D>();
    if (Controller)
    {
        FRotator Rot = Controller->GetControlRotation();
        FRotator Yaw(0, Rot.Yaw, 0);

        FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
        FVector Right   = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

        AddMovementInput(Forward, Axis.Y);
        AddMovementInput(Right,   Axis.X);
    }
}

void ALotACharacter::Look(const FInputActionValue& Value)
{
    // Only rotate character if right mouse is down
    if (!bRightMouseDown)
    {
        return;
    }

    FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

// Right Mouse => we turn the character with mouse
void ALotACharacter::OnRightMousePressed(const FInputActionValue& /*Value*/)
{
    bRightMouseDown = true;
    // Enable direct rotation
    bUseControllerRotationYaw = true;
}

// Release => revert
void ALotACharacter::OnRightMouseReleased(const FInputActionValue& /*Value*/)
{
    bRightMouseDown = false;
    bUseControllerRotationYaw = false;
}

//////////////////////////////////////////////////////////////////////////
// Interact => Press E => line trace => pick up item

void ALotACharacter::OnInteract()
{
    const float TraceDist = 200.f;
    FVector Start = FollowCamera->GetComponentLocation();
    FVector End   = Start + (FollowCamera->GetComponentRotation().Vector() * TraceDist);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (bHit && Hit.GetActor())
    {
        if (AItemBase* Item = Cast<AItemBase>(Hit.GetActor()))
        {
            ServerPickupItem(Item);
        }
    }
}

void ALotACharacter::ServerPickupItem_Implementation(AItemBase* ItemActor)
{
    if (!ItemActor || !IsValid(ItemActor) || ItemActor->IsActorBeingDestroyed())
    {
        return;
    }

    FS_ItemInfo Info = ItemActor->ItemDetails;
    int32 Qty        = ItemActor->StackCount;

    // Example main bag key:
    UBagComponent* MainBag = FindBagComponent(TEXT("Bag_MainInventory"));
    if (!MainBag)
    {
        UE_LOG(LogTemp, Warning, TEXT("No main bag => cannot pick up item"));
        return;
    }

    int32 SlotIndex = FindOrCreateSlotIndex(MainBag, Info, Qty);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("No available slot for item %s"), *Info.ItemName.ToString());
        return;
    }

    MainBag->TryAddItem(Info, Qty, SlotIndex);
    ItemActor->OnPickedUp(); // remove from world
}

int32 ALotACharacter::FindOrCreateSlotIndex(UBagComponent* Bag, const FS_ItemInfo& Item, int32 Quantity)
{
    const TArray<FBagSlotState>& Slots = Bag->GetSlotStates();
    int32 FirstEmpty = INDEX_NONE;

    for (int32 i = 0; i < Slots.Num(); i++)
    {
        const FBagSlotState& S = Slots[i];
        if (!S.IsEmpty() && S.ItemInfo.ItemID == Item.ItemID)
        {
            int32 Space = Item.MaxStackSize - S.Quantity;
            if (Space > 0)
            {
                return i; // partial stack
            }
        }
        else if (S.IsEmpty() && FirstEmpty == INDEX_NONE)
        {
            FirstEmpty = i;
        }
    }
    return FirstEmpty;
}

//////////////////////////////////////////////////////////////////////////
// Bag Logic

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
            Rebuild.BagKey    = BagKey;
            Rebuild.BagInfo   = S.BagInfo;
            Rebuild.SlotStates= S.SlotStates;
            NewBag->LoadState(Rebuild);
        }

        // bind weight changed
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
    NewState.BagKey     = BagKey;
    NewState.BagInfo    = BagComp->GetBagInfo();
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
        OutState.BagInfo   = S.BagInfo;
        OutState.SlotStates= S.SlotStates;
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

void ALotACharacter::OnBagWeightChanged(float /*NewWeight*/)
{
    UpdateBagWeights();
}

void ALotACharacter::UpdateBagWeights()
{
    // Recalc weight
    for (auto& Pair : ActiveBagComponents)
    {
        if (UBagComponent* Bag = Pair.Value)
        {
            Bag->UpdateWeight();
        }
    }
    float TW = GetTotalBagsWeight();
    OnTotalWeightChanged(TW);
}

FName ALotACharacter::GenerateBagKey(const FS_ItemInfo& BagInfo) const
{
    return *FString::Printf(TEXT("Bag_%s"), *BagInfo.ItemID.ToString());
}

bool ALotACharacter::IsBagKeyValid(const FName& BagKey) const
{
    return !BagKey.IsNone() && BagKey.ToString().StartsWith(TEXT("Bag_"));
}
