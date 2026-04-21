// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingMovementComponent.generated.h"

UCLASS()
class IGSTECHNICALTEST_API UClimbingMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UClimbingMovementComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Climbing")
    void MoveToLocation(const FVector& TargetLocation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AcceptanceRadius = 50.0f;

private:
    UPROPERTY()
    class AClimbingCharacter* OwnerCharacter;

    bool bIsMoving = false;
    FVector TargetDestination;
};