#include "LotAGameModeBase.h"
#include "UObject/ConstructorHelpers.h"
#include "InventoryWidget.h"
#include "LotAPlayerController.h"

ALotAGameModeBase::ALotAGameModeBase()
{
    // Set the default pawn class to BP_ThirdPersonCharacter
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (PlayerPawnBPClass.Succeeded())
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }

    // Set the default player controller class
    PlayerControllerClass = ALotAPlayerController::StaticClass();

    // Find and set the inventory widget class
    static ConstructorHelpers::FClassFinder<UInventoryWidget> InventoryWidgetBPClass(TEXT("/Game/Inventory/Widgets/WBP_Inventory"));
    if (InventoryWidgetBPClass.Succeeded())
    {
        DefaultInventoryWidgetClass = InventoryWidgetBPClass.Class;
    }
}

void ALotAGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay - Default Pawn: %s"), *GetNameSafe(DefaultPawnClass));
}

void ALotAGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
}