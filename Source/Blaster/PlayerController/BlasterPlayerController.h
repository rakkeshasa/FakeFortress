// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABlasterPlayerController();

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetDeathMessage(const FString KillerName);
	void HideDeathMessage();
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDGunType(const FString GunType);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PawnLeavingGame() override;

	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);

	virtual float GetServerTime();
	virtual void ReceivedPlayer() override;
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	void HandleCooldown();
	void PlayAnnounceSound(class ABlasterGameState* BlasterGameState);

	float SingleTripTime = 0.f;	

	FHighPingDelegate HighPingDelegate;

	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

	FORCEINLINE class UChatComponent* GetChatComponent() const { return ChatComponent; }

	void BroadcastCountDown(int sec);

protected:
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();
	virtual void SetupInputComponent() override;

	/*
	*  ������ Ŭ�� Ÿ�̸� ����ȭ
	*/

	// ���� ���� �ð��� ��û, ��û�� ���� Ŭ���̾�Ʈ�� �ð��� �Ű�����
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Ŭ���̾�Ʈ�� ��û�� ������ ���� �ð��� Ȯ��
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	
	// ������ Ŭ���̾�Ʈ�� �ð� ����
	float ClientServerDelta = 0.f; 
	
	// �󸶳� ���� �ð��� ����ȭ ��ų������
	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	// �ð� ����ȭ Ÿ�̸�
	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime, bool bIsTeamsMatch);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	UFUNCTION()
	void OnRep_ShowTeamScores();

	FString GetInfoText(const TArray<class ABlasterPlayerState*>& Players);
	FString GetTeamsInfoText(class ABlasterGameState* BlasterGameState);

	UFUNCTION(Client, Reliable)
	void ClientCountDown(int sec);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	// ä��
	UPROPERTY(VisibleAnywhere, Category = Chat)
	UChatComponent* ChatComponent;

	/*UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<UUserWidget> ChatBoxClass;

	UPROPERTY()
	class UChatBox* ChatBox;*/

	/*
	*  ���� �޴�
	*/
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;
	FString HUDGun;
	bool bInitializeGun = false;
	bool bInitializeTeamScore = false;

	/*
	*  �� ����
	*/
	float HighPingRunningTime = 0.f;
	float PingAnimationRunningTime = 0.f;

	// �� ��� �˸� �ð�
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;
	
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 100.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	USoundBase* ThirtySecSoundCue;

	UPROPERTY(EditAnywhere)
	USoundBase* TenSecSoundCue;

	UPROPERTY(EditAnywhere)
	USoundBase* FiveSecSoundCue;

	UPROPERTY(EditAnywhere)
	USoundBase* WinnerSoundCue;

	UPROPERTY(EditAnywhere)
	USoundBase* LoserSoundCue;
};
