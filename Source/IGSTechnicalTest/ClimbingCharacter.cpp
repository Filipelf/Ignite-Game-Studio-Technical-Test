// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingCharacter.h"
#include "ClimbingMovementComponent.h"

AClimbingCharacter::AClimbingCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    ClimbingMovementComponent = CreateDefaultSubobject<UClimbingMovementComponent>(TEXT("ClimbingMovementComponent"));
}

void AClimbingCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AClimbingCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

