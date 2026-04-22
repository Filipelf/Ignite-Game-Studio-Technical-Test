// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingMovementComponent.h"
#include "ClimbingCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UClimbingMovementComponent::UClimbingMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UClimbingMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<AClimbingCharacter>(GetOwner());

    if (OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClimbingMovementComponent Initialized"));

        FVector Pos = OwnerCharacter->GetActorLocation();
        FRotator Rot = OwnerCharacter->GetActorRotation();

        FHitResult Hit;
        if (FindClimbingSurface(Pos, Hit))
        {
            CurrentClimbingSurface = Hit.GetComponent();

            if (SnapToSurface(Pos, Rot))
            {
                OwnerCharacter->SetActorLocationAndRotation(Pos, Rot);
                UE_LOG(LogTemp, Warning, TEXT("Initial Snap Applied"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No Surface Found at Spawn"));
        }
    }
}

bool UClimbingMovementComponent::FindClimbingSurface(const FVector& FromPosition, FHitResult& OutHit)
{
    if (!OwnerCharacter)
        return false;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OwnerCharacter->GetRootComponent()))
        QueryParams.AddIgnoredComponent(RootComp);

    QueryParams.bTraceComplex = true;

    FVector SphereCenter = FVector::ZeroVector;
    FVector DirToCenter = (SphereCenter - FromPosition).GetSafeNormal();

    FVector Start = FromPosition;
    FVector End = FromPosition + DirToCenter * MaxTraceDistance;

    DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 1.0f, 0, 2.0f);

    if (GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_WorldStatic, QueryParams))
    {
        DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Green, false, 0.1f);

        if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            CurrentClimbingSurface = OutHit.GetComponent();
            return true;
        }
    }

    FVector Start2 = FromPosition;
    FVector End2 = FromPosition - FVector::UpVector * MaxTraceDistance;

    DrawDebugLine(GetWorld(), Start2, End2, FColor::Cyan, false, 1.0f, 0, 2.0f);

    if (GetWorld()->LineTraceSingleByChannel(OutHit, Start2, End2, ECC_WorldStatic, QueryParams))
    {
        DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Blue, false, 0.1f);

        if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            CurrentClimbingSurface = OutHit.GetComponent();
            return true;
        }
    }

    return false;
}

bool UClimbingMovementComponent::SnapToSurface(FVector& InOutPosition, FRotator& OutRotation)
{
    if (!bEnableSnap || !OwnerCharacter) return false;

    if (InOutPosition.ContainsNaN())
    {
        UE_LOG(LogTemp, Error, TEXT("SnapToSurface got NaN position"));
        return false;
    }

    FHitResult Hit;
    if (FindClimbingSurface(InOutPosition, Hit))
    {
        CurrentClimbingSurface = Hit.GetComponent();

        FVector OutwardNormal = Hit.Normal;
        FVector SphereCenter = FVector::ZeroVector;
        FVector DirToCenter = (SphereCenter - Hit.Location).GetSafeNormal();

        if (FVector::DotProduct(OutwardNormal, DirToCenter) > 0)
        {
            OutwardNormal = -OutwardNormal;
        }

        FVector TargetPosition = Hit.Location + OutwardNormal * SurfaceOffset;

        float PositionDelta = FVector::Dist(InOutPosition, TargetPosition);
        if (PositionDelta > SnapPositionTolerance)
        {
            InOutPosition = TargetPosition;
        }

        FVector NewUp = OutwardNormal;
        FVector NewForward = OwnerCharacter->GetActorForwardVector();
        NewForward = FVector::VectorPlaneProject(NewForward, NewUp);

        FRotator TargetRotation;

        if (!NewForward.IsNearlyZero())
        {
            NewForward.Normalize();
            TargetRotation = UKismetMathLibrary::MakeRotFromXZ(NewForward, NewUp);
        }
        else
        {
            FVector WorldForward = FVector::ForwardVector;
            FVector NewRight = FVector::CrossProduct(NewUp, WorldForward).GetSafeNormal();
            NewForward = FVector::CrossProduct(NewRight, NewUp).GetSafeNormal();
            TargetRotation = UKismetMathLibrary::MakeRotFromXZ(NewForward, NewUp);
        }

        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        float DeltaTime = GetWorld()->GetDeltaSeconds();
        TargetRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 10.0f);

        FRotator RotDelta = (TargetRotation - CurrentRotation).GetNormalized();
        float RotDiff = FMath::Abs(RotDelta.Yaw) + FMath::Abs(RotDelta.Pitch) + FMath::Abs(RotDelta.Roll);

        if (RotDiff > SnapRotationTolerance)
        {
            OutRotation = TargetRotation;
        }
        else
        {
            OutRotation = CurrentRotation;
        }

        return true;
    }

    OutRotation = OwnerCharacter->GetActorRotation();
    return false;
}

void UClimbingMovementComponent::MoveToLocation(const FVector& TargetLocation)
{
    if (!OwnerCharacter) return;

    FHitResult Hit;

    FVector SphereCenter = FVector::ZeroVector;
    FVector DirFromCenter = (TargetLocation - SphereCenter).GetSafeNormal();

    FVector Start = TargetLocation + DirFromCenter * 500.0f;
    FVector End = TargetLocation - DirFromCenter * MaxTraceDistance;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OwnerCharacter->GetRootComponent()))
    {
        QueryParams.AddIgnoredComponent(RootComp);
    }
    QueryParams.bTraceComplex = true;

    DrawDebugLine(GetWorld(), Start, End, FColor::Magenta, false, 2.0f, 0, 3.0f);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams))
    {
        DrawDebugSphere(GetWorld(), Hit.Location, 25.0f, 12, FColor::Red, false, 2.0f);

        if (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            TargetDestination = Hit.Location + Hit.Normal * SurfaceOffset;
            bIsMoving = true;

            UE_LOG(LogTemp, Warning, TEXT("Moving to Valid Destination: %s"), *TargetDestination.ToString());
            return;
        }
    }

    if (FindClimbingSurface(TargetLocation, Hit))
    {
        TargetDestination = Hit.Location + Hit.Normal * SurfaceOffset;
        bIsMoving = true;
        UE_LOG(LogTemp, Warning, TEXT("Moving to Destination (Fallback): %s"), *TargetDestination.ToString());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Invalid Click. No Surface Found"));
}

void UClimbingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCharacter) return;

    if (!bIsMoving)
    {
        if (bEnableSnap)
        {
            FVector CurrentPos = OwnerCharacter->GetActorLocation();
            FRotator CurrentRot = OwnerCharacter->GetActorRotation();

            FVector SnappedPos = CurrentPos;
            FRotator SnappedRot = CurrentRot;

            if (SnapToSurface(SnappedPos, SnappedRot))
            {
                float PosDelta = FVector::Dist(CurrentPos, SnappedPos);

                FRotator RotDelta = (SnappedRot - CurrentRot).GetNormalized();
                float RotDiff = FMath::Abs(RotDelta.Yaw) + FMath::Abs(RotDelta.Pitch) + FMath::Abs(RotDelta.Roll);

                if (PosDelta > SnapPositionTolerance || RotDiff > SnapRotationTolerance)
                {
                    OwnerCharacter->SetActorLocationAndRotation(SnappedPos, SnappedRot);
                }
            }

            if (CurrentClimbingSurface)
            {
                FHitResult Hit;
                if (FindClimbingSurface(OwnerCharacter->GetActorLocation(), Hit))
                {
                    FVector CorrectPos = Hit.Location + Hit.Normal * SurfaceOffset;
                    float Dist = FVector::Dist(OwnerCharacter->GetActorLocation(), CorrectPos);

                    if (Dist > 5.0f)
                    {
                        FVector NewPos = FMath::VInterpTo(OwnerCharacter->GetActorLocation(), CorrectPos, DeltaTime, 5.0f);
                        OwnerCharacter->SetActorLocation(NewPos);
                    }
                }
            }
        }
        return;
    }

    FVector CurrentPos = OwnerCharacter->GetActorLocation();
    FVector DynamicTarget = TargetDestination;

    if (bEnableSnap)
    {
        FHitResult Hit;
        if (FindClimbingSurface(TargetDestination, Hit))
        {
            DynamicTarget = Hit.Location + Hit.Normal * SurfaceOffset;
        }
    }

    FVector Direction = (DynamicTarget - CurrentPos).GetSafeNormal();

    if (Direction.IsNearlyZero())
    {
        bIsMoving = false;
        UE_LOG(LogTemp, Warning, TEXT("Arrived at Destination (ZeroVector)"));
        return;
    }

    if (bEnableSnap)
    {
        FVector UpVector = OwnerCharacter->GetActorUpVector();
        Direction = FVector::VectorPlaneProject(Direction, UpVector);
        if (!Direction.IsNearlyZero())
        {
            Direction.Normalize();
        }
    }

    FVector NewLocation = CurrentPos + Direction * MovementSpeed * DeltaTime;
    OwnerCharacter->SetActorLocation(NewLocation);

    if (bEnableSnap)
    {
        FHitResult Hit;
        if (FindClimbingSurface(NewLocation, Hit))
        {
            FRotator NewRotation = OwnerCharacter->GetActorRotation();
            FVector NewUp = Hit.Normal;
            FVector NewForward = OwnerCharacter->GetActorForwardVector();
            NewForward = FVector::VectorPlaneProject(NewForward, NewUp);

            if (!NewForward.IsNearlyZero())
            {
                NewForward.Normalize();
                NewRotation = UKismetMathLibrary::MakeRotFromXZ(NewForward, NewUp);
                OwnerCharacter->SetActorRotation(NewRotation);
            }
        }
    }

    float DistanceToTarget = FVector::Dist(NewLocation, DynamicTarget);
    if (DistanceToTarget <= AcceptanceRadius)
    {
        bIsMoving = false;
        UE_LOG(LogTemp, Warning, TEXT("Arrived at Destination. Distance: %f"), DistanceToTarget);
    }
}