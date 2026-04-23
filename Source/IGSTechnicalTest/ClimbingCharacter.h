// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ClimbingCharacter.generated.h"

UCLASS()
class IGSTECHNICALTEST_API AClimbingCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AClimbingCharacter();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UClimbingMovementComponent* ClimbingMovementComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void RotateCameraLeft();

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void RotateCameraRight();

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ResetCameraRotation();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
