#include "ItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AItemBase::AItemBase()
{
    PrimaryActorTick.bCanEverTick = true;
    StackCount = 1; // default stack of 1

    // Create mesh
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    SetRootComponent(ItemMesh);

    // Create optional collision if you want overlap-based pickup
    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    PickupCollision->SetupAttachment(ItemMesh);
    PickupCollision->InitSphereRadius(50.f);
    PickupCollision->SetGenerateOverlapEvents(true);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
    PickupCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();

    // If you want overlap-based pickup, bind it here:
    if (PickupCollision)
    {
        PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnPickupOverlap);
    }
}

void AItemBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

float AItemBase::GetEffectiveWeight() const
{
    // If it's a bag with a WeightReductionPercentage, apply that
    if (ItemDetails.ItemType == EItemType::Bag && ItemDetails.WeightReductionPercentage > 0.0f)
    {
        return ItemDetails.Weight * (1.0f - (ItemDetails.WeightReductionPercentage / 100.0f));
    }
    return ItemDetails.Weight;
}

void AItemBase::OnPickedUp()
{
    // Called on the server after adding item to bag. 
    // Optionally play a sound/particle effect, then destroy:
    Destroy();
}

void AItemBase::OnPickupOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    // Example overlap logic if you want auto pickup:
    /*
    if (ALotACharacter* Character = Cast<ALotACharacter>(OtherActor))
    {
        // Call the server function to pick up
        Character->ServerPickupItem(this);
    }
    */
}
