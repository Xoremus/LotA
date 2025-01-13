#include "ItemBase.h"
#include "LotA/LotACharacter.h"

AItemBase::AItemBase()
{
    PrimaryActorTick.bCanEverTick = true;
    StackCount = 1;

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    SetRootComponent(ItemMesh);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(ItemMesh);
    PickupCollision->SetSphereRadius(50.f);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
    PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();
}

void AItemBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AItemBase::OnInteract_Implementation(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("ItemBase OnInteract called for %s"), *GetName());
    
    if (ALotACharacter* Character = Cast<ALotACharacter>(Interactor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Calling ServerPickupItem on character %s"), *Character->GetName());
        Character->ServerPickupItem(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Interactor is not a LotACharacter: %s"), 
            Interactor ? *Interactor->GetName() : TEXT("None"));
    }
}

bool AItemBase::CanInteract_Implementation(AActor* Interactor) const
{
    return IsValid(Interactor);
}

FText AItemBase::GetInteractionText_Implementation(AActor* Interactor) const
{
    return FText::Format(NSLOCTEXT("ItemBase", "InteractPrompt", "Press E to pick up {0}"), ItemDetails.ItemName);
}

void AItemBase::OnPickedUp()
{
    Destroy();
}