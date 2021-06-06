#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KrazyKarts/Components/GoKartMovementComponent.h"
#include "KrazyKarts/Components/GoKartReplicationComponent.h"
#include "GoKart.generated.h"

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGoKartMovementComponent* GoKartMoveComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGoKartReplicationComponent* GoKartReplicateComponent;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Move(FGoKartMove Move);

	AGoKart();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float Val);
	void MoveRight(float Val);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)	
	FGoKartState ServerState;

	TArray<FGoKartMove> UnacknowledgedMoves;

	UFUNCTION()
	void OnRep_ServerState();

	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	FString GetEnumText(ENetRole Role);

};
