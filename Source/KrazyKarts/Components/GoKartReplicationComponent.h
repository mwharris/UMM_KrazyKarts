#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartReplicationComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartReplicationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
		
};