// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/WeaponTypes.h"

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
	//메쉬의 콜리전 채널을 커스텀채널로 설정해준다.
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	//LineTrace에 막혀야 하므로 Visibility 채널에는 Block으로 설정한다.
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//회전 속도 변경
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	//Turning Enum 초기화
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	//Net Update Frequency 변경
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	MaxHealth = 100.f;
	Health = 100.f;

	//충돌시에도 스폰이 될 수 있도록 설정
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

}

//리플리케이션 프로퍼티 적용
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//오로지 오너 액터에만 리플리케이션 될 수 있게 설정
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
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

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	//무기가 없으면 공격 몽타주 실행 X
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	//메쉬로부터 애님 인스턴스를 가져온다
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage)
	{
		//애님인스턴스에서 몽타주를 플레이하고, aiming 여부를 체크하여 섹션으로 점프한다
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	//무기가 없으면 공격 몽타주 실행 X
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	//메쉬로부터 애님 인스턴스를 가져온다
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ReloadMontage)
	{
		//애님인스턴스에서 몽타주를 플레이하고, aiming 여부를 체크하여 섹션으로 점프한다
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_SubmachineGun:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Rifle");
				break;
			default:
				break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	//무기가 없으면 공격 몽타주 실행 X
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	//메쉬로부터 애님 인스턴스를 가져온다
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		//애님인스턴스에서 몽타주를 플레이하고, aiming 여부를 체크하여 섹션으로 점프한다
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if(Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode =  GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if(BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}

}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::PollInit()
{
	// PlayerState는 BeginPlay에서 initialize가 되지않는다. (할당되는데 1~2 프레임 소요)
	// 따라서, Tick에서 최대한 빠르게 initialize해준다.
	if(BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if(BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement() 
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

// 서버에서만 실행되는 함수
void ABlasterCharacter::Elim()
{
	if(Combat && Combat->EquippedWeapon)
	{ 
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	// 리스폰 타이머를 맞춘다.
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->SetHUDWeaponTypeText(EWeaponType::EWT_MAX);
	}
	bElimmed = true;
	PlayElimMontage();

	// Start dissolve effect
	if(DissolveMaterialInstance)
	{
		// 저장되어있는 매터리얼 인스턴스를 통해 새로운 다이나믹 인스턴스를 만든다.
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		// 이를 메쉬에 설정한다.
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		// 매터리얼 인스턴스의 변수값을 바꿔준다.
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if(Combat)
	{
		Combat->FireButtonPressed(false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn ElimBot
	if(ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if(ElimBotSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode =  GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if(BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}

}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if(ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if(Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(const ULocalPlayer* Player = (GEngine && GetWorld()) ? GEngine->GetFirstGamePlayer(GetWorld()) : nullptr)
	{
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if(PlayerController)
		{
			UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
			if(Subsystem)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
				bInputSet = true;
			}
		}
	}

	UpdateHUDHealth();
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(HasAuthority() && !bInputSet)
	{
		if(const ULocalPlayer* Player = (GEngine && GetWorld()) ? GEngine->GetFirstGamePlayer(GetWorld()) : nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(GetController());
			if(PlayerController)
			{
				UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
				if(Subsystem)
				{
					Subsystem->AddMappingContext(InputMappingContext, 0);
					bInputSet = true;
				}
			}
		}
	}

	RotateInPlace(DeltaTime);
	
	HideCameraIfCharacterClose();
	PollInit();
}


void ABlasterCharacter::RotateInPlace(float DeltaSeconds)
{
	if(bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	//시뮬레이티드프록시가 아니면 매 프레임마다 AimOffset 실행
	if(GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaSeconds);
	}
	//시뮬레이티드프록시라면 에임오프셋을 적용하지 않고 특수 회전을 부여한다.
	else
	{
		TimeSinceLastMovementReplication += DeltaSeconds;
		if(TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}


void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if(bDisableGameplay) return;
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
	if(bDisableGameplay) return;
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;

	float speed = CalculateSpeed();
	if(speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	CalculateAO_Pitch();
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if(ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if(ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::EquipButtonPressed()
{
	if(bDisableGameplay) return;
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
	if(bDisableGameplay) return;
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if(bDisableGameplay) return;
	if(Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if(bDisableGameplay) return;
	if(Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(bDisableGameplay) return;
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if(bDisableGameplay) return;
	if(Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(bDisableGameplay) return;
	if(Combat)
	{
		Combat->FireButtonPressed(false);
	}
}


void ABlasterCharacter::AimOffset(float DeltaTime)
{
	//무기가 없으면 리턴
	if(Combat && Combat->EquippedWeapon == nullptr) return;

	//현재의 속도와 공중에 떠있는 지 여부를 체크하여, AimOffset을 적용할지를 결정
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// Standing Still, not Jumping -> AimOffset 적용
	// 움직이기 직전까지의 방향과 현재 가리키는 방향의 Delta를 계산하여, 이의 Yaw값을 설정해준다
	// Rotate RootBone을 통해서 루트본 회전을 할 것이므로, 컨트롤러에 의해 회전이 바뀌어도 된다.
	if(Speed == 0.f && !bIsInAir) 
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	// Running or Jumping
	// 움직이는 동안은 컨트롤러에 의해 캐릭터의 방향이 바뀌도록 한다
	if(Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	//클라이언트에서 발생하는 Pitch 값 문제 해결
	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	// Pitch 회전값은 움직이는 것과 관계없이 항상 설정하도록 한다
	AO_Pitch = GetBaseAimRotation().Pitch;
	
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
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
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::ReloadButtonPressed);
	}
	
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	//AO_Yaw값을 실시간으로 체크해서 AimOffset 범위를 넘어갔을 경우, enum 값을 바꿔준다.
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	
	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// 로컬 플레이어에만 이루어져야 하므로, 로컬로 컨트롤 되지 않는 캐릭터는 무시
	if(!IsLocallyControlled()) return;

	// 카메라 컴포넌트와 액터 컴포넌트의 거리가 일정거리 이하일때,
	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		// 메쉬가 안보이도록 하고,
		GetMesh()->SetVisibility(false);
		// weaponmesh의 bOwnerNoSee를 true로 만들어 Owner에게 안보이도록 한다.
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
	
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// 시그니처에 콜백함수 바인딩
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if(DissolveCurve)
	{
		// 타임라인에 커브 변수 설정 및 실행
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
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

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if(Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}
