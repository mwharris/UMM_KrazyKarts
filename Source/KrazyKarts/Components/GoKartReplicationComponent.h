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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Move(FGoKartMove Move);
	
	UGoKartReplicationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)	
	FGoKartState ServerState;
	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	TArray<FGoKartMove> UnacknowledgedMoves;
	
	UFUNCTION()
	void OnRep_ServerState();

	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	void UpdateServerState(const FGoKartMove& Move);
		
};