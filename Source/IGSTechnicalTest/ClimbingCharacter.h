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
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UClimbingMovementComponent* ClimbingMovementComponent;
};
