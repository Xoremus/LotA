#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "S_ItemInfo.h"
#include "IInteractable.h"
#include "ItemBase.generated.h"

UCLASS()
class LOTA_API AItemBase : public AActor, public IInteractable
{
    GENERATED_BODY()

public:    
    AItemBase();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

public:
    // IInteractable Interface Implementation
    virtual void OnInteract_Implementation(AActor* Interactor) override;
    virtual bool CanInteract_Implementation(AActor* Interactor) const override;
    virtual FText GetInteractionText_Implementation(AActor* Interactor) const override;

    /** Item information (ID, weight, type, etc.) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FS_ItemInfo ItemDetails;

    /** If the item is stackable, how many units exist in this single actor? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
    int32 StackCount;

    /** Mesh to visually represent this pickup in the world */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Mesh")
    UStaticMeshComponent* ItemMesh;

    /** Optional collision for overlap-based pickup */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Collision")
    USphereComponent* PickupCollision;

    /** Called when the item is successfully picked up (on the server) */
    UFUNCTION(BlueprintCallable, Category="Item")
    virtual void OnPickedUp();
};