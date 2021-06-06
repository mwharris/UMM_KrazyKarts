#include "KrazyKarts/Pawns/GoKart.h"
#include "Containers/Array.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("Movement Component"));
	ReplicationComponent = CreateDefaultSubobject<UGoKartReplicationComponent>(TEXT("Replication Component"));
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Tick both of our components
	MovementComponent->DoTick(DeltaTime);
	ReplicationComponent->DoTick(DeltaTime);
	// Display our replication Role for testing purposes
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

void AGoKart::MoveForward(float Val) 
{
	MovementComponent->SetThrottle(Val);
}

void AGoKart::MoveRight(float Val) 
{
	MovementComponent->SetSteeringThrow(Val);
}

FString AGoKart::GetEnumText(ENetRole ActorRole) 
{
	switch (ActorRole)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR";
	}
}