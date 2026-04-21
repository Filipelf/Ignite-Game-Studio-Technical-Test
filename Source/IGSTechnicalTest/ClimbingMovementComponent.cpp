// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingMovementComponent.h"
#include "ClimbingCharacter.h"

UClimbingMovementComponent::UClimbingMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UClimbingMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<AClimbingCharacter>(GetOwner());
}

void UClimbingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsMoving || !OwnerCharacter)
        return;

    FVector CurrentLocation = OwnerCharacter->GetActorLocation();
    FVector Direction = (TargetDestination - CurrentLocation).GetSafeNormal();
    FVector NewLocation = CurrentLocation + Direction * MovementSpeed * DeltaTime;

    OwnerCharacter->SetActorLocation(NewLocation);

    float DistanceToTarget = FVector::Dist(NewLocation, TargetDestination);
    if (DistanceToTarget <= AcceptanceRadius)
    {
        bIsMoving = false;
        OwnerCharacter->SetActorLocation(TargetDestination);
    }
}

void UClimbingMovementComponent::MoveToLocation(const FVector& TargetLocation)
{
    if (!OwnerCharacter)
        return;

    TargetDestination = TargetLocation;
    bIsMoving = true;
}