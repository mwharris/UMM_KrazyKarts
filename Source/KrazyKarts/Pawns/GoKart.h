#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float Val);
	void MoveRight(float Val);

protected:
	virtual void BeginPlay() override;

private:
	// Mass of car in kg
	UPROPERTY(EditAnywhere)
	float Mass = 1000;
	// The force applied to the car when the throttle is down
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;
	// The amount of drag applied to the car when calculating Air Resistance (kg/m)
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;
	// The rolling resistance that our tires exert.
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015f;
	// The minimum radius of our turning circle at full turn (meters).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;

	FVector Velocity;	// m/s
	float Throttle = 0;
	float SteeringThrow = 0;

	void ApplyRotation(float DeltaTime);
	FVector CalculateAirResistance();
	FVector CalculateRollingResistance();
	void UpdateLocationViaVelocity(float DeltaTime);

};
