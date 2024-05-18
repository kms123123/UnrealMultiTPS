// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Sound/SoundCue.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement Component"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::Destroyed()
{
	ExplodeDamage();
	
	Super::Destroyed();
}

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay();

	SpawnTrailSystem();
	StartDestoryTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ImpactResult.GetActor());
	if(BlasterCharacter)
	{
		Destroy();
	}
	
	if(BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}
