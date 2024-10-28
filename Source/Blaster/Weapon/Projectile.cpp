// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	// 플레이어와 충돌처리해주기 위해
	// ECollisionChannel::ECC_Pawn은 FPS에서 충돌처리가 너무 세세하지가 않다.
	// 따라서 플레이어 메쉬에 따로 콜리전 오브젝트 타입을 지정해준다.
	// 플레이어 메쉬는 더 이상 Pawn 타입을 따르지 않고 SkeletalMesh 타입을 따른다.
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// 총알 나가는 이펙트
	if (Tracer)
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

	if (HasAuthority())
	{
		// 서버에서만 충돌 처리
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// AProjectile::BeginPlay() -> HasAuthority() -> AddDynamic(this, &AProjectile::OnHit)
	// OnHit은 서버 환경에서만 실행된다.
	
	// 피격 대상
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	bool bCharacterHit = false;

	if (BlasterCharacter)
	{
		bCharacterHit = true;
		ImpactParticles = ImpactCharacterParticles;
	}

	// 버그) 땅에다가 쏠 시 이펙트가 발생하지 않음
	// 원인) Destroy가 되기 전에 이미 사라짐(?) 너무 빨리 사라진다
	// 해결) 파괴 되기 전에 멀티캐스트RPC로 이펙트 출력
	if (HasAuthority())
	{
		Multicast_OnHit(bCharacterHit);
	}

	Destroy();
}

void AProjectile::Multicast_OnHit_Implementation(bool bCharacterHit)
{
	ImpactParticles = bCharacterHit ? ImpactCharacterParticles : ImpactObstacleParticles;

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
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

void AProjectile::ExplodeDamage()
{
	// ProjectileWeapon::Fire에서 투사체를 소환하기 전에 Instigator를 설정했음
	APawn* FiringPawn = GetInstigator();
	// 서버에서만 데미지 처리
	if (FiringPawn && HasAuthority())
	{
		// 상대방에게 데미지를 입히기 위해 가해자의 컨트롤러를 가져옴
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Damage, // 기본 데미지
				10.f, // 최소 데미지
				GetActorLocation(), // 로켓의 기준으로 가상의 원 생성
				DamageInnerRadius, // 안쪽 원 반지름
				DamageOuterRadius, // 바깥쪽 원 반지름
				1.f, // DamageFalloff
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // 무시할 Actor(빈 배열)
				this, // DamageCauser
				FiringController // InstigatorController
			);
		}
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	// OnDestroyed.Broadcast(this);
	// 클라이언트에서도 호출된다. -> 클라이언트에 복제된 Actor가 없어진다.
	Super::Destroyed();
}

