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

	// �÷��̾�� �浹ó�����ֱ� ����
	// ECollisionChannel::ECC_Pawn�� FPS���� �浹ó���� �ʹ� ���������� �ʴ�.
	// ���� �÷��̾� �޽��� ���� �ݸ��� ������Ʈ Ÿ���� �������ش�.
	// �÷��̾� �޽��� �� �̻� Pawn Ÿ���� ������ �ʰ� SkeletalMesh Ÿ���� ������.
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// �Ѿ� ������ ����Ʈ
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
		// ���������� �浹 ó��
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// AProjectile::BeginPlay() -> HasAuthority() -> AddDynamic(this, &AProjectile::OnHit)
	// OnHit�� ���� ȯ�濡���� ����ȴ�.
	
	// �ǰ� ���
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	bool bCharacterHit = false;

	if (BlasterCharacter)
	{
		bCharacterHit = true;
		ImpactParticles = ImpactCharacterParticles;
	}

	// ����) �����ٰ� �� �� ����Ʈ�� �߻����� ����
	// ����) Destroy�� �Ǳ� ���� �̹� �����(?) �ʹ� ���� �������
	// �ذ�) �ı� �Ǳ� ���� ��Ƽĳ��ƮRPC�� ����Ʈ ���
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
	// ProjectileWeapon::Fire���� ����ü�� ��ȯ�ϱ� ���� Instigator�� ��������
	APawn* FiringPawn = GetInstigator();
	// ���������� ������ ó��
	if (FiringPawn && HasAuthority())
	{
		// ���濡�� �������� ������ ���� �������� ��Ʈ�ѷ��� ������
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Damage, // �⺻ ������
				10.f, // �ּ� ������
				GetActorLocation(), // ������ �������� ������ �� ����
				DamageInnerRadius, // ���� �� ������
				DamageOuterRadius, // �ٱ��� �� ������
				1.f, // DamageFalloff
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // ������ Actor(�� �迭)
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
	// Ŭ���̾�Ʈ������ ȣ��ȴ�. -> Ŭ���̾�Ʈ�� ������ Actor�� ��������.
	Super::Destroyed();
}

