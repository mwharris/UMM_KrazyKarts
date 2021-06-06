
#include "KrazyKarts/Components/GoKartReplicationComponent.h"
#include "Net/UnrealNetwork.h"

UGoKartReplicationComponent::UGoKartReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

void UGoKartReplicationComponent::DoTick(float DeltaTime) 
{
	// Get our owning Pawn for a check later
	auto ControlledPawn = Cast<APawn>(GetOwner());
	if (MovementComponent == nullptr || ControlledPawn == nullptr) return;
	// Get our latest local Move from the Movement Component
	FGoKartMove LastMove = MovementComponent->GetLastMove();
	// Autonomous proxy - Clients controlling pawn
	if (GetOwnerRole() == ROLE_AutonomousProxy) 
	{
		// Add our latest move to a list of moves that haven't yet been acknowledged by the Server
		UnacknowledgedMoves.Add(LastMove);
		// RPC to tell the Server we're moving
		Server_Move(LastMove);
	}
	// Server controlling it's own pawn
	else if (GetOwnerRole() == ROLE_Authority && ControlledPawn->IsLocallyControlled()) 
	{
		// Simply update our ServerState - our local movement has already simulated via Movement Component
		UpdateServerState(LastMove);
	}
	// Simulated proxy (another connection's pawn)
	else if (GetOwnerRole() == ROLE_SimulatedProxy) 
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartReplicationComponent::ClientTick(float DeltaTime) 
{
	ClientTimeSinceLastUpdate += DeltaTime;
	// Safety check to ensure we're not using extremely small floating point numbers
	if (ClientTimeBetweenUpdates <= KINDA_SMALL_NUMBER) return; 
	// Build of parameters for our Lerp
	FVector StartLocation = ClientInterpStartLocation;
	FVector TargetLocation = ServerState.Transform.GetLocation();
	float Alpha = ClientTimeSinceLastUpdate / ClientTimeBetweenUpdates;
	// Lerp from start location to the latest ServerState location, over the time between our last 2 updates
	FVector NewLocation = FMath::LerpStable(StartLocation, TargetLocation, Alpha);
	GetOwner()->SetActorLocation(NewLocation);
	/*
	// Manually simulate the move we were sent
	MovementComponent->SimulateMove(ServerState.LastMove);
	*/
}

// Client - handle Server response
void UGoKartReplicationComponent::OnRep_ServerState() 
{
	if (GetOwnerRole() == ROLE_AutonomousProxy) 
	{
		OnRepServerState_AutonomousProxy();
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy) 
	{
		OnRepServerState_SimulatedProxy();
	}
}

void UGoKartReplicationComponent::OnRepServerState_SimulatedProxy() 
{
	ClientInterpStartLocation = GetOwner()->GetActorLocation();
	ClientTimeBetweenUpdates = ClientTimeSinceLastUpdate;
	ClientTimeSinceLastUpdate = 0;
}

void UGoKartReplicationComponent::OnRepServerState_AutonomousProxy() 
{
	if (MovementComponent == nullptr) return;
	// Set our Transform (position/rotation) and Velocity
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
	UpdateServerState(Move);
}

void UGoKartReplicationComponent::ClearAcknowledgedMoves(FGoKartMove LastMove) 
{
	UnacknowledgedMoves.RemoveAll([&](const FGoKartMove& Move) {
		return Move.Timestamp <= LastMove.Timestamp;
	});
}

void UGoKartReplicationComponent::UpdateServerState(const FGoKartMove& Move) 
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}