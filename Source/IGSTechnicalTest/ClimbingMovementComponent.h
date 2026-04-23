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
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AcceptanceRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
    bool bEnableSnap = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
    float SnapDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
    float SurfaceOffset = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
    float SnapRotationTolerance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
    float SnapPositionTolerance = 5.0f;

    UFUNCTION(BlueprintCallable)
    void MoveToLocation(const FVector& TargetLocation);

    UFUNCTION()
    bool SnapToSurface(FVector& InOutPosition, FRotator& OutRotation);

private:
    UPROPERTY(EditAnywhere, Category = "Climbing")
    float MaxTraceDistance = 3000.0f;

    UPROPERTY()
    class AClimbingCharacter* OwnerCharacter;

    UPROPERTY()
    class UPrimitiveComponent* CurrentClimbingSurface;

    bool bIsMoving = false;
    FVector TargetDestination;

    bool FindClimbingSurface(const FVector& FromPosition, FHitResult& OutHit);
    void NoSnapMovement(float DeltaTime);
    void CharacterIdleStateWithSnap();
    void CharacterMovingStateWithSnap(float DeltaTime);
};