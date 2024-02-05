// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
