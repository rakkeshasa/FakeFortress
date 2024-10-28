// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	// 시뮬레이션 업데이트 중 blocking hit을 처리한다.
	// 시뮬레이션 중이면 HandleImpact()를 호출하고 기본적으로 EHandleHitWallResult::Deflect를 반환하여 HandleDeflection()을 통해 다중 바운스 및 슬라이딩 지원을 활성화한다.
	// 더 이상 시뮬레이션하지 않으면 EHandleHitWallResult::Abort를 반환하여 추가 시뮬레이션 시도를 중단한다.
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	// EHandleBlockingHitResult은 HandleBlockingHit()이 호출된 후 시뮬레이션의 진행방식을 나타냄
	// 발사체를 막은 물체가 파괴되어 이동이 계속되어야 하는 경우와 같이 추가 슬라이드/멀티 바운스 로직을 무시할 수 있는 경우에 사용됨.
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// 로켓은 멈추지 않고, 콜리전 박스가 히트를 감지할 때만 폭발한다.

}