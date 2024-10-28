// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	// nullptr�̹Ƿ� ���� ĳ��Ʈ Ȯ���� ���Ѵ�
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupSpawnPoint::SpawnPickup()
{
	// �������� ������ ���ȯ
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());
		
		// �Ⱦ��� �� ���Ŀ� ��� Ÿ�̸Ӱ� ���ư����� �� ���ΰ�?
		// �������� �Ⱦ��� �Ǹ� Destroy�� �ǹǷ� ��������Ʈ�� �̿��Ѵ�.
		if (HasAuthority() && SpawnedPickup)
		{
			// ����) ���� ��ġ�� ĳ���Ͱ� �������� ���ȯ�� ���� ����
			// ����) ���� ��ġ�� ĳ���Ͱ� �־� ��ȯ���ڸ��� �ı��Ǽ� �ݹ��� �Ͼ�� ����
			// �ذ�) Overlap �̺�Ʈ ������ �ֱ�
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	// OnDestroy ��������Ʈ�� ��ε�ĳ��Ʈ�� �� ȣ���ϰ� DestroyedActor�� �޴´�.
	// DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FActorDestroyedSignature, AActor, OnDestroyed, AActor*, DestroyedActor );
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);

	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	// Ŭ���̾�Ʈ���׵� ������ �Ǿ���Ѵ�. -> bReplicates = true
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

