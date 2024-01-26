// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
}
