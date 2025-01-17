// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	//애님 인스턴스를 보유하는 폰을 블래스터 캐릭터로 치환
	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if(BlasterCharacter == nullptr) return;

	//캐릭터로부터 Velocity 벡터를 얻고, 그 크기를 speed로 설정한다.
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	speed = Velocity.Size();

	//캐릭터 무브먼트를 얻고, 내장되어있는 IsFalling함수를 쓴다.
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	//캐릭터 무브먼트를 얻고, 내장되어있는 가속 벡터를 얻은 뒤, 이것의 크기가 0보다 큰지 확인
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	//캐릭터로부터 현재 무기를 장착하였는지 확인한다
	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	//캐릭터로부터 무기변수를 가져온다
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	//캐릭터 클래스의 bIsCrouched를 그대로 애님인스턴스 변수에 설정한다.
	bIsCrouched = BlasterCharacter->bIsCrouched;
	//캐릭터가 현재 조준 중인지 확인한다 
	bAiming = BlasterCharacter->IsAiming();
	//캐릭터가 현재 회전중인지 확인한다
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	//캐릭터가 루트 본 회전을 사용해야 되는지 확인한다
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	//캐릭터가 제거되었는지 확인한다
	bElimmed = BlasterCharacter->IsElimmed();

	//GetBaseAimRotation은 폰의 AimRotation을 반환하는데, 이는 곧 컨트롤러가 보는 방향을 말한다.
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	//FABRIK IK를 쓰기 위한 로직
	//먼저 Weapon의 메쉬와 캐릭터의 메쉬에 접근 가능해야한다
	if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		//GetSocketTransform을 통해 소켓의 월드 트랜스폼을 가져온다
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		//TransformToBoneSpace를 통해 특정 트랜스폼의 위치를 특정 본에 대한 본 space로 치환한다
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("Hand_R"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		//그렇게 치환한 위치와 회전정보를 트랜스폼에 저장한다
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//총이 가리키는 방향이 자연스럽게 FindLookAtRotation을 이용해 오른손으로부터 타겟의 회전값을 오른손에 설정해준다.
		//이는 로컬에서만 사용하도록 설정 (Bandwidth 낭비 X)
		if(BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bUseAimOffsets = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
	bTransformRightHand = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
}
