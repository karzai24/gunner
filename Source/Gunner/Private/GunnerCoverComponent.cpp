#include "GunnerCoverComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
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
    if (!Character) return false;
    const float Half = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start = Center + FVector(0, 0, Height - Half);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GunnerCoverWall), false, Character);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - WallNormal * (bAttached ? AttachedOffset + 28.f : QueryReach), ECC_WorldStatic, Params)
        && FMath::Abs(Hit.ImpactNormal.Z) < 0.2f && FVector::DotProduct(Hit.ImpactNormal, WallNormal) > 0.95f;
}
bool UGunnerCoverComponent::TryAttach(const FVector& SearchDirection)
{
    if (!Character || !Character->HasAuthority() || bAttached || !Character->GetCharacterMovement()->IsMovingOnGround()) return false;
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
    WallNormal = Hit.ImpactNormal.GetSafeNormal2D();
    // Reject short obstacles rather than treating ankle-high geometry as protection.
    FHitResult BodyHit;
    if (!WallAt(Center, BodyHit, 90.f) || BodyHit.Component != Hit.Component) return false;
    FHitResult HighHit;
    bLow = !WallAt(Center, HighHit, 145.f) || HighHit.Component != Hit.Component;
    FVector Desired = Hit.ImpactPoint + WallNormal * AttachedOffset;
    Desired.Z = Center.Z;
    FHitResult Sweep;
    Character->SetActorLocation(Desired, true, &Sweep);
    if (FVector::Dist2D(Character->GetActorLocation(), Desired) > 3.f)
    {
        // A blocked snap keeps its safe swept position; never teleport through the obstruction.
        return false;
    }
    Wall = Hit.Component;
    bAttached = true;
    PeekState = EPeekState::None;
    bPeekRequestBlocked = false;
    auto* Movement = Character->GetCharacterMovement();
    Movement->StopMovementImmediately();
    Movement->SetPlaneConstraintNormal(WallNormal);
    Movement->SetPlaneConstraintOrigin(Desired);
    Movement->SetPlaneConstraintEnabled(true);
    SetComponentTickEnabled(true);
    return true;
}
void UGunnerCoverComponent::Detach()
{
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
    FHitResult Hit;
    const FVector ValidationPoint = IsPeeking() ? PeekAnchor : (Character ? Character->GetActorLocation() : FVector::ZeroVector);
    if (!Character || !Wall.IsValid() || !Character->GetCharacterMovement()->IsMovingOnGround()
        || !WallAt(ValidationPoint, Hit) || Hit.Component != Wall)
    {
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
    if (!bAttached || !Character) return DesiredDirection;
    if (IsPeeking()) return FVector::ZeroVector;
    const FVector Tangent = FVector::CrossProduct(FVector::UpVector, WallNormal);
    const float Along = FVector::DotProduct(DesiredDirection, Tangent);
    FHitResult Ahead;
    // Keep the entire capsule beside the same wall. Corners require explicit detach/re-attach.
    const FVector Test = Character->GetActorLocation() + Tangent * FMath::Sign(Along) * 44.f;
    if (FMath::Abs(Along) < 0.05f || !WallAt(Test, Ahead) || Ahead.Component != Wall) return FVector::ZeroVector;
    return Tangent * Along;
}
bool UGunnerCoverComponent::CanPeek(float Side) const
{
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
    // A forward-only crouch gait cannot represent a lateral step while facing the gun.
    if (!Character || Character->bIsCrouched || Character->GetCharacterMovement()->bWantsToCrouch) return false;
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
