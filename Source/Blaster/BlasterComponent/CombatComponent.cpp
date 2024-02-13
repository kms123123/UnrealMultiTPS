// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	//속도 수치 설정
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//게임 시작 시 BaseWalkSpeed로 이동속도 설정
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	//Server키워드 RPC는 서버에서 호출해도 똑같이 실행되므로 문제가 없다.
	ServerSetAiming(bIsAiming);

	//현재 Aim을 하고 있는지 판단 후 속도를 바꿔준다.
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if(Character && bFireButtonPressed)
	{
		//공격 몽타주 로직 실행
		Character->PlayFireMontage(bAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	//클라이언트가 RPC 함수를 실행 -> 서버에서 값 바꿈 -> 그 변수는 리플리케이션 되어있으므로 그대로 리플리케이트되어 클라이언트에 반영
	bAiming = bIsAiming;

	//클라이언트와 서버와의 속도가 모두 같아야 하므로, 클라이언트->서버 요청도 들어줘서 이동속도를 바꿔야 한다.
	//이 로직이 없으면 클라이언트는 느린 반면에 서버는 느리게 변하지 않으므로, 서버가 업데이트 할때마다 렉이 걸린 것처럼 움직임
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//무기 변수가 리플리케이션 되어 애님 인스턴스에서 이 값을 클라이언트도 전달받을 수 있다.
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(Character == nullptr || WeaponToEquip == nullptr) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	//메쉬를 통해 소켓을 가져올 수 있다.
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);

	//무기를 들 때부터 움직임에 따라 몸의 방향이 바뀌지 않고 컨트롤러의 회전에 따라 바뀌도록 한다
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

