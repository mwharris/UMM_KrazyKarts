#include "KrazyKarts/Pawns/GoKart.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
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
    DOREPLIFETIME(AGoKart, ReplicatedTransform);
    DOREPLIFETIME(AGoKart, Velocity);
    DOREPLIFETIME(AGoKart, Throttle);
    DOREPLIFETIME(AGoKart, SteeringThrow);
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
	// Create our "driving force" by taking our input * driving force * forward
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	// Calculate and apply air resistance to our driving force
	Force += CalculateAirResistance();
	Force += CalculateRollingResistance();
	// Find Acceleration using F = M/A
	FVector Acceleration = Force / Mass;
	// Find Velocity based on our Acceleration and time traveled Dv = a * Dt
	Velocity += Acceleration * DeltaTime;
	// Perform movement and rotations
	UpdateLocationViaVelocity(DeltaTime);
	ApplyRotation(DeltaTime);
	// If we're the Server - update our replicated transform for the Clients
	if (HasAuthority()) 
	{
		ReplicatedTransform = GetActorTransform();
	}
	// Display our replication Role for testing purposes
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ReplicatedTransform() 
{
	SetActorTransform(ReplicatedTransform);
}

void AGoKart::ApplyRotation(float DeltaTime) 
{
	// dX - change in location along our turning circle over time
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	// dTheta = dX / R (change in angle around the circle over time)
	float dTheta = DeltaLocation / MinTurningRadius;
	// Rotation Angle (radians) = dTheta * User Input
	float RotationAngleRadians = dTheta * SteeringThrow;
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

void AGoKart::MoveForward(float Val) 
{
	Throttle = Val;
	Server_MoveForward(Val);	
}
void AGoKart::Server_MoveForward_Implementation(float Val) 
{
	Throttle = Val;
}
bool AGoKart::Server_MoveForward_Validate(float Val) 
{
	return FMath::Abs(Val) <= 1;
}

void AGoKart::MoveRight(float Val) 
{
	SteeringThrow = Val;
	Server_MoveRight(Val);
}
void AGoKart::Server_MoveRight_Implementation(float Val) 
{
	SteeringThrow = Val;
}
bool AGoKart::Server_MoveRight_Validate(float Val) 
{
	return FMath::Abs(Val) <= 1;
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