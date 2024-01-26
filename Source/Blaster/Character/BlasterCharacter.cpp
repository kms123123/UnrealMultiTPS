// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if(UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,&ABlasterCharacter::Move );
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,&ABlasterCharacter::Look );
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
	}
	
}

