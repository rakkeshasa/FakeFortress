

#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	// �� �����Ӹ��� �� �Լ��� ȣ��
	// �� �Լ��� NativeInitializeAniation�Լ����� ������ ĳ���͸� �����ϱ� ���� ������ ĳ���Ϳ� �����ϸ� �ȵȴ�.
	if (BlasterCharacter == nullptr)
	{
		// NativeInitializeAnimation���� ���� ȣ����� ��� ������ ĳ���� ����
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (BlasterCharacter == nullptr) return;

	// ĳ���� �ӵ� ����
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f; // z�� �ӵ��� �ʿ� ����
	Speed = Velocity.Size();

	// �� �� �ΰ����� �̵� ���� ����
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	
	
	//////////////////////
	//     ���� ����    //
	//////////////////////
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();

	//////////////////////
	//     �ൿ ����    //
	//////////////////////
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bElimmed = BlasterCharacter->IsElimmed();
	bHoldingTheFlag = BlasterCharacter->IsHoldingTheFlag();

	// ���콺 ȸ�� ��ġ: Global Rotation���� ���� x���� �������� ��
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	
	// �̵� ���� ��ġ: ���� x���� �������� 0��.
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

	// ĳ���� ����
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	// �� ���⺰ �޼� ��ġ
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// ������ World Transform�� ���缭 ������
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		// ���� �����̽����� ���̷����� �޽� �������ֱ�
		// �������� �ѱ� ���� ��ġ�� �ٲ��� �����Ƿ� �������� ��� ���� �� ��ġ�� ����ش�.
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		/*
		*  �������� ���� �ѱ��� ������ ���߱�(������: ������)
		*/
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	
	bool bFABRIKOverride = BlasterCharacter->IsLocallyControlled() && 
		BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && 
		BlasterCharacter->bFinishedSwapping;
	if (bFABRIKOverride)
	{
		bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
	}

	bUseAimOffsets = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();
	bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableGameplay();

}

//void UBlasterAnimInstance::OnReloadFailedToBlendOut(UAnimMontage* AnimMontage, bool bInterrupted)
//{
//	if (bInterrupted)
//	{
//		BlasterCharacter->GetCombat()->FinishReloading();
//	}
//	else
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Animation is blending out"));
//	}
//}
//
//void UBlasterAnimInstance::OnReloadSucceedAnimationEnd(UAnimMontage* AnimMontage, bool bInterrupted)
//{
//	UE_LOG(LogTemp, Warning, TEXT("Animation has completed"));
//}
