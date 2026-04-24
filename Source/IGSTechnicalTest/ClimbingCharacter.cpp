// Fill out your copyright notice in the Description page of Project Settings.

#include "ClimbingCharacter.h"
#include "ClimbingMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets up movement component and top-down spring arm camera
AClimbingCharacter::AClimbingCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    ClimbingMovementComponent = CreateDefaultSubobject<UClimbingMovementComponent>(TEXT("ClimbingMovementComponent"));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    CameraBoom->TargetArmLength = 1500.0f;
    CameraBoom->SetRelativeRotation(FRotator(-65.0f, 0.0f, 0.0f));
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 10.0f;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
}

void AClimbingCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AClimbingCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Binds input actions
void AClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// Rotates camera 90 degrees left around pivot
void AClimbingCharacter::RotateCameraLeft()
{
    if (CameraBoom)
    {
        FRotator NewRotation = CameraBoom->GetComponentRotation();
        NewRotation.Yaw += 90.0f;
        CameraBoom->SetWorldRotation(NewRotation);
    }
}

// Rotates camera 90 degrees right around pivot
void AClimbingCharacter::RotateCameraRight()
{
    if (CameraBoom)
    {
        FRotator NewRotation = CameraBoom->GetComponentRotation();
        NewRotation.Yaw -= 90.0f;
        CameraBoom->SetWorldRotation(NewRotation);
    }
}

// Resets camera to default top-down angle
void AClimbingCharacter::ResetCameraRotation()
{
    if (CameraBoom)
    {
        FRotator NewRotation = CameraBoom->GetComponentRotation();
        NewRotation = FRotator(-65.0f, 0.0f, 0.0f);
        CameraBoom->SetWorldRotation(NewRotation);
    }
}