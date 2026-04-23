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
            UE_LOG(LogTemp, Error, TEXT("No Surface Found at Spawn"));
    }
}

void UClimbingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCharacter)
        return;

    if (!bEnableSnap)
    {
        if (bIsMoving)
        {
            FVector CurrentPos = OwnerCharacter->GetActorLocation();
            FVector Direction = (TargetDestination - CurrentPos).GetSafeNormal();

            if (Direction.IsNearlyZero())
            {
                bIsMoving = false;
                return;
            }

            FVector NewLocation = CurrentPos + Direction * MovementSpeed * DeltaTime;
            OwnerCharacter->SetActorLocation(NewLocation);

            if (FVector::Dist(NewLocation, TargetDestination) <= AcceptanceRadius)
            {
                bIsMoving = false;
            }
        }
        return;
    }

    if (!bIsMoving)
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
                OwnerCharacter->SetActorLocationAndRotation(SnappedPos, SnappedRot);
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

        return;
    }

    FVector DynamicTarget = TargetDestination;
    FHitResult Hit;

    if (FindClimbingSurface(TargetDestination, Hit))
        DynamicTarget = Hit.Location + Hit.Normal * SurfaceOffset;

    FVector CurrentPos = OwnerCharacter->GetActorLocation();
    FVector Direction = (DynamicTarget - CurrentPos).GetSafeNormal();

    if (Direction.IsNearlyZero())
    {
        bIsMoving = false;
        UE_LOG(LogTemp, Warning, TEXT("Arrived at Destination (ZeroVector)"));
        return;
    }

    FVector UpVector = OwnerCharacter->GetActorUpVector();
    Direction = FVector::VectorPlaneProject(Direction, UpVector);

    if (!Direction.IsNearlyZero())
        Direction.Normalize();

    FVector NewLocation = CurrentPos + Direction * MovementSpeed * DeltaTime;
    OwnerCharacter->SetActorLocation(NewLocation);

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

    if (FVector::Dist(NewLocation, DynamicTarget) <= AcceptanceRadius)
    {
        bIsMoving = false;
        UE_LOG(LogTemp, Warning, TEXT("Arrived at Destination. Distance: %f"), FVector::Dist(NewLocation, DynamicTarget));
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

    if (CurrentClimbingSurface)
    {
        FHitResult NormalHit;
        FVector StartNormal = FromPosition;
        FVector EndNormal = FromPosition - OwnerCharacter->GetActorUpVector() * MaxTraceDistance;

        if (GetWorld()->LineTraceSingleByChannel(NormalHit, StartNormal, EndNormal, ECC_WorldStatic, QueryParams))
        {
            if (NormalHit.GetComponent() && NormalHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
            {
                FVector TraceDir = -NormalHit.Normal;
                FVector TraceStart = FromPosition + NormalHit.Normal * 100.0f;
                FVector TraceEnd = FromPosition + TraceDir * MaxTraceDistance;

                DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 1.0f, 0, 2.0f);

                if (GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
                {
                    DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Green, false, 0.1f);

                    if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
                        return true;
                }
            }
        }

        FVector MeshCenter = CurrentClimbingSurface->Bounds.Origin;
        FVector DirToCenter = (MeshCenter - FromPosition).GetSafeNormal();

        FVector StartCenter = FromPosition;
        FVector EndCenter = FromPosition + DirToCenter * MaxTraceDistance;

        DrawDebugLine(GetWorld(), StartCenter, EndCenter, FColor::Magenta, false, 1.0f, 0, 2.0f);

        if (GetWorld()->LineTraceSingleByChannel(OutHit, StartCenter, EndCenter, ECC_WorldStatic, QueryParams))
        {
            DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Orange, false, 0.1f);

            if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
                return true;
        }
    }

    FVector StartDown = FromPosition + FVector::UpVector * 100.0f;
    FVector EndDown = FromPosition - FVector::UpVector * MaxTraceDistance;

    DrawDebugLine(GetWorld(), StartDown, EndDown, FColor::Cyan, false, 1.0f, 0, 2.0f);

    if (GetWorld()->LineTraceSingleByChannel(OutHit, StartDown, EndDown, ECC_WorldStatic, QueryParams))
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
    if (!bEnableSnap || !OwnerCharacter)
        return false;

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
        FVector MeshCenter = Hit.GetComponent()->Bounds.Origin;
        FVector DirToCenter = (MeshCenter - Hit.Location).GetSafeNormal();

        if (FVector::DotProduct(OutwardNormal, DirToCenter) > 0)
            OutwardNormal = -OutwardNormal;

        FVector TargetPosition = Hit.Location + OutwardNormal * SurfaceOffset;

        float PositionDelta = FVector::Dist(InOutPosition, TargetPosition);

        if (PositionDelta > SnapPositionTolerance)
            InOutPosition = TargetPosition;

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
            OutRotation = TargetRotation;
        else
            OutRotation = CurrentRotation;

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
        QueryParams.AddIgnoredComponent(RootComp);

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