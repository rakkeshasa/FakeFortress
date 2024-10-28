// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown; 
}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	UFUNCTION()
	void CountDownPlay(int sec);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	// 팀전인지 체크용
	bool bTeamsMatch = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	virtual void Logout(AController* Exiting) override;

private:
	float CountdownTime = 0.f;

	bool ThirtyPlay = false;
	bool TenPlay = false;
	bool FivePlay = false;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
