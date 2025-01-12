#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "S_ItemInfo.h"

// .generated.h must be last include
#include "ItemBase.generated.h"

/**
 * AItemBase
 * Represents an in-world pickup actor that can be collected by pressing E (line trace)
 * or optionally by overlap if you un-comment the overlap logic.
 */
UCLASS()
class LOTA_API AItemBase : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AItemBase();

    /** Item information (ID, weight, type, etc.) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FS_ItemInfo ItemDetails;

    /** If the item is stackable, how many units exist in this single actor? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
    int32 StackCount;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    /**
     * Mesh to visually represent this pickup in the world.
     * If you only do line-traces, you can keep this as the root.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Mesh")
    UStaticMeshComponent* ItemMesh;

    /**
     * Optional collision for overlap-based pickup.
     * If you rely purely on line traces + pressing E, you can leave or remove it.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item|Collision")
    USphereComponent* PickupCollision;

    /**
     * Returns the effective weight (including bag reduction if this item is a bag).
     */
    UFUNCTION(BlueprintCallable, Category="Item")
    float GetEffectiveWeight() const;

    /**
     * Called when the item is successfully picked up (on the server).
     * By default, we just Destroy() the actor.
     */
    UFUNCTION(BlueprintCallable, Category="Item")
    virtual void OnPickedUp();

    /**
     * If you want auto pickup by overlap, we declare it so no compile error occurs
     * if you call AddDynamic(...) in BeginPlay.
     */
    UFUNCTION()
    virtual void OnPickupOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );
};
