// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Sound/SoundCue.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	// WaitingToStart 상태 유지(Default Pawn 생성되어 이동 기능만 가능하고 메쉬가 없다)
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 프로그램 실행 후 BlasterMap 레벨에 입장하는데 걸린 시간
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 대기 시간 체크
	if (MatchState == MatchState::WaitingToStart)
	{
		// GetWorld()->GetTimeSeconds(): 프로그램 실행 후 지금까지의 시간
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// InProgress 상태로 전환(모든 캐릭터 스폰 및 입력 기능 사용 가능)
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 31.f && !ThirtyPlay)
		{
			ThirtyPlay = true;
			CountDownPlay(30);
		}

		if (CountdownTime <= 11.f && !TenPlay)
		{
			TenPlay = true;
			CountDownPlay(10);
		}

		if (CountdownTime <= 6.f && !FivePlay)
		{
			FivePlay = true;
			CountDownPlay(5);
		}

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
			ThirtyPlay = false;
			TenPlay = false;
			FivePlay = false;
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{

		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// 서버가 현재 맵으로 다시 이동하고 게임을 새로 시작하여 모든 클래스의 모든 개체를 다시 생성함
			RestartGame();
		}
	}
}

void ABlasterGameMode::CountDownPlay(int sec)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->BroadcastCountDown(sec);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// 게임모드 -> 서버 환경
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
		FString AttackerName = AttackerPlayerState->GetPlayerName();
		VictimPlayerState->SetKiller(AttackerName);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// 캐릭터로부터 컨트롤러 분리
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		// 플레이어들한테 가장 먼 부활 포인트에서 부활
		TArray<AActor*>AllPlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), AllPlayerStarts);

		TArray<AActor*>Characters;
		UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), Characters);

		TArray<float>StockedDistances;

		for (int i = 0; i < AllPlayerStarts.Num(); i++)
		{
			float MinDistance = (AllPlayerStarts[i]->GetActorLocation() - Characters[0]->GetActorLocation()).Size();

			for (int j = 1; j < Characters.Num(); j++)
			{
				float Distance = (AllPlayerStarts[i]->GetActorLocation() - Characters[j]->GetActorLocation()).Size();

				if (Distance < MinDistance)
				{
					MinDistance = Distance;
				}
			}
			StockedDistances.Add(MinDistance);
		}

		float MaxDistance = StockedDistances[0];
		int32 Selection = 0;

		for (int i = 1; i < StockedDistances.Num(); i++)
		{
			if (MaxDistance < StockedDistances[i])
			{
				MaxDistance = StockedDistances[i];
				Selection = i;
			}
		}

		//int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1); // 랜덤 부활 포인트의 경우
		RestartPlayerAtPlayerStart(ElimmedController, AllPlayerStarts[Selection]);
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	// 최다득점자가 나갈 경우
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());
	if (LeavingCharacter)
	{
		LeavingCharacter->Elim(true);
	}
}

float ABlasterGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// 레벨에 있는 모든 플레이어의 컨트롤러
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

void ABlasterGameMode::Logout(AController* Exiting)
{
	if (Exiting == nullptr) { return; }

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(Exiting->PlayerState);
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
	{
		BlasterGameState->TopScoringPlayers.Remove(BlasterPlayerState);
	}

	Super::Logout(Exiting);
}
