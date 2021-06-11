#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KrazyKarts/Components/GoKartMovementComponent.h"
#include "GoKartReplicationComponent.generated.h"

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

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;
	FVector InterpolateLocation(float Alpha) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
	}
	FVector InterpolateDerivative(float Alpha) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, Alpha);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Move(FGoKartMove Move);
	
	UGoKartReplicationComponent();
	void DoTick(float DeltaTime);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)	
	FGoKartState ServerState;
	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;
	UPROPERTY()
	USceneComponent* MeshOffsetRoot;

	TArray<FGoKartMove> UnacknowledgedMoves;
	float ClientTimeSinceLastUpdate = 0;
	float ClientTimeBetweenUpdates = 0;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;
	
	float ClientTime = 0;
	
	UFUNCTION()
	void OnRep_ServerState();
	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Val)
	{
		MeshOffsetRoot = Val;
	}
	
	void ClientTick(float DeltaTime);
	FHermiteCubicSpline CreateSpline();
	void InterpolateLocation(const FHermiteCubicSpline& Spline, float Alpha);
	void InterpolateVelocity(const FHermiteCubicSpline& Spline, float Alpha);
	void InterpolateRotation(float Alpha);
	void OnRepServerState_SimulatedProxy();
	void OnRepServerState_AutonomousProxy();
	void UpdateServerState(const FGoKartMove& Move);
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
		
};