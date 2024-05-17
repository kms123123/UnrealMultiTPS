// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	void DestoryTimerFinished();
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	FTimerHandle DestoryTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	
};
