// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
	//캐릭터 클래스의 bIsCrouched를 그대로 애님인스턴스 변수에 설정한다.
	bIsCrouched = BlasterCharacter->bIsCrouched;
	//캐릭터가 현재 조준 중인지 확인한다 
	bAiming = BlasterCharacter->IsAiming();

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
}
