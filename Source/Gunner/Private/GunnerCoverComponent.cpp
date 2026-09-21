#include "GunnerCoverComponent.h"
#include "GunnerAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/RootMotionSource.h"
#include "Engine/World.h"

UGunnerCoverComponent::UGunnerCoverComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.TickInterval = 0.04f;
}
void UGunnerCoverComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (Character)
    {
        // CharacterMovement already runs before its owning character. Drive input
        // before movement without making a circular owner/component prerequisite.
        Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
    }
}
bool UGunnerCoverComponent::WallAt(const FVector& Center, FHitResult& Hit, float Height) const
{
    return WallAtNormal(Center, WallNormal, bAttached ? AttachedOffset + 28.f : QueryReach, Hit, Height);
}
bool UGunnerCoverComponent::WallAtNormal(const FVector& Center, const FVector& Normal, float Reach,
    FHitResult& Hit, float Height) const
{
    if (!Character) return false;
    const float Half = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start = Center + FVector(0, 0, Height - Half);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GunnerCoverWall), false, Character);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - Normal * Reach, ECC_WorldStatic, Params)
        && FMath::Abs(Hit.ImpactNormal.Z) < 0.2f && FVector::DotProduct(Hit.ImpactNormal, Normal) > 0.95f;
}
bool UGunnerCoverComponent::HasSupportedFloor(const FVector& Center, float ReferenceFeetZ, float* OutFloorZ) const
{
    if (!Character) return false;
    FFindFloorResult Floor;
    Character->GetCharacterMovement()->FindFloor(Center + FVector(0.f, 0.f, 35.f), Floor, false);
    if (!Floor.IsWalkableFloor() || !Floor.HitResult.Component.IsValid()
        || Floor.HitResult.Component->Mobility != EComponentMobility::Static
        || FMath::Abs(Floor.HitResult.ImpactPoint.Z - ReferenceFeetZ) > 35.f) return false;
    if (OutFloorZ) *OutFloorZ = Floor.HitResult.ImpactPoint.Z;
    return true;
}
bool UGunnerCoverComponent::ValidateApproach(const FVector& Start, FVector& Destination) const
{
    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float FeetZ = Start.Z - HalfHeight;
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerCoverApproach), false, Character);
    FHitResult Obstacle;
    if (GetWorld()->SweepSingleByChannel(Obstacle, Start, Destination, FQuat::Identity,
        Capsule->GetCollisionObjectType(), Shape, Query, Responses)) return false;
    const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(Start, Destination) / 70.f));
    for (int32 Step = 0; Step <= Steps; ++Step)
    {
        const FVector Point = FMath::Lerp(Start, Destination, static_cast<float>(Step) / Steps);
        float FloorZ = 0.f;
        if (!HasSupportedFloor(Point, FeetZ, &FloorZ)) return false;
        if (Step == Steps) Destination.Z = FloorZ + HalfHeight + 2.f;
    }
    // Recheck the route after adapting the destination to its actual supporting floor.
    return !GetWorld()->SweepSingleByChannel(Obstacle, Start, Destination, FQuat::Identity,
        Capsule->GetCollisionObjectType(), Shape, Query, Responses)
        && !GetWorld()->OverlapBlockingTestByChannel(Destination, FQuat::Identity,
            Capsule->GetCollisionObjectType(), Shape, Query, Responses);
}
bool UGunnerCoverComponent::TryAttach(const FVector& SearchDirection, bool bFastApproach)
{
    if (!Character || !Character->HasAuthority() || !Character->GetController() || bAttached || bEntering
        || !Character->GetCharacterMovement()->IsMovingOnGround()
        || Character->GetCharacterMovement()->HasRootMotionSources()
        || Character->GetCharacterMovement()->bWantsToCrouch != Character->bIsCrouched) return false;
    const FVector Direction = SearchDirection.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return false;
    const FVector Center = Character->GetActorLocation();
    const float Half = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start = Center + FVector(0, 0, 55.f - Half);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GunnerCoverAttach), false, Character);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * QueryReach, ECC_WorldStatic, Params)
        || FMath::Abs(Hit.ImpactNormal.Z) > 0.2f || !Hit.Component.IsValid()
        || Hit.Component->Mobility != EComponentMobility::Static) return false;
    const FVector CandidateNormal = Hit.ImpactNormal.GetSafeNormal2D();
    // Reject short obstacles rather than treating ankle-high geometry as protection.
    FHitResult BodyHit;
    if (!WallAtNormal(Center, CandidateNormal, QueryReach, BodyHit, 90.f) || BodyHit.Component != Hit.Component) return false;
    FHitResult HighHit;
    const bool bCandidateLow = !WallAtNormal(Center, CandidateNormal, QueryReach, HighHit, 145.f)
        || HighHit.Component != Hit.Component;
    FVector Desired = Hit.ImpactPoint + CandidateNormal * AttachedOffset;
    Desired.Z = Center.Z;
    if (!ValidateApproach(Center, Desired)) return false;
    if (!WallAtNormal(Desired, CandidateNormal, AttachedOffset + 28.f, BodyHit, 90.f)
        || BodyHit.Component != Hit.Component) return false;
    const bool bDestinationLow = !WallAtNormal(Desired, CandidateNormal, AttachedOffset + 28.f, HighHit, 145.f)
        || HighHit.Component != Hit.Component;
    if (bDestinationLow != bCandidateLow) return false;

    // All admission queries above are pure: rejection does not move the pawn or replace
    // an existing anchor. The current armed gait honestly presents this approach.
    auto* Movement = Character->GetCharacterMovement();
    const float Speed = FMath::Clamp(bFastApproach ? FastApproachSpeed : ApproachSpeed, 100.f, 650.f);
    const float Duration = FMath::Clamp(FVector::Dist2D(Center, Desired) / Speed, 0.12f, 0.8f);
    TSharedPtr<FRootMotionSource_MoveToForce> Motion = MakeShared<FRootMotionSource_MoveToForce>();
    Motion->InstanceName = TEXT("GunnerCoverApproach");
    Motion->Priority = 480;
    Motion->AccumulateMode = ERootMotionAccumulateMode::Override;
    Motion->StartLocation = Center;
    Motion->TargetLocation = Desired;
    Motion->Duration = Duration;
    Motion->bRestrictSpeedToExpected = true;
    Motion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
    Motion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
    const uint16 SourceId = Movement->ApplyRootMotionSource(Motion);
    if (SourceId == static_cast<uint16>(ERootMotionSourceID::Invalid)) return false;

    PendingWall = Hit.Component;
    PendingNormal = CandidateNormal;
    PendingDestination = Desired;
    bPendingLow = bCandidateLow;
    EntryHalfHeight = Half;
    EntryFeetZ = Center.Z - Half;
    EntryElapsed = 0.f;
    EntryDuration = Duration;
    EntryMotionSourceId = SourceId;
    bEntering = true;
    Character->ConsumeMovementInputVector();
    Character->StopJumping();
    Movement->StopMovementImmediately();
    SetComponentTickInterval(0.f);
    SetComponentTickEnabled(true);
    return true;
}
void UGunnerCoverComponent::CancelTransition()
{
    if (!bEntering && EntryMotionSourceId == 0) return;
    bEntering = false;
    bPendingLow = false;
    if (Character)
    {
        auto* Movement = Character->GetCharacterMovement();
        if (EntryMotionSourceId != 0) Movement->RemoveRootMotionSourceByID(EntryMotionSourceId);
        Character->ConsumeMovementInputVector();
        Movement->StopMovementImmediately();
    }
    EntryMotionSourceId = 0;
    EntryElapsed = EntryDuration = 0.f;
    PendingWall.Reset();
    PendingNormal = PendingDestination = FVector::ZeroVector;
    SetComponentTickInterval(0.04f);
    SetComponentTickEnabled(bAttached);
}
void UGunnerCoverComponent::CompleteApproach()
{
    // Keep the final swept location. Never finish by snapping through a new obstacle.
    const TWeakObjectPtr<UPrimitiveComponent> NewWall = PendingWall;
    const FVector NewNormal = PendingNormal;
    const bool bNewLow = bPendingLow;
    CancelTransition();
    Wall = NewWall;
    WallNormal = NewNormal;
    bLow = bNewLow;
    bAttached = true;
    PeekState = EPeekState::None;
    bPeekRequestBlocked = false;
    auto* Movement = Character->GetCharacterMovement();
    Movement->SetPlaneConstraintNormal(WallNormal);
    Movement->SetPlaneConstraintOrigin(Character->GetActorLocation());
    Movement->SetPlaneConstraintEnabled(true);
    SetComponentTickEnabled(true);
}
void UGunnerCoverComponent::TickApproach(float DeltaTime)
{
    if (!Character || !Character->HasAuthority() || !Character->GetController() || !PendingWall.IsValid()
        || PendingWall->Mobility != EComponentMobility::Static
        || !Character->GetCharacterMovement()->IsMovingOnGround()
        || !FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), EntryHalfHeight, 1.f))
    {
        CancelTransition();
        return;
    }
    EntryElapsed += DeltaTime;
    FHitResult WallHit;
    FHitResult HighHit;
    const bool bLowNow = !WallAtNormal(PendingDestination, PendingNormal, AttachedOffset + 28.f, HighHit, 145.f)
        || HighHit.Component != PendingWall;
    FVector RevalidatedDestination = PendingDestination;
    if (EntryElapsed > EntryDuration + 0.4f
        || !WallAtNormal(PendingDestination, PendingNormal, AttachedOffset + 28.f, WallHit, 90.f)
        || WallHit.Component != PendingWall || bLowNow != bPendingLow
        || !HasSupportedFloor(Character->GetActorLocation(), EntryFeetZ)
        || !HasSupportedFloor(PendingDestination, EntryFeetZ)
        || !ValidateApproach(Character->GetActorLocation(), RevalidatedDestination)
        || FVector::DistSquared(RevalidatedDestination, PendingDestination) > FMath::Square(2.f))
    {
        // Validate before CharacterMovement runs so a newly introduced blocker
        // cannot deflect the approach sideways through SlideAlongSurface.
        CancelTransition();
        return;
    }
    if (FVector::Dist2D(Character->GetActorLocation(), PendingDestination) <= 2.f
        && FMath::Abs(Character->GetActorLocation().Z - PendingDestination.Z) <= 5.f)
        CompleteApproach();
}
void UGunnerCoverComponent::Detach()
{
    CancelTransition();
    if (Character && bAttached)
    {
        Character->GetCharacterMovement()->SetPlaneConstraintEnabled(false);
        if (IsPeeking())
        {
            Character->ConsumeMovementInputVector();
            Character->GetCharacterMovement()->StopMovementImmediately();
        }
    }
    bAttached = false;
    bLow = false;
    PeekState = EPeekState::None;
    bPeekRequestBlocked = false;
    PeekTransitionTime = 0.f;
    Wall.Reset();
    SetComponentTickInterval(0.04f);
    SetComponentTickEnabled(false);
}
void UGunnerCoverComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Detach();
    Super::EndPlay(Reason);
}
void UGunnerCoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (bEntering)
    {
        TickApproach(DeltaTime);
        return;
    }
    FHitResult Hit;
    const FVector ValidationPoint = IsPeeking() ? PeekAnchor : (Character ? Character->GetActorLocation() : FVector::ZeroVector);
    if (!Character || !Wall.IsValid() || !Character->GetCharacterMovement()->IsMovingOnGround()
        || !WallAt(ValidationPoint, Hit, 90.f) || Hit.Component != Wall)
    {
        Detach();
        return;
    }
    FHitResult HeightHit;
    const bool bLowNow = !WallAt(ValidationPoint, HeightHit, 145.f) || HeightHit.Component != Wall;
    if (bLowNow != bLow)
    {
        // Height changes need an authored stance transition; do not silently carry
        // a cached protection state onto a different-height part of one mesh.
        Detach();
        return;
    }
    if (PeekState != EPeekState::SteppingOut && PeekState != EPeekState::Returning) return;
    const FVector Target = PeekState == EPeekState::Returning ? PeekAnchor : PeekDestination;
    const FVector ToTarget = (Target - Character->GetActorLocation()).GetSafeNormal2D();
    const float Remaining = FVector::Dist2D(Target, Character->GetActorLocation());
    if (Remaining <= 2.f)
    {
        Character->ConsumeMovementInputVector();
        Character->GetCharacterMovement()->StopMovementImmediately();
        PeekState = PeekState == EPeekState::Returning ? EPeekState::None : EPeekState::Exposed;
        SetComponentTickInterval(0.04f);
        return;
    }
    PeekTransitionTime += DeltaTime;
    if (PeekTransitionTime > 2.5f)
    {
        // A newly introduced obstacle can block the route. Leave the pawn at its safe
        // swept position and release the cover lock instead of forcing it through.
        Detach();
        return;
    }
    Character->AddMovementInput(ToTarget, FMath::Clamp(Remaining / 30.f, 0.f, 1.f));
}
FVector UGunnerCoverComponent::ConstrainMovement(const FVector& DesiredDirection) const
{
    if (bEntering) return FVector::ZeroVector;
    if (!bAttached || !Character) return DesiredDirection;
    if (IsPeeking()) return FVector::ZeroVector;
    const FVector Tangent = FVector::CrossProduct(FVector::UpVector, WallNormal);
    const float Along = FVector::DotProduct(DesiredDirection, Tangent);
    if (FMath::Abs(Along) < 0.05f) return FVector::ZeroVector;
    const auto* Movement = Character->GetCharacterMovement();
    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    const float Speed = FMath::Max(Character->GetVelocity().Size2D(), Movement->GetMaxSpeed());
    // Include the braking distance so raising cover speed cannot overrun the fixed
    // capsule-sized lookahead that sufficed for the original slow shuffle.
    const float LookAhead = FMath::Max(44.f, Capsule->GetScaledCapsuleRadius()
        + FMath::Square(Speed) / (2.f * FMath::Max(100.f, Movement->BrakingDecelerationWalking)));
    const FVector Start = Character->GetActorLocation();
    const float FeetZ = Start.Z - Capsule->GetScaledCapsuleHalfHeight();
    const int32 Steps = FMath::Max(1, FMath::CeilToInt(LookAhead / 70.f));
    for (int32 Step = 1; Step <= Steps; ++Step)
    {
        const FVector Test = Start + Tangent * FMath::Sign(Along) * LookAhead * (static_cast<float>(Step) / Steps);
        FHitResult Ahead;
        FHitResult HighHit;
        if (!WallAt(Test, Ahead, 90.f) || Ahead.Component != Wall || !HasSupportedFloor(Test, FeetZ))
            return FVector::ZeroVector;
        const bool bLowAhead = !WallAt(Test, HighHit, 145.f) || HighHit.Component != Wall;
        if (bLowAhead != bLow) return FVector::ZeroVector;
    }
    return Tangent * Along;
}
bool UGunnerCoverComponent::CanPeek(float Side) const
{
    if (bEntering) return false;
    if (!bAttached || !Character) return true;
    if (bLow) return true;
    if (bPeekRequestBlocked || PeekState == EPeekState::Returning) return false;
    FVector Direction;
    if (IsPeeking())
    {
        return FindOpenEdge(PeekAnchor, Side, Direction) && FVector::DotProduct(Direction, PeekDirection) > 0.95f;
    }
    FVector Destination;
    return FindPeekDestination(Character->GetActorLocation(), Side, Destination, Direction);
}

bool UGunnerCoverComponent::GetLowCoverTop(float& OutWorldZ) const
{
    if (!IsLowCover() || !Character || !Wall.IsValid()) return false;
    const float Feet = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector OverWall = Character->GetActorLocation() - WallNormal * (AttachedOffset + 8.f);
    OverWall.Z = Feet + 155.f;
    FHitResult Top;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerCoverTop), false, Character);
    if (!GetWorld()->LineTraceSingleByChannel(Top, OverWall, FVector(OverWall.X, OverWall.Y, Feet + 85.f),
        ECC_WorldStatic, Query) || Top.Component != Wall || Top.ImpactNormal.Z < 0.9f) return false;
    OutWorldZ = Top.ImpactPoint.Z;
    return OutWorldZ - Feet >= 90.f && OutWorldZ - Feet <= 145.f;
}

bool UGunnerCoverComponent::FindOpenEdge(const FVector& Anchor, float Side, FVector& OutDirection) const
{
    if (!Character || !Character->GetController() || !Wall.IsValid()) return false;
    FHitResult AnchorWall;
    if (!WallAt(Anchor, AnchorWall, 145.f) || AnchorWall.Component != Wall) return false;
    FHitResult Edge;
    const FVector Tangent = FVector::CrossProduct(FVector::UpVector, WallNormal);
    const FVector CameraRight = FRotationMatrix(FRotator(0, Character->GetController()->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y);
    const float Along = FVector::DotProduct(CameraRight * Side, Tangent);
    if (FMath::Abs(Along) < 0.15f) return false;
    OutDirection = Tangent * FMath::Sign(Along);
    return !WallAt(Anchor + OutDirection * 85.f, Edge, 145.f);
}

bool UGunnerCoverComponent::FindPeekDestination(const FVector& Anchor, float Side,
    FVector& OutDestination, FVector& OutDirection) const
{
    if (!Character) return false;
    // Lateral crouched exposure needs actual directional source coverage in the
    // assigned graph. Never start while a pending stance would change anchor Z.
    if (Character->bIsCrouched || Character->GetCharacterMovement()->bWantsToCrouch)
    {
        const auto* Anim = Cast<UGunnerAnimInstance>(Character->GetMesh()->GetAnimInstance());
        if (!Anim || !Anim->bDirectionalCrouchPoseReady
            || Character->GetCharacterMovement()->bWantsToCrouch != Character->bIsCrouched) return false;
    }
    if (!FindOpenEdge(Anchor, Side, OutDirection)) return false;
    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    OutDestination = Anchor + OutDirection * 70.f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerCoverPeek), false, Character);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    FHitResult Obstacle;
    if (GetWorld()->SweepSingleByChannel(Obstacle, Anchor, OutDestination, FQuat::Identity,
        Capsule->GetCollisionObjectType(),
        FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight), Query, Responses)) return false;
    // Validate the route and the exposed position before starting the authored gait.
    for (int32 Step = 1; Step <= 2; ++Step)
    {
        const FVector Point = FMath::Lerp(Anchor, OutDestination, Step * 0.5f);
        FFindFloorResult Floor;
        Character->GetCharacterMovement()->FindFloor(Point + FVector(0, 0, 20.f), Floor, false);
        if (!Floor.IsWalkableFloor() || !Floor.HitResult.Component.IsValid()
            || Floor.HitResult.Component->Mobility != EComponentMobility::Static
            || FMath::Abs(Floor.HitResult.ImpactPoint.Z - (Anchor.Z - HalfHeight)) > 35.f) return false;
    }
    return true;
}

void UGunnerCoverComponent::BeginReturn()
{
    if (PeekState == EPeekState::None || PeekState == EPeekState::Returning) return;
    PeekState = EPeekState::Returning;
    PeekTransitionTime = 0.f;
    SetComponentTickInterval(0.f);
}

void UGunnerCoverComponent::SetPeekDesired(bool bDesired, float Side)
{
    if (!bAttached || !Character || bLow) return;
    if (!bDesired)
    {
        bPeekRequestBlocked = false;
        BeginReturn();
        return;
    }
    if (PeekState == EPeekState::Returning) return;
    if (IsPeeking())
    {
        FVector Direction;
        if (!FindOpenEdge(PeekAnchor, Side, Direction) || FVector::DotProduct(Direction, PeekDirection) < 0.95f)
        {
            bPeekRequestBlocked = true;
            BeginReturn();
        }
        return;
    }
    FVector Destination;
    FVector Direction;
    const FVector Anchor = Character->GetActorLocation();
    if (!FindPeekDestination(Anchor, Side, Destination, Direction))
    {
        bPeekRequestBlocked = true;
        return;
    }
    bPeekRequestBlocked = false;
    PeekAnchor = Anchor;
    PeekDestination = Destination;
    PeekDirection = Direction;
    PeekTransitionTime = 0.f;
    PeekState = EPeekState::SteppingOut;
    Character->ConsumeMovementInputVector();
    Character->GetCharacterMovement()->StopMovementImmediately();
    SetComponentTickInterval(0.f);
}
