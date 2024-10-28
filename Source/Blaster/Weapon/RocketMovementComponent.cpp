// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	// �ùķ��̼� ������Ʈ �� blocking hit�� ó���Ѵ�.
	// �ùķ��̼� ���̸� HandleImpact()�� ȣ���ϰ� �⺻������ EHandleHitWallResult::Deflect�� ��ȯ�Ͽ� HandleDeflection()�� ���� ���� �ٿ �� �����̵� ������ Ȱ��ȭ�Ѵ�.
	// �� �̻� �ùķ��̼����� ������ EHandleHitWallResult::Abort�� ��ȯ�Ͽ� �߰� �ùķ��̼� �õ��� �ߴ��Ѵ�.
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	// EHandleBlockingHitResult�� HandleBlockingHit()�� ȣ��� �� �ùķ��̼��� �������� ��Ÿ��
	// �߻�ü�� ���� ��ü�� �ı��Ǿ� �̵��� ��ӵǾ�� �ϴ� ���� ���� �߰� �����̵�/��Ƽ �ٿ ������ ������ �� �ִ� ��쿡 ����.
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// ������ ������ �ʰ�, �ݸ��� �ڽ��� ��Ʈ�� ������ ���� �����Ѵ�.

}