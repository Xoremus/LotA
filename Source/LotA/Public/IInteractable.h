#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInteractable.generated.h"

// This class does not need to be modified
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class LOTA_API IInteractable
{
	GENERATED_BODY()

public:
	// Called when a player attempts to interact with this object
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnInteract(AActor* Interactor);
	virtual void OnInteract_Implementation(AActor* Interactor) = 0;

	// Can this object be interacted with currently?
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const = 0;

	// Get interaction text to display (e.g., "Press E to Pick Up")
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	FText GetInteractionText(AActor* Interactor) const;
	virtual FText GetInteractionText_Implementation(AActor* Interactor) const = 0;
};