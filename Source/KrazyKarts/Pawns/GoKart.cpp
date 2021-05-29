#include "KrazyKarts/Pawns/GoKart.h"

AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Create our "driving force" by taking our input * driving force * forward
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	// Find Acceleration using F = M/A
	FVector Acceleration = Force / Mass;
	// Find Velocity based on our Acceleration and time traveled Dv = a * Dt
	Velocity += Acceleration * DeltaTime;

	// Apply rotation and movement based on Velocity
	ApplyRotation(DeltaTime);
	UpdateLocationViaVelocity(DeltaTime);
}

void AGoKart::ApplyRotation(float DeltaTime) 
{
	// Degrees = User input * Degrees/s * Seconds
	float RotationAngleDegrees = SteeringThrow * MaxDegPerSecond * DeltaTime;
	// Build an FQuat for our rotation about the Up vector
	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngleDegrees));
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

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(float Val) 
{
	Throttle = Val;
}

void AGoKart::MoveRight(float Val) 
{
	SteeringThrow = Val;
}