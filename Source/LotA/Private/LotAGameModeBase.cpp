#include "LotAGameModeBase.h"
#include "UObject/ConstructorHelpers.h"
#include "InventoryWidget.h"
#include "LotAPlayerController.h"
#include "PickupNotificationManager.h"

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

    // Try to find the notification manager blueprint class
    static ConstructorHelpers::FClassFinder<APickupNotificationManager> NotificationManagerBPClass(
        TEXT("/Game/UI/BP_PickupNotificationManager"));

    UE_LOG(LogTemp, Warning, TEXT("Searching for BP_PickupNotificationManager at path: %s"), 
        TEXT("/All/Game/UI/BP_PickupNotificationManager"));
    
    if (NotificationManagerBPClass.Succeeded())
    {
        NotificationManagerClass = NotificationManagerBPClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("Found NotificationManagerClass: %s"), 
            *NotificationManagerClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find BP_PickupNotificationManager at specified path"));
    }
}

void ALotAGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay - Default Pawn: %s"), *GetNameSafe(DefaultPawnClass));
    
    SpawnNotificationManager();
}

void ALotAGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
}

void ALotAGameModeBase::SpawnNotificationManager()
{
    // Only spawn if we have a valid class and haven't spawned yet
    if (NotificationManagerClass && !NotificationManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnNotificationManager: Attempting to spawn with class %s"), 
            *NotificationManagerClass->GetName());

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        NotificationManager = GetWorld()->SpawnActor<APickupNotificationManager>(
            NotificationManagerClass, 
            FVector(0), 
            FRotator(0), 
            SpawnParams);

        if (NotificationManager)
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnNotificationManager: Successfully spawned: %s"), 
                *NotificationManager->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnNotificationManager: Failed to spawn manager"));
        }
    }
    else
    {
        if (!NotificationManagerClass)
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnNotificationManager: NotificationManagerClass is not set!"));
        }
        if (NotificationManager)
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnNotificationManager: Manager already exists: %s"), 
                *NotificationManager->GetName());
        }
    }
}