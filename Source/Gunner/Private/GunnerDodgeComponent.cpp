#include "GunnerDodgeComponent.h"

#include "GunnerCombatComponent.h"
#include "GunnerCoverComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

UGunnerDodgeComponent::UGunnerDodgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGunnerDodgeComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    Combat = Character->FindComponentByClass<UGunnerCombatComponent>();
    Cover = Character->FindComponentByClass<UGunnerCoverComponent>();
    Character->MovementModeChangedDelegate.AddDynamic(this, &UGunnerDodgeComponent::OnMovementModeChanged);
}

void UGunnerDodgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelDodge();
    if (Character)
    {
        Character->MovementModeChangedDelegate.RemoveDynamic(this, &UGunnerDodgeComponent::OnMovementModeChanged);
    }
    Super::EndPlay(EndPlayReason);
}

bool UGunnerDodgeComponent::FindSafeDestination(const FVector& Direction, FVector& Destination) const
{
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    const FVector Start = Character->GetActorLocation();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GunnerDodgeClearance), false, Character);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    const ECollisionChannel Channel = Capsule->GetCollisionObjectType();
    float Distance = FMath::Clamp(DodgeDistance, 100.f, 350.f);
    FHitResult Obstacle;
    if (GetWorld()->SweepSingleByChannel(Obstacle, Start, Start + Direction * Distance,
        FQuat::Identity, Channel, Shape, Query, Responses))
    {
        if (Obstacle.bStartPenetrating) return false;
        Distance = Distance * Obstacle.Time - 4.f;
    }
    if (Distance < 100.f) return false;
    Destination = Start + Direction * Distance;

    // Check support along the route as well as at the landing. The actual movement remains
    // swept by CharacterMovement, and leaving walking cancels the source immediately.
    const float StartFloorZ = Start.Z - HalfHeight;
    const int32 Steps = FMath::CeilToInt(Distance / 70.f);
    for (int32 Step = 1; Step <= Steps; ++Step)
    {
        const FVector Point = FMath::Lerp(Start, Destination, static_cast<float>(Step) / Steps);
        FFindFloorResult Floor;
        Movement->FindFloor(Point + FVector(0.f, 0.f, 35.f), Floor, false);
        if (!Floor.IsWalkableFloor() || !Floor.HitResult.Component.IsValid()
            || Floor.HitResult.Component->Mobility != EComponentMobility::Static
            || FMath::Abs(Floor.HitResult.ImpactPoint.Z - StartFloorZ) > 35.f)
        {
            return false;
        }
        if (Step == Steps) Destination.Z = Floor.HitResult.ImpactPoint.Z + HalfHeight + 2.f;
    }

    // A raised destination must also have full standing clearance. Retaining the actual
    // capsule prevents a rolling pose from granting passage under an untested low ceiling.
    return !GetWorld()->SweepSingleByChannel(Obstacle, Start, Destination, FQuat::Identity,
        Channel, Shape, Query, Responses);
}

bool UGunnerDodgeComponent::TryDodge()
{
    if (!Character || !Character->HasAuthority() || !Character->GetController() || bDodging
        || !RollMontage || Character->bIsCrouched || (Cover && Cover->IsAttached())) return false;
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!Movement->IsMovingOnGround() || Movement->bWantsToCrouch) return false;
    if (Combat && (Combat->IsReloading() || Combat->IsMeleeing()
        || Combat->GetActionState() == EGunnerCombatAction::Equipping)) return false;
    UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
    if (!Anim) return false;
    const float Duration = RollMontage->GetPlayLength() / FMath::Max(0.01f, RollMontage->RateScale);
    if (!FMath::IsFinite(Duration) || Duration < 0.1f || Duration > 5.f) return false;

    const FVector Velocity = Character->GetVelocity();
    const FVector Direction = Velocity.SizeSquared2D() > FMath::Square(30.f)
        ? Velocity.GetSafeNormal2D()
        : FRotator(0.f, Character->GetController()->GetControlRotation().Yaw, 0.f).Vector();
    FVector Destination;
    if (!FindSafeDestination(Direction, Destination)) return false;

    if (Combat) Combat->StopAllActions();
    if (Anim->Montage_Play(RollMontage, 1.f, EMontagePlayReturnType::Duration) <= 0.f) return false;
    bDodging = true;
    ActiveMontage = RollMontage;
    ++ActionSerial;
    bPreviousOrientToMovement = Movement->bOrientRotationToMovement;
    Movement->bOrientRotationToMovement = false;
    Movement->StopMovementImmediately();
    Character->StopJumping();
    Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));

    TSharedPtr<FRootMotionSource_MoveToForce> Motion = MakeShared<FRootMotionSource_MoveToForce>();
    Motion->InstanceName = TEXT("GunnerDodge");
    Motion->Priority = 500;
    Motion->AccumulateMode = ERootMotionAccumulateMode::Override;
    Motion->StartLocation = Character->GetActorLocation();
    Motion->TargetLocation = Destination;
    Motion->Duration = Duration;
    Motion->bRestrictSpeedToExpected = true;
    Motion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
    Motion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
    MotionSourceId = Movement->ApplyRootMotionSource(Motion);
    if (MotionSourceId == static_cast<uint16>(ERootMotionSourceID::Invalid))
    {
        CancelDodge();
        return false;
    }
    FOnMontageEnded End;
    End.BindUObject(this, &UGunnerDodgeComponent::OnMontageEnded, ActionSerial);
    Anim->Montage_SetEndDelegate(End, ActiveMontage);
    GetWorld()->GetTimerManager().SetTimer(TimeoutTimer, this, &UGunnerDodgeComponent::CancelDodge,
        Duration + 0.3f, false);
    return true;
}

void UGunnerDodgeComponent::CancelDodge()
{
    FinishDodge(true);
}

void UGunnerDodgeComponent::FinishDodge(bool bStopMontage)
{
    if (!bDodging && !ActiveMontage && MotionSourceId == 0) return;
    bDodging = false;
    ++ActionSerial;
    UAnimMontage* Montage = ActiveMontage;
    ActiveMontage = nullptr;
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TimeoutTimer);
    if (Character)
    {
        UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
        if (MotionSourceId != 0) Movement->RemoveRootMotionSourceByID(MotionSourceId);
        Movement->StopMovementImmediately();
        Movement->bOrientRotationToMovement = bPreviousOrientToMovement;
        if (bStopMontage && Montage)
        {
            if (UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance()) Anim->Montage_Stop(0.1f, Montage);
        }
    }
    MotionSourceId = 0;
}

void UGunnerDodgeComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ExpectedSerial)
{
    if (bDodging && ExpectedSerial == ActionSerial && Montage == ActiveMontage) FinishDodge(false);
}

void UGunnerDodgeComponent::OnMovementModeChanged(ACharacter* ChangedCharacter, EMovementMode PreviousMode,
    uint8 PreviousCustomMode)
{
    if (bDodging && ChangedCharacter == Character && !Character->GetCharacterMovement()->IsMovingOnGround())
    {
        CancelDodge();
    }
}
