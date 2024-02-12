// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//루트의 콜라이더의 크기를 Crouch에 따라 조정할 계획이므로, 메쉬에 붙인다.
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	//컨트롤러의 로테이션 값을 받아서 로테이트 하도록 설정한다.
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//카메라를 스프링암에 붙인다.
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	//스프링암이 회전할 것이므로 카메라는 컨트롤러의 회전값을 받지 않도록 한다.
	FollowCamera->bUsePawnControlRotation = false;
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	//컴포넌트는 따로 프로퍼티 처럼 매크로로 설정하지 않고 다음과 같은 함수로 리플리케이션을 설정할 수 있다.
	Combat->SetIsReplicated(true);

	//무브먼트 컴포넌트에서 Crouch를 가능하게한다.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	//캡슐 컴포넌트에서 카메라에 대한 충돌을 무시하도록 하여, 캐릭터가 뒤로 지나갔을 때에 카메라가 순간적으로 움직이는 현상을 막는다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

//리플리케이션 프로퍼티 적용
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//오로지 오너 액터에만 리플리케이션 될 수 있게 설정
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//컴뱃 컴포넌트가 생성되고 난 후, 캐릭터를 설정해준다.
	if(Combat)
	{
		Combat->Character = this;
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if(PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if(Subsystem)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D VectorValue = Value.Get<FVector2D>();

	const FRotator Rotation = GetControlRotation();
	const FRotator RotationYaw(0.f, Rotation.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(RotationYaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(RotationYaw).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, VectorValue.Y);
	AddMovementInput(Right, VectorValue.X);
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D VectorValue = Value.Get<FVector2D>();

	if(Controller)
	{
		AddControllerYawInput(VectorValue.Y);
		AddControllerPitchInput(VectorValue.X);
	}
}

void ABlasterCharacter::Jump()
{
	Super::Jump();
}

void ABlasterCharacter::EquipButtonPressed()
{
	//서버에서만 실행되어야 하므로 HasAuthority 적용, 아닌 경우에는 Remote Procedure Call 함수 실행
	if(Combat)
	{
		if( HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();			
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if(Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	//무기가 없으면 리턴
	if(Combat && Combat->EquippedWeapon == nullptr) return;

	//현재의 속도와 공중에 떠있는 지 여부를 체크하여, AimOffset을 적용할지를 결정
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// Standing Still, not Jumping -> AimOffset 적용
	// 움직이기 직전까지의 방향과 현재 가리키는 방향의 Delta를 계산하여, 이의 Yaw값을 설정해준다
	// 이때는 컨트롤러에 의해 캐릭터의 방향이 바뀌지 않도록 한다
	if(Speed == 0.f && !bIsInAir) 
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = false;
	}
	// Running or Jumping
	// 움직이는 동안은 컨트롤러에 의해 캐릭터의 방향이 바뀌도록 한다
	if(Speed > 0.f || bIsInAir)
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
	}

	// Pitch 회전값은 움직이는 것과 관계없이 항상 설정하도록 한다
	AO_Pitch = GetBaseAimRotation().Pitch;
	
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if(UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,&ABlasterCharacter::Move );
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,&ABlasterCharacter::Look );
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::EquipButtonPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
	}
	
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if(IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}