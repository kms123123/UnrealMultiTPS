// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USoundCue;
class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AProjectile();
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
private:
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;
	
	UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticle;

	UPROPERTY(EditAnywhere)
	UParticleSystem* PlayerHitParticle;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

};
