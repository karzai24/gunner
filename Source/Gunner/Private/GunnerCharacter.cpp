#include "GunnerCharacter.h"
#include "GunnerInputConfig.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

AGunnerCharacter::AGunnerCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(36.f, 90.f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 480.f, 0.f);
    Movement->MaxWalkSpeed = 450.f;
    Movement->MinAnalogWalkSpeed = 20.f;
    Movement->MaxAcceleration = 1200.f;
    Movement->BrakingDecelerationWalking = 1600.f;
    Movement->JumpZVelocity = 440.f;
    Movement->AirControl = 0.2f;
    Movement->SetCrouchedHalfHeight(62.f);
    // Crouch remains disabled until real crouched idle + locomotion are integrated.
    Movement->GetNavAgentPropertiesRef().bCanCrouch = false;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.f;
    CameraBoom->TargetOffset = FVector(0.f, 0.f, 65.f);
    CameraBoom->SocketOffset = FVector(0.f, 50.f, 0.f);
    CameraBoom->ProbeSize = 12.f;
    CameraBoom->bUsePawnControlRotation = true;
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->FieldOfView = 75.f;
    FollowCamera->bUsePawnControlRotation = false;
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGunnerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!ensureMsgf(Input && InputConfig && InputConfig->Move && InputConfig->MouseLook &&
        InputConfig->StickLook && InputConfig->Traverse, TEXT("Gunner pawn needs a complete input data asset"))) return;
    Input->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &AGunnerCharacter::Move);
    Input->BindAction(InputConfig->MouseLook, ETriggerEvent::Triggered, this, &AGunnerCharacter::MouseLook);
    Input->BindAction(InputConfig->StickLook, ETriggerEvent::Triggered, this, &AGunnerCharacter::StickLook);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Started, this, &AGunnerCharacter::Traverse);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    Input->BindAction(InputConfig->Traverse, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
}

void AGunnerCharacter::Move(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FVector2D Axis = Value.Get<FVector2D>();
    const FRotationMatrix Yaw(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
    AddMovementInput(Yaw.GetUnitAxis(EAxis::X), Axis.Y);
    AddMovementInput(Yaw.GetUnitAxis(EAxis::Y), Axis.X);
}

void AGunnerCharacter::MouseLook(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGunnerCharacter::StickLook(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
}

void AGunnerCharacter::Traverse()
{
    // M0 has only the grounded jump fallback. Cover/vault arbitration belongs to M3.
    if (GetCharacterMovement()->IsMovingOnGround()) Jump();
}
