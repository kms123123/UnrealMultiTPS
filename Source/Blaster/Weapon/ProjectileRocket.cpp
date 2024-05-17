// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystemInstanceController.h"
#include "Components/AudioComponent.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::Destroyed()
{
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if(!HasAuthority())
	{
		// ReSharper disable once CppBoundToDelegateMethodIsNotMarkedAsUFunction
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

	if(TrailSystem)
	{
		TrailSystemComponent =  UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if(ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	// 자신의 Owner을 return
	APawn* FiringPawn = GetInstigator();
	if(FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if(FiringController)
		{
			// 스플래시 데미지를 주는 함수
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				200.f,
				500.f,
				1.f, // 데미지 감소함수, 1.f이면 linear하게 감소
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				FiringController // GameMode 클래스에 전달해주기 위함
			);
		}
	}

	GetWorldTimerManager().SetTimer(
		DestoryTimer,
		this,
		&AProjectileRocket::DestoryTimerFinished,
		DestroyTime
	);

	//벽 파티클 소환
	if(ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorTransform());
	}

	//사운드 재생
	if(ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if(RocketMesh)
	{
		RocketMesh->SetVisibility(false);
	}
	if(CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileRocket::DestoryTimerFinished()
{
	Destroy();
}
