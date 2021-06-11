
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
	// Increase our ClientTimeSinceLastUpdate with our DeltaTime
	ClientTimeSinceLastUpdate += DeltaTime;
	// Safety check to ensure we're not using extremely small floating point numbers
	if (ClientTimeBetweenUpdates <= KINDA_SMALL_NUMBER) return; 
	// Create a Spline we can use with our cubic interpolations
	FHermiteCubicSpline Spline = CreateSpline();
	// Interpolate location, velocity, and rotation based on our latest received update
	float Alpha = ClientTimeSinceLastUpdate / ClientTimeBetweenUpdates;
	InterpolateLocation(Spline, Alpha);
	InterpolateVelocity(Spline, Alpha);
	InterpolateRotation(Alpha);
}

FHermiteCubicSpline UGoKartReplicationComponent::CreateSpline() 
{
	FHermiteCubicSpline Spline;
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	// Derivative = Velocity * TimeBetweenUpdates * (conversion from m [Velocity] to cm [Unreal unit location])
	float VelocityToDerivative = ClientTimeBetweenUpdates * 100;
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative;
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative;
	return Spline;
}

// CubicInterp from Start Location/Derivative to the latest ServerState Location/Derivative, over the time between our last 2 updates
void UGoKartReplicationComponent::InterpolateLocation(const FHermiteCubicSpline& Spline, float Alpha) 
{
	FVector NewLocation = Spline.InterpolateLocation(Alpha);
	if (MeshOffsetRoot != nullptr) 
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline& Spline, float Alpha) 
{
	float VelocityToDerivative = ClientTimeBetweenUpdates * 100;
	// CubicInterpDerivative to get the Derivative at our current Alpha point
	FVector Derivative = Spline.InterpolateDerivative(Alpha);
	// Convert to a Velocity (Derivative = Velocity * TimeBetweenUpdates * 100 => Velocity = Derivate / (TimeBetweenUpdates * 100))
	FVector NewVelocity = Derivative / VelocityToDerivative;
	// Set our Velocity
	MovementComponent->SetVelocity(NewVelocity);
}

// Slerp from Start Rotation to the latest ServerState Rotation, over the time between our last 2 updates
void UGoKartReplicationComponent::InterpolateRotation(float Alpha) 
{
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, Alpha);
	if (MeshOffsetRoot != nullptr) 
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
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
	if (MovementComponent == nullptr) return;
	if (MeshOffsetRoot != nullptr) 
	{
		ClientStartTransform = MeshOffsetRoot->GetComponentTransform();
	}
	ClientStartVelocity = MovementComponent->GetVelocity();
	ClientTimeBetweenUpdates = ClientTimeSinceLastUpdate;
	ClientTimeSinceLastUpdate = 0;
	GetOwner()->SetActorTransform(ServerState.Transform);
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
	float ClientProposedTime = ClientTime + Move.DeltaTime;
	bool ClientTimeValid = GetWorld()->TimeSeconds > ClientProposedTime; 
	return ClientTimeValid && Move.IsValid();
}

// Server - Perform a Move command (extract data for processing in Tick())
void UGoKartReplicationComponent::Server_Move_Implementation(FGoKartMove Move) 
{
	if (MovementComponent == nullptr) return;
	ClientTime += Move.DeltaTime;
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
	if (MeshOffsetRoot != nullptr) 
	{
		ServerState.Transform = MeshOffsetRoot->GetComponentTransform();
	}
	ServerState.Velocity = MovementComponent->GetVelocity();
}