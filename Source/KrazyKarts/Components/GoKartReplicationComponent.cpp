
#include "KrazyKarts/Components/GoKartReplicationComponent.h"

UGoKartReplicationComponent::UGoKartReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGoKartReplicationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGoKartReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}