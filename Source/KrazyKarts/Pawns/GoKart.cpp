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
	// If this instance is a Locally Controller player...
	if (IsLocallyControlled()) 
	{
		// Create a Move command for simulation
		FGoKartMove CurrentMove = CreateMove(DeltaTime);
		// Server doesn't need to maintain a Move list
		if (!HasAuthority())
		{
			UnacknowledgedMoves.Add(CurrentMove);
			UE_LOG(LogTemp, Warning, TEXT("Queue Length: %d"), UnacknowledgedMoves.Num());
		}
		// Client - Tell the Server to simulate our Move
		Server_Move(CurrentMove);
		// Simulate our own move (Server & Client)
		SimulateMove(CurrentMove);
	}
	// Server/Client - Simulate our Move
	// Display our replication Role for testing purposes
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

void AGoKart::SimulateMove(FGoKartMove Move) 
{
	// Create our "driving force" by taking our input * driving force * forward
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	// Calculate and apply air resistance to our driving force
	Force += CalculateAirResistance();
	Force += CalculateRollingResistance();
	// Find Acceleration using F = M/A
	FVector Acceleration = Force / Mass;
	// Find Velocity based on our Acceleration and time traveled Dv = a * Dt
	Velocity += Acceleration * Move.DeltaTime;
	// Perform movement and rotations
	UpdateLocationViaVelocity(Move.DeltaTime);
	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);
}

void AGoKart::ApplyRotation(float DeltaTime, float MoveSteeringThrow) 
{
	// dX - change in location along our turning circle over time
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	// dTheta = dX / R (change in angle around the circle over time)
	float dTheta = DeltaLocation / MinTurningRadius;
	// Rotation Angle (radians) = dTheta * User Input
	float RotationAngleRadians = dTheta * MoveSteeringThrow;
	// Build an FQuat for our rotation about the Up vector
	FQuat RotationDelta(GetActorUpVector(), RotationAngleRadians);
	// Rotate our Velocity vector by our rotation
	Velocity = RotationDelta.RotateVector(Velocity);
	// Apply our rotation
	AddActorWorldRotation(RotationDelta);
}

void AGoKart::UpdateLocationViaVelocity(float DeltaTime) 
{
	// Translate our Velocity into a movement vector accounting for changing our m/s scale into cm/s Unreal units
	FVector Translation = (Velocity * DeltaTime * 100);
	// Move the car, sweeping for collisions
	FHitResult OutHit;
	AddActorWorldOffset(Translation, true, &OutHit);
	// Check if we did have a collision
	if (OutHit.IsValidBlockingHit()) 
	{
		Velocity = FVector::ZeroVector;
	}
}

FVector AGoKart::CalculateAirResistance() 
{
	// Air Resistance = -Speed^2 * DragCoefficient
	return (-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector AGoKart::CalculateRollingResistance() 
{
	// Get Unreal's Gravity variable and convert to meters
	float GravityAcceleration = -GetWorld()->GetGravityZ() / 100;
	// F = m * g
	float NormalForce = Mass * GravityAcceleration;
	// RollingResistance = RRCoefficient * NormalForce
	// -Velocity.GetSafeNormal() is here because the force is applied opposite of the Velocity
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

// Client - set our Transform and Velocity from Server response
void AGoKart::OnRep_ServerState() 
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
	ClearAcknowledgedMoves(ServerState.LastMove);
}

// Server - Validate a Move command
bool AGoKart::Server_Move_Validate(FGoKartMove Move) 
{
	return true; // FMath::Abs(Move.Throttle) <= 1 && FMath::Abs(Move.SteeringThrow) <= 1;
}

// Server - Perform a Move command (extract data for processing in Tick())
void AGoKart::Server_Move_Implementation(FGoKartMove Move) 
{
	SimulateMove(Move);
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

void AGoKart::MoveForward(float Val) 
{
	Throttle = Val;
}

void AGoKart::MoveRight(float Val) 
{
	SteeringThrow = Val;
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove) 
{
	UnacknowledgedMoves.RemoveAll([&](const FGoKartMove& Move) {
		return Move.Timestamp <= LastMove.Timestamp;
	});
}

FGoKartMove AGoKart::CreateMove(float DeltaTime) 
{
	FGoKartMove CurrentMove;
	CurrentMove.Throttle = Throttle;
	CurrentMove.SteeringThrow = SteeringThrow;
	CurrentMove.DeltaTime = DeltaTime;
	CurrentMove.Timestamp = GetWorld()->GetTimeSeconds();
	return CurrentMove;
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