#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KrazyKarts/Components/GoKartMovementComponent.h"
#include "KrazyKarts/Components/GoKartReplicationComponent.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGoKartMovementComponent* MovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UGoKartReplicationComponent* ReplicationComponent;

	AGoKart();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void MoveForward(float Val);
	void MoveRight(float Val);

protected:
	virtual void BeginPlay() override;

private:
	FString GetEnumText(ENetRole Role);

};
