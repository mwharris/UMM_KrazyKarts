#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

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

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGoKartMove LastMove;	// The move that produced this state
	UPROPERTY()
	FTransform Transform;
	UPROPERTY()
	FVector Velocity;
};

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Move(FGoKartMove Move);

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

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)	
	FGoKartState ServerState;

	float Throttle = 0;
	float SteeringThrow = 0;
	FVector Velocity;
	TArray<FGoKartMove> UnacknowledgedMoves;

	UFUNCTION()
	void OnRep_ServerState();

	void SimulateMove(const FGoKartMove& Move);
	void ApplyRotation(float DeltaTime, float MoveSteeringThrow);
	FVector CalculateAirResistance();
	FVector CalculateRollingResistance();
	void UpdateLocationViaVelocity(float DeltaTime);
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	FGoKartMove CreateMove(float DeltaTime);
	FString GetEnumText(ENetRole Role);

};
