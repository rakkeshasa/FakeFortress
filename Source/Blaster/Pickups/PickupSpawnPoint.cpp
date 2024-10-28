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

	// nullptr이므로 따로 캐스트 확인은 안한다
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupSpawnPoint::SpawnPickup()
{
	// 랜덤으로 아이템 재소환
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());
		
		// 픽업을 한 이후에 어떻게 타이머가 돌아가도록 할 것인가?
		// 아이템이 픽업이 되면 Destroy가 되므로 델리게이트를 이용한다.
		if (HasAuthority() && SpawnedPickup)
		{
			// 버그) 스폰 위치에 캐릭터가 서있으면 재소환이 되지 않음
			// 원인) 스폰 위치에 캐릭터가 있어 소환되자마자 파괴되서 콜백이 일어나지 않음
			// 해결) Overlap 이벤트 딜레이 주기
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	// OnDestroy 델리게이트가 브로드캐스트할 때 호출하고 DestroyedActor를 받는다.
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
	// 클라이언트한테도 복제가 되어야한다. -> bReplicates = true
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

