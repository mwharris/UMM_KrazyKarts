#include "KrazyKarts/Components/GoKartMovementComponent.h"

UGoKartMovementComponent::UGoKartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime) 
{
	FGoKartMove CurrentMove;
	CurrentMove.Throttle = Throttle;
	CurrentMove.SteeringThrow = SteeringThrow;
	CurrentMove.DeltaTime = DeltaTime;
	CurrentMove.Timestamp = GetWorld()->GetTimeSeconds();
	return CurrentMove;
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move) 
{
	// Create our "driving force" by taking our input * driving force * forward
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
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

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float MoveSteeringThrow) 
{
	// dX - change in location along our turning circle over time
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * DeltaTime;
	// dTheta = dX / R (change in angle around the circle over time)
	float dTheta = DeltaLocation / MinTurningRadius;
	// Rotation Angle (radians) = dTheta * User Input
	float RotationAngleRadians = dTheta * MoveSteeringThrow;
	// Build an FQuat for our rotation about the Up vector
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngleRadians);
	// Rotate our Velocity vector by our rotation
	Velocity = RotationDelta.RotateVector(Velocity);
	// Apply our rotation
	GetOwner()->AddActorWorldRotation(RotationDelta);
}

void UGoKartMovementComponent::UpdateLocationViaVelocity(float DeltaTime) 
{
	// Translate our Velocity into a movement vector accounting for changing our m/s scale into cm/s Unreal units
	FVector Translation = (Velocity * DeltaTime * 100);
	// Move the car, sweeping for collisions
	FHitResult OutHit;
	GetOwner()->AddActorWorldOffset(Translation, true, &OutHit);
	// Check if we did have a collision
	if (OutHit.IsValidBlockingHit()) 
	{
		Velocity = FVector::ZeroVector;
	}
}

FVector UGoKartMovementComponent::CalculateAirResistance() 
{
	// Air Resistance = -Speed^2 * DragCoefficient
	return (-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient);
}

FVector UGoKartMovementComponent::CalculateRollingResistance() 
{
	// Get Unreal's Gravity variable and convert to meters
	float GravityAcceleration = -GetWorld()->GetGravityZ() / 100;
	// F = m * g
	float NormalForce = Mass * GravityAcceleration;
	// RollingResistance = RRCoefficient * NormalForce
	// -Velocity.GetSafeNormal() is here because the force is applied opposite of the Velocity
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

FVector UGoKartMovementComponent::GetVelocity() const
{
	return Velocity;
}

void UGoKartMovementComponent::SetVelocity(FVector NewVelocity) 
{
	Velocity = NewVelocity;	
}

void UGoKartMovementComponent::SetThrottle(float Value) 
{
	Throttle = Value;
}

void UGoKartMovementComponent::SetSteeringThrow(float Value) 
{
	SteeringThrow = Value;
}