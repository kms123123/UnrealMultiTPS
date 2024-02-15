// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	//움직이는 방향에 맞춰 회전도 변하도록 설정
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
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
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

