#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float SteeringThrow;
	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	float Timestamp;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMovementComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SimulateMove(const FGoKartMove& Move);

	FGoKartMove& GetLastMove();
	FVector GetVelocity() const;
	void SetVelocity(FVector NewVelocity);
	void SetThrottle(float Value);
	void SetSteeringThrow(float Value);

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

	FVector Velocity;
	float Throttle = 0;
	float SteeringThrow = 0;
	FGoKartMove LastMove;

	FGoKartMove CreateMove(float DeltaTime);
	FVector CalculateAirResistance();
	FVector CalculateRollingResistance();
	void UpdateLocationViaVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime, float MoveSteeringThrow);
		
};
