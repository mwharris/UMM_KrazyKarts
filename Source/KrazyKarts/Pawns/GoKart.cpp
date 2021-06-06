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

	GoKartMoveComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("Movement Component"));
	GoKartReplicateComponent = CreateDefaultSubobject<UGoKartReplicationComponent>(TEXT("Replication Component"));
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGoKart, ServerState);
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
	// Autonomous proxy - Clients controlling pawn
	if (GetLocalRole() == ROLE_AutonomousProxy) 
	{
		// Create a Move command
		FGoKartMove CurrentMove = GoKartMoveComponent->CreateMove(DeltaTime);
		// Add it to a list of moves that haven't yet been acknowledged by the Server
		UnacknowledgedMoves.Add(CurrentMove);
		// RPC to tell the Server we're moving
		Server_Move(CurrentMove);
		// Simulate our movement locally
		GoKartMoveComponent->SimulateMove(CurrentMove);
	}
	// Simulated proxy - some other connection's pawn
	else if (GetLocalRole() == ROLE_SimulatedProxy) 
	{
		GoKartMoveComponent->SimulateMove(ServerState.LastMove);
	}
	// Server controlling it's own pawn
	else if (GetLocalRole() == ROLE_Authority && IsLocallyControlled()) 
	{
		// Create a Move command
		FGoKartMove CurrentMove = GoKartMoveComponent->CreateMove(DeltaTime);
		// Simulate our movement locally
		Server_Move(CurrentMove);
	}
	// Display our replication Role for testing purposes
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

// Client - handle Server response
void AGoKart::OnRep_ServerState() 
{
	// Set out Transform (position/rotation) and Velocity
	SetActorTransform(ServerState.Transform);
	GoKartMoveComponent->SetVelocity(ServerState.Velocity);
	// Clear any moves from our queue that have now been acknowledged
	ClearAcknowledgedMoves(ServerState.LastMove);
	// Replay/simulate the moves that are still not acknowledged in order to sync up with the Server
	for (const FGoKartMove& UnacknowledgedMove : UnacknowledgedMoves) 
	{
		GoKartMoveComponent->SimulateMove(UnacknowledgedMove);
	}
}

// Server - Validate a Move command
bool AGoKart::Server_Move_Validate(FGoKartMove Move) 
{
	return true; // FMath::Abs(Move.Throttle) <= 1 && FMath::Abs(Move.SteeringThrow) <= 1;
}

// Server - Perform a Move command (extract data for processing in Tick())
void AGoKart::Server_Move_Implementation(FGoKartMove Move) 
{
	GoKartMoveComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = GoKartMoveComponent->GetVelocity();
}

void AGoKart::MoveForward(float Val) 
{
	GoKartMoveComponent->SetThrottle(Val);
}

void AGoKart::MoveRight(float Val) 
{
	GoKartMoveComponent->SetSteeringThrow(Val);
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove) 
{
	UnacknowledgedMoves.RemoveAll([&](const FGoKartMove& Move) {
		return Move.Timestamp <= LastMove.Timestamp;
	});
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