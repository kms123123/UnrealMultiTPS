// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Blaster.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	//클라이언트에서도 이 액터가 보이도록 설정
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	//프로젝타일은 움직이니까 월드 다이나믹 채널로 설정
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	//부딪혀야되니까 쿼리앤피직스
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//모든 채널을 무시했다가
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	//visibility, worldstatic 채널만 부딪히게 설정
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	// 캐릭터와도 부딪히게 SkeletalMesh(커스텀) 채널에 대해서도 부딪힘
	// 원통 콜라이더도 폰 채널이기 때문에, 커스텀 채널을 따로 사용한다.
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	

}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	//서버에서만 충돌 이벤트를 바인딩한다
	if(HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::DestoryTimerFinished()
{
	Destroy();
}

void AProjectile::ExplodeDamage()
{
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
				DamageInnerRadius,
				DamageOuterRadius,
				1.f, // 데미지 감소함수, 1.f이면 linear하게 감소
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				FiringController // GameMode 클래스에 전달해주기 위함
			);
		}
	}
}

//파괴되면 클라이언트에서도 실행되기 때문에 여기서 파티클이나 사운드를 재생시킨다.
void AProjectile::Destroyed()
{
	Super::Destroyed();
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
}

void AProjectile::StartDestoryTimer()
{
	GetWorldTimerManager().SetTimer(
		DestoryTimer,
		this,
		&AProjectile::DestoryTimerFinished,
		DestroyTime
	);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
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
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

