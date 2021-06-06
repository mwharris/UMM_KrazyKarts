
#include "KrazyKarts/Components/GoKartReplicationComponent.h"
#include "Net/UnrealNetwork.h"

UGoKartReplicationComponent::UGoKartReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UGoKartReplicationComponent::BeginPlay()
{
	Super::BeginPlay();
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	if (MovementComponent == nullptr) 
	{
		UE_LOG(LogTemp, Error, TEXT("Replicator can't find Movement Component!"));
	}
}

void UGoKartReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UGoKartReplicationComponent, ServerState);
}

void UGoKartReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) 
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Get our owning Pawn for a check later
	auto ControlledPawn = Cast<APawn>(GetOwner());
	if (MovementComponent == nullptr || ControlledPawn == nullptr) return;
	// Autonomous proxy - Clients controlling pawn
	if (GetOwnerRole() == ROLE_AutonomousProxy) 
	{
		// Create a Move command
		FGoKartMove CurrentMove = MovementComponent->CreateMove(DeltaTime);
		// Add it to a list of moves that haven't yet been acknowledged by the Server
		UnacknowledgedMoves.Add(CurrentMove);
		// RPC to tell the Server we're moving
		Server_Move(CurrentMove);
		// Simulate our movement locally
		MovementComponent->SimulateMove(CurrentMove);
	}
	// Simulated proxy - some other connection's pawn
	else if (GetOwnerRole() == ROLE_SimulatedProxy) 
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
	// Server controlling it's own pawn
	else if (GetOwnerRole() == ROLE_Authority && ControlledPawn->IsLocallyControlled()) 
	{
		// Create a Move command
		FGoKartMove CurrentMove = MovementComponent->CreateMove(DeltaTime);
		// Simulate our movement locally
		Server_Move(CurrentMove);
	}
}

// Client - handle Server response
void UGoKartReplicationComponent::OnRep_ServerState() 
{
	if (MovementComponent == nullptr) return;
	// Set out Transform (position/rotation) and Velocity
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);
	// Clear any moves from our queue that have now been acknowledged
	ClearAcknowledgedMoves(ServerState.LastMove);
	// Replay/simulate the moves that are still not acknowledged in order to sync up with the Server
	for (const FGoKartMove& UnacknowledgedMove : UnacknowledgedMoves) 
	{
		MovementComponent->SimulateMove(UnacknowledgedMove);
	}
}

// Server - Validate a Move command
bool UGoKartReplicationComponent::Server_Move_Validate(FGoKartMove Move) 
{
	return true; // FMath::Abs(Move.Throttle) <= 1 && FMath::Abs(Move.SteeringThrow) <= 1;
}

// Server - Perform a Move command (extract data for processing in Tick())
void UGoKartReplicationComponent::Server_Move_Implementation(FGoKartMove Move) 
{
	if (MovementComponent == nullptr) return;
	MovementComponent->SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartReplicationComponent::ClearAcknowledgedMoves(FGoKartMove LastMove) 
{
	UnacknowledgedMoves.RemoveAll([&](const FGoKartMove& Move) {
		return Move.Timestamp <= LastMove.Timestamp;
	});
}