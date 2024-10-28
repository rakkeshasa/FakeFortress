// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ThrowGrenade.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AThrowGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AThrowGrenade();
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:

	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
