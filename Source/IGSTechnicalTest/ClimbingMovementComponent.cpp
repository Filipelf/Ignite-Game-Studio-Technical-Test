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

// Shows debug text on screen and log
void UClimbingMovementComponent::DebugPrint(const FString& Message, float Duration)
{
    if (bShowDebug && OwnerCharacter)
    {
        GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, Message);
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
    }
}

// Initializes component and snaps character to surface on spawn
void UClimbingMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<AClimbingCharacter>(GetOwner());

    if (OwnerCharacter)
    {
        DebugPrint(TEXT("ClimbingMovementComponent Initialized"), 3.0f);

        FVector Pos = OwnerCharacter->GetActorLocation();
        FRotator Rot = OwnerCharacter->GetActorRotation();

        FHitResult Hit;
        if (FindClimbingSurface(Pos, Hit))
        {
            CurrentClimbingSurface = Hit.GetComponent();

            if (SnapToSurface(Pos, Rot))
            {
                OwnerCharacter->SetActorLocationAndRotation(Pos, Rot);
                DebugPrint(TEXT("Initial Snap Applied"), 3.0f);
            }
        }
        else
        {
            DebugPrint(TEXT("No Surface Found at Spawn"), 5.0f);
        }
    }
}

// Main update loop, delegates to state functions
void UClimbingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCharacter)
        return;

    if (!bEnableSnap)
    {
        NoSnapMovement(DeltaTime);
        return;
    }

    if (!bIsMoving)
    {
        CharacterIdleStateWithSnap();
        return;
    }

    CharacterMovingStateWithSnap(DeltaTime);
}

// Linear movement without surface snapping
void UClimbingMovementComponent::NoSnapMovement(float DeltaTime)
{
    if (!bIsMoving)
        return;

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
        bIsMoving = false;
}

// Maintains surface adhesion while stationary
void UClimbingMovementComponent::CharacterIdleStateWithSnap()
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
                FVector NewPos = FMath::VInterpTo(OwnerCharacter->GetActorLocation(), CorrectPos, GetWorld()->GetDeltaSeconds(), 5.0f);
                OwnerCharacter->SetActorLocation(NewPos);
            }
        }
    }
}

// Moves along surface with dynamic target and rotation alignment
void UClimbingMovementComponent::CharacterMovingStateWithSnap(float DeltaTime)
{
    FVector DynamicTarget = TargetDestination;
    FHitResult Hit;

    if (FindClimbingSurface(TargetDestination, Hit))
        DynamicTarget = Hit.Location + Hit.Normal * SurfaceOffset;

    FVector CurrentPos = OwnerCharacter->GetActorLocation();
    FVector Direction = (DynamicTarget - CurrentPos).GetSafeNormal();

    if (Direction.IsNearlyZero())
    {
        bIsMoving = false;
        DebugPrint(TEXT("Arrived at Destination (ZeroVector)"));
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

    float DistanceToTarget = FVector::Dist(NewLocation, DynamicTarget);

    if (DistanceToTarget <= AcceptanceRadius)
    {
        if (WaypointQueue.Num() > 0)
        {
            TargetDestination = WaypointQueue[0];
            WaypointQueue.RemoveAt(0);

            DebugPrint(FString::Printf(TEXT("Moving to Next Waypoint")));
        }
        else
        {
            bIsMoving = false;

            DebugPrint(TEXT("Arrived at Final Destination"), 3.0f);
        }
    }
}

// Locates climbable surface
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

                if (bShowDebug)
                    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 0.2f, 0, 2.0f);

                if (GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
                {
                    if (bShowDebug)
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

        if (bShowDebug)
            DrawDebugLine(GetWorld(), StartCenter, EndCenter, FColor::Magenta, false, 1.0f, 0, 2.0f);

        if (GetWorld()->LineTraceSingleByChannel(OutHit, StartCenter, EndCenter, ECC_WorldStatic, QueryParams))
        {
            if (bShowDebug)
                DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Orange, false, 0.1f);

            if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
                return true;
        }
    }

    FVector StartDown = FromPosition + FVector::UpVector * 100.0f;
    FVector EndDown = FromPosition - FVector::UpVector * MaxTraceDistance;

    if (bShowDebug)
        DrawDebugLine(GetWorld(), StartDown, EndDown, FColor::Cyan, false, 1.0f, 0, 2.0f);

    if (GetWorld()->LineTraceSingleByChannel(OutHit, StartDown, EndDown, ECC_WorldStatic, QueryParams))
    {
        if (bShowDebug)
            DrawDebugSphere(GetWorld(), OutHit.Location, 20.0f, 12, FColor::Blue, false, 0.1f);

        if (OutHit.GetComponent() && OutHit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            CurrentClimbingSurface = OutHit.GetComponent();
            return true;
        }
    }

    return false;
}

// Calculates optimal position and rotation to adhere to surface
bool UClimbingMovementComponent::SnapToSurface(FVector& InOutPosition, FRotator& OutRotation)
{
    if (!bEnableSnap || !OwnerCharacter)
        return false;

    if (InOutPosition.ContainsNaN())
    {
        DebugPrint(TEXT("SnapToSurface got NaN position"), 5.0f);
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

// Checks if straight path is blocked by non-climbable obstacle
bool UClimbingMovementComponent::IsPathBlocked(const FVector& Start, const FVector& End)
{
    if (!OwnerCharacter)
        return true;

    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    QueryParams.bTraceComplex = true;

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams);

    if (bHit && Hit.GetComponent())
    {
        if (!Hit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            if (bShowDebug)
            {
                DrawDebugLine(GetWorld(), Start, Hit.Location, FColor::Red, false, 2.0f, 0, 4.0f);
                DrawDebugSphere(GetWorld(), Hit.Location, 30.0f, 12, FColor::Red, false, 2.0f);
            }

            return true;
        }
    }

    if (bShowDebug)
        DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 0.5f, 0, 2.0f);

    return false;
}

// Generates offset point to bypass obstacle
FVector UClimbingMovementComponent::FindDetourPoint(const FVector& Current, const FVector& Target, int32 Attempt)
{
    if (!OwnerCharacter)
        return Target;

    FVector Direction = (Target - Current).GetSafeNormal();

    if (Direction.IsNearlyZero())
        return Target;

    FVector UpVector = OwnerCharacter->GetActorUpVector();
    FVector Right = FVector::CrossProduct(UpVector, Direction).GetSafeNormal();
    FVector Left = -Right;

    FVector DetourDirection = (Attempt % 2 == 0) ? Right : Left;

    float DetourDistance = BaseDetourDistance + (Attempt * DetourDistanceIncrement);

    FVector DetourPoint = Current + DetourDirection * DetourDistance;

    FHitResult Hit;

    if (FindClimbingSurface(DetourPoint, Hit))
        DetourPoint = Hit.Location + Hit.Normal * SurfaceOffset;

    if (bShowDebug)
    {
        DrawDebugSphere(GetWorld(), DetourPoint, 40.0f, 12, FColor::Yellow, false, 3.0f);
        DrawDebugString(GetWorld(), DetourPoint, FString::Printf(TEXT("Attempt %d"), Attempt), nullptr, FColor::Yellow, 3.0f);
    }

    return DetourPoint;
}

// Validates click and finds path, using steering algorithm if blocked
void UClimbingMovementComponent::MoveToLocation(const FVector& TargetLocation)
{
    if (!OwnerCharacter)
        return;

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

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams))
    {
        if (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ClimbableSurface")))
        {
            FVector FinalDestination = Hit.Location + Hit.Normal * SurfaceOffset;
            FVector CurrentPos = OwnerCharacter->GetActorLocation();

            if (IsPathBlocked(CurrentPos, FinalDestination))
            {
                bool bFoundDetour = false;

                for (int32 Attempt = 0; Attempt < MaxDetourAttempts; Attempt++)
                {
                    FVector DetourPoint = FindDetourPoint(CurrentPos, FinalDestination, Attempt);

                    if (!IsPathBlocked(CurrentPos, DetourPoint))
                    {
                        if (!IsPathBlocked(DetourPoint, FinalDestination))
                        {
                            WaypointQueue.Empty();
                            WaypointQueue.Add(DetourPoint);
                            WaypointQueue.Add(FinalDestination);

                            TargetDestination = WaypointQueue[0];
                            WaypointQueue.RemoveAt(0);
                            bIsMoving = true;

                            DebugPrint(FString::Printf(TEXT("Obstacle Found. Using Detour")));
                            return;
                        }
                    }
                }

                DebugPrint(TEXT("No Detour Found. Moving Straight."), 3.0f);
            }

            TargetDestination = FinalDestination;
            bIsMoving = true;

            DebugPrint(TEXT("Moving to Valid Destination"), 2.0f);

            return;
        }
    }

    if (FindClimbingSurface(TargetLocation, Hit))
    {
        TargetDestination = Hit.Location + Hit.Normal * SurfaceOffset;
        bIsMoving = true;

        DebugPrint(TEXT("Moving to Destination (Fallback)"), 2.0f);

        return;
    }

    DebugPrint(TEXT("Invalid Click. No Surface Found."), 3.0f);
}