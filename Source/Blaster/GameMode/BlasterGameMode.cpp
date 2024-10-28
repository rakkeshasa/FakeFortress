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
	// WaitingToStart ���� ����(Default Pawn �����Ǿ� �̵� ��ɸ� �����ϰ� �޽��� ����)
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// ���α׷� ���� �� BlasterMap ������ �����ϴµ� �ɸ� �ð�
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ��� �ð� üũ
	if (MatchState == MatchState::WaitingToStart)
	{
		// GetWorld()->GetTimeSeconds(): ���α׷� ���� �� ���ݱ����� �ð�
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// InProgress ���·� ��ȯ(��� ĳ���� ���� �� �Է� ��� ��� ����)
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
			// ������ ���� ������ �ٽ� �̵��ϰ� ������ ���� �����Ͽ� ��� Ŭ������ ��� ��ü�� �ٽ� ������
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
	// ���Ӹ�� -> ���� ȯ��
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
		// ĳ���ͷκ��� ��Ʈ�ѷ� �и�
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	if (ElimmedController)
	{
		// �÷��̾������ ���� �� ��Ȱ ����Ʈ���� ��Ȱ
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

		//int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1); // ���� ��Ȱ ����Ʈ�� ���
		RestartPlayerAtPlayerStart(ElimmedController, AllPlayerStarts[Selection]);
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	// �ִٵ����ڰ� ���� ���
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

	// ������ �ִ� ��� �÷��̾��� ��Ʈ�ѷ�
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
