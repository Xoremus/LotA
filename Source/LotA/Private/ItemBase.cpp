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
    if (ALotACharacter* Character = Cast<ALotACharacter>(Interactor))
    {
        Character->ServerPickupItem(this);
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