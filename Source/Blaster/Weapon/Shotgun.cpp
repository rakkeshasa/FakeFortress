// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"

//void AShotgun::Fire(const FVector& HitTarget)
//{
//	AWeapon::Fire(HitTarget);
//
//	APawn* OwnerPawn = Cast<APawn>(GetOwner());
//	if (OwnerPawn == nullptr) return;
//	AController* InstigatorController = OwnerPawn->GetController();
//
//	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
//	
//	if (MuzzleFlashSocket)
//	{
//		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
//		FVector Start = SocketTransform.GetLocation();
//		
//		// ��ź���̱⿡ ���� ����� ���� �� �����Ƿ� map���� ó��
//		TMap<ABlasterCharacter*, uint32> HitMap;
//
//		// �� źȯ���� ó�����ֱ�
//		for (uint32 i = 0; i < NumberOfScatters; i++)
//		{
//			FHitResult FireHit;
//			WeaponTraceHit(Start, HitTarget, FireHit);
//
//			// źȯ�� �ϳ��� �¾Ҵٸ� ���� Ƚ��++, �ƴ϶�� 1ȸ �ǰ�
//			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
//			if (BlasterCharacter && HasAuthority() && InstigatorController)
//			{
//				if (HitMap.Contains(BlasterCharacter))
//				{
//					HitMap[BlasterCharacter]++;
//				}
//				else
//				{
//					HitMap.Emplace(BlasterCharacter, 1);
//				}
//			}
//
//			if (ImpactParticles)
//			{
//				UGameplayStatics::SpawnEmitterAtLocation(
//					GetWorld(),
//					ImpactParticles,
//					FireHit.ImpactPoint,
//					FireHit.ImpactNormal.Rotation()
//				);
//			}
//
//			if (HitSound)
//			{
//				UGameplayStatics::PlaySoundAtLocation(
//					this,
//					HitSound,
//					FireHit.ImpactPoint,
//					.5f,
//					FMath::FRandRange(-.5f, .5f)
//				);
//			}
//		}
//
//		// �����ڿ��� �ǰݵ� ź����ŭ ������ ����
//		for (auto HitPair : HitMap)
//		{
//			// ����ȯ�濡�� ������ ����
//			if (HitPair.Key && HasAuthority() && InstigatorController)
//			{
//				UGameplayStatics::ApplyDamage(
//					HitPair.Key, // �ǰ� ���
//					Damage * HitPair.Value, // ���� źȯ ��
//					InstigatorController,
//					this,
//					UDamageType::StaticClass()
//				);
//			}
//		}
//	}
//}

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	// ������ AWeapon::Fire���� �Ű������� �����Ѵ�.
	AWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		// ��ź���̱⿡ ���� ����� ���� �� �����Ƿ� map���� ó��
		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotHitMap;

		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(BlasterCharacter)) HeadShotHitMap[BlasterCharacter]++;
					else HeadShotHitMap.Emplace(BlasterCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(BlasterCharacter)) HitMap[BlasterCharacter]++;
					else HitMap.Emplace(BlasterCharacter, 1);
				}

				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						FireHit.ImpactPoint,
						FireHit.ImpactNormal.Rotation()
					);
				}

				if (HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HitSound,
						FireHit.ImpactPoint,
						.5f,
						FMath::FRandRange(-.5f, .5f)
					);
				}
			}
		}

		TArray<ABlasterCharacter*> HitCharacters;
		TMap<ABlasterCharacter*, float> DamageMap; // ���� �������� ���� map

		for (auto HitPair : HitMap)
		{
			// �Ϲ� �� ������ ����
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);
				
				HitCharacters.AddUnique(HitPair.Key);
			}
		}

		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key)
			{
				// ��� �� ������ �����ֱ�
				if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);
				
				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		// �����ڿ��� �ǰݵ� ź����ŭ ������ ����
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					// ����ȯ�濡�� ������ ����
					UGameplayStatics::ApplyDamage(
						DamagePair.Key, // �ǰ���
						DamagePair.Value, // �Ϲݼ� + ��弦
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}

		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
			BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
			if (BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensation() && BlasterOwnerCharacter->IsLocallyControlled())
			{
				BlasterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfScatters; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;

		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
		HitTargets.Add(ToEndLoc);
	}
}
