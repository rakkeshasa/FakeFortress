

#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/PlayerState.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Components/BoxComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ����) ��� ĳ���Ͱ� ��Ȱ���� �ʴ� ���
	// ����) ��Ȱ �� �ٸ� ĳ���Ͱ� �ش� ��ġ�� �־� �ݸ����� ���ļ� ��Ȱ�� �ȵ�
	// �ذ�) ��Ȱ �ɼ��� ����
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// ī�޶� ����
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); // CombatComponent ���� �Ӽ� ����
	// ������Ʈ�� ���� ������ ��� ������� �ʿ䰡 ����.

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	// LagCompensation�� ���������� ���
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// �÷��̾� - �ٸ� �÷��̾� - ī�޶� �� �� ȭ���� Ȯ��Ǵ� ���� �ذ�
	// ĸ�� ������Ʈ�� �޽��� ī�޶�� �浹�� �ȵǵ��� ����
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// �÷��̾� ���� �� �������� ������ �ϱ� ����
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	// ����) �Ѿ��� �߿��� ����
	// ����) �ִϸ��̼� ��Ÿ�������� ���� ĳ���Ͱ� �Ⱥ��ϰ��
	// �ذ�) �ش� �ɼ� Ȱ��ȭ
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
	
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*
	*  ������ �ǰ���(server-side rewind)�� ���� ��Ʈ �ڽ���
	*/
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	// �ǰ��� ���� üũ�� ���̰� ������ NoCollision���� ����
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), blanket);

	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), backpack);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	
	// ����) �÷��̾� �һ��� hp�ٰ� �˸°� ��µ��� ����
	// ����) �÷��̾� ��Ʈ�ѷ��� ���� ��� SetHUDHealth�� ȣ������ ����
	//     ) ������ BeginPlay�� BlasterController���� ���� ȣ��Ǽ� ���� ��
	// �ذ�) ��Ʈ�ѷ��� ĳ���Ϳ� ���ǵ� ������ PlayerController::OnPossess�� �̿�
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDGun();

	// ����) �÷��̾� �һ��� ���� ������ �۵����� ����
	// ����) BeginPlay���� ù ƽ�� PlayerState�� �������� ����.
	// �ذ�) PlayerState�� ������ �ξ� nullptr�� ��� Tick�Լ����� PlayerState �����ϱ�.

	if (BlasterPlayerController)
	{
		BlasterPlayerController->HideDeathMessage();
	}

	if (HasAuthority())
	{
		// OnTakeAnyDamage: Actor Ŭ���� FTakeAnyDamageSignatureŸ�� ��������Ʈ
		// ProjectileBullet::OnHit ���� ApplyDamage�� ���� ȣ��
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (Combat && Combat->EquippedWeapon) GetCharacterMovement()->bOrientRotationToMovement = false;
	if (Combat && Combat->EquippedWeapon) bUseControllerRotationYaw = true;

	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	// ENetRole::ROLE_SimulatedProxy -> ���� �������� �ʴ� ĳ����(��� �÷��̾�)
	// ���� ũ�ٴ� ���� ���� �����ϴ� �÷��̾�ų� �������� �������� �÷��̰ų�
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		// ���������� �����ϴ� ���� �÷��̾��� ���
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// �浹�� ���� ���� ���
	// DOREPLIFETIME_CONDITION( ������ ������ ���� Ŭ����, ����� ����, ���� )
	// COND_OwnerOnly -> ��ǻ�Ϳ��� Pawn�� �����ϴ� ���(Owner)���Ը� ����
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;

		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);

		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		// �ѱ� �������� �ٸ� ������ ���
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;

	return Velocity.Size();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	// �Ű������� ���� Material ������Ʈ�ϱ�
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// ��������Ʈ
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	
	// Ÿ�Ӷ����� Ŀ�갡 �ʿ�
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// HasAuthority() -> OnSphereOverlap() -> SetOverlappingWeapon()
	// ���� ȯ�濡�� �۵���.

	// ���� ����) ���� ĳ���Ͱ� ���� Ȱ��ȭ�� Ŭ�� ĳ���Ͱ� ������ ��Ȱ��ȭ ��Ű�� ����
	// ���� �ذ�) if (IsLocallyControlled())�� ���α�
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}
	
	OverlappingWeapon = Weapon;

	// ���� ���� Controll�ϰ� �ִ� BlasterCharacter�ΰ�?
	// ���� ȯ�濡�� �����ϴ� BlasterCharacter? -> ���� ������ ����ϴ� ������ ĳ����
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	// EquippedWeapon�� ���� �Ӽ��� �ƴϱ⿡ ��� Ŭ���̾�Ʈ�� nullptr�� �ȴ�.
	// EquippedWeapon�� CombatComponent::EquipWeapon���� �������ش�.(���� ȯ��)
	// ���� ȯ���̸� EquipWeapon�� ȣ���߰�, Ŭ��ȯ���̸� RPC�� ȣ���ߴ�.
	// Ŭ�󿡼� RPC�� ȣ���Ͽ� ���� ȯ�濡�� EquipWeapon�� �����Ѵ�.(���� ���ν����ϱ�)
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();

	return Combat->HitTarget;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterPlayerState == nullptr) return ETeam::ET_NoTeam;

	return BlasterPlayerState->GetTeam();
}

bool ABlasterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;

	return Combat->bLocallyReloading;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	// OverlappingWeapon�� �Ӽ����� ReplicatedUsing = OnRep_OverlappingWeapon�� ������.
	// OverlappingWeapon�� �������� Ŭ�����׷� ������ �Ǹ� �� �Լ��� ȣ���ϴµ� �Լ��� �������� ȣ������ �ʰ�(!!!), Ŭ�󿡼� ȣ���Ѵ�.
	// ���� �������� �� �̻� ���� �˾� â�� �ȶ��.
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	// ������) ��������Ʈ ������ �ƴ� �̻� ���� ���� ������ ���� ������ ���� ���Ѵ�.

	// LastWeapon�� ���� �浹�� ���⸦ �����ϱ� ���� ������ ����
	// OnSphereEndOverlap() -> SetOverlappingWeapon(nullptr)
	if (LastWeapon)
	{
		// ������ ������ ���Ⱑ �ִٸ� ���� �����
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	// Actor Ŭ�������� �������� ����ǰ� ������ ������ ȣ��Ǵ� �Լ�
	Super::OnRep_ReplicatedMovement();

	// ���� �� ƽ���� ȣ������ �ʰ� �������� ����ɶ��� ȣ���Ѵ�.
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f; // ��� �������� ���������Ƿ� Ÿ�̸� �ʱ�ȭ
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	// ���� ��忡�� ȣ��(���� ȯ��)
	DropOrDestroyWeapons();

	MulticastElim(bPlayerLeftGame);
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}

		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}

		if (Combat->TheFlag)
		{
			Combat->TheFlag->Dropped();
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	BlasterPlayerState->AddToScore(0.f);
	BlasterPlayerState->AddToDefeats(0);
	SetTeamColor(BlasterPlayerState->GetTeam());
	SetSpawnPoint();
}

void ABlasterCharacter::SetSpawnPoint()
{
	// ������ ���� ����Ʈ
	if (HasAuthority() && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == BlasterPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}
		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				ChosenPlayerStart->GetActorRotation()
			);
		}
	}
}

void ABlasterCharacter::Destroyed()
{
	// ������) ���� ȯ�濡���� ���� ������� Ŭ���̾�Ʈ������ �Ȼ������ ����
	// �ذ�) Destroyed���� ����(��� �÷��̾����׼� �����)

	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	// ���� ���� ���̸� ���� ����߸��� �� ���� ���¶�� ���� ���ش�.
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	// ������ Ŭ���̾�Ʈ ��ο��� ���� �ִϸ��̼��� ��µǾ���
	// ABlasterGameMode::PlayerEliminated -> Elim (���Ӹ�� = ����ȯ��)
	bLeftGame = bPlayerLeftGame;

	// ź�� 0���� ǥ���ϱ�
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		// BlasterPlayerController->SetHUDGunType("");
	}

	bElimmed = true;
	PlayElimMontage();

	// �ؽ�ó ������� ȿ��
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	StartDissolve();

	// ĳ���� ������ ����
	bDisableGameplay = true;

	// �߷����� ĳ���Ͱ� �ƾ��� �߶��ϴ� ���� ����
	GetCharacterMovement()->DisableMovement();

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	// �ݸ��� ����
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ���� ����Ʈ ����
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::ElimTimerFinished()
{
	// ��Ȱ��Ű��(����ȯ��)
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	// ���� ȯ��
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}

	// ���� ������(Ŭ�� ȯ��)
	if (bLeftGame && IsLocallyControlled())
	{
		// ReturnToMainMenu�� OnPlayerLeftGame�� ���ε��ؾ���.
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	
	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;

	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}

	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	// ĳ���� ȸ��
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	// ĳ���� �þ� ���Ͽ�����
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		if (Combat->bHoldingTheFlag) return;

		// ���� �߻�) ���ڸ����� ���� �ֿ�� ���� �������� ȸ��
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		if (Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquipButtonPressed();
		
		bool bSwap = Combat->ShouldSwapWeapons() && 
			!HasAuthority() && 
			Combat->CombatState == ECombatState::ECS_Unoccupied && 
			OverlappingWeapon == nullptr;

		if (bSwap)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	// Implementation: RPC�� �����ϸ� ��� �������
	// ������ ������ �����ϹǷ� ���� ȯ���̴�
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	// bIsCrouched�� Character Ŭ������ ���� �Ӽ� ����(��Ƽ�÷��� ����)
	if (bIsCrouched)
	{
		// ServerUpdateWidget(true);
		UnCrouch();
	}
	else
	{
		// ServerUpdateWidget(false);
		// Crouch �Լ����� �̼� ���϶� ĸ�� �ݸ����� �۰� ����� ������ ���ԵǾ��ִ�.
		Crouch();
	}
}

void ABlasterCharacter::Crouch(bool bClientSimulation)
{
	Super::Crouch();

	ServerUpdateWidget(false);
}

void ABlasterCharacter::UnCrouch(bool bClientSimulation)
{
	Super::UnCrouch();

	ServerUpdateWidget(true);
}

void ABlasterCharacter::ServerUpdateWidget_Implementation(bool visible)
{
	MulticastUpdateWidget(visible);
}

void ABlasterCharacter::MulticastUpdateWidget_Implementation(bool visible)
{
	if (OverheadWidget == nullptr) return;

	OverheadWidget->SetVisibility(visible);
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		if (Combat->bHoldingTheFlag) return;
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// �þ߿� ���� ĳ������ ��ݽ��� �����̵��� ���ֱ�(�� �����̱�)
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	// ������ ���� ���� �ش� ���� ����
	if (Speed == 0.f && !bIsInAir)
	{
		// ABP���� Root ���� ȸ��(��ݽ� �¿� ȸ��)
		bRotateRootBone = true;

		// ���콺�� ������ ���� ����
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		bUseControllerRotationYaw = true;

		// ���� �Ѱ�ġ���� ������ �� ���� �ݴ������� ���� ���ư��� ������
		TurnInPlace(DeltaTime);
	}
	
	// ������ �� ���� �����ϱ�
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;

		// ���� ������ ����(ù ���� �������� ���콺 ȸ�� ���� ��� ����)
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	// ���� �߻�) Ŭ���̾�Ʈ �÷��̾ ���� �Ʒ��� ���̸� �ٸ� �÷��̾����� �� �ൿ�� �Ⱥ��δ�.
	// ����) ������ ��ŷ�Ͽ� ������ ���� ��, ��ŷ�ϴ� �������� ������ ����
	// �ڵ�) CompressAxisToShort(float Angle) {	return FMath::RoundToInt(Angle * 65536.f / 360.f) & 0xFFFF;	}
	// ������ INT���� �ִ밪���� ������ϰ�, 360�� �ʰ��ϴ� ���� ���� ����ŷ ó���Ѵ�.
	// ����ŷ ó���� ���� ��� ���� ���� ������� �ȴ�. (-90 -> 270)
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}

	// UE_LOG(LogTemp, Warning, TEXT("AO_Pitch: %f"), AO_Pitch);
}

void ABlasterCharacter::SimProxiesTurn()
{
	// Ŭ��-Ŭ�� ȸ�� �ִϸ��̼� �ε巴�� ������ֱ�
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		// �޸��� �߿� ȸ�� �ִϸ��̼��� ��¾ȵǵ��� ��
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	
	// UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);
	
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	// �ֱ� ������ ���ķ� ȸ�� �Ѱ�ġ�� ���� �ʾҴٸ�
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	// ��ũ���� �� ���� Ű ������ ��ũ���� ����
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}
void ABlasterCharacter::FireButtonReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (AnimInstance->IsAnyMontagePlaying()) return;

	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ABlasterCharacter::UpdateHUDGun()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->SetHUDGun();
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	// ���� ���� ���� ȯ��
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;

		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (bElimmed || BlasterGameMode == nullptr) return;
	
	// �Ʊ��̸� �������� 0���� ó��
	Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			// ���� ������ ����
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}
	// Health�� ���� �Ӽ����� ���� �� ������ OnRep_Health�� ȣ��
	// OnRep_Health�� Ŭ���̾�Ʈ������ ȣ��ȴ�.
	// �׷��� ReceiveDamage�� ���������� ȣ��ȴ�.(BeginBlay->HasAuthority)
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	// ��Ƽĳ��Ʈ RPC�� ����ϴ� ��� ����ٰ� ����Ѵ�.
	// ReceiveDamage�� ���������� ����ǹǷ� Ŭ���̾�Ʈ�� �ɾ�������Ѵ�.
	// ��︮�� ���� OnRep_Health�̴�.
	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	// �ǰ� ������ ��Ȳ������ �´� �ִϸ��̼� ���
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
	
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	// �� ĳ���Ͱ� ���εǾ �þ߸� �������(���� ������ ���� ��)
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			// ���� �����ڿ��Ը� �Ⱥ��̰� �ϱ�
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}

		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}

		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::PollInit()
{
	// BeginPlay ���� PlayerState�� �׻� ���õ��� �ʴ´�.
	// 2~3 ������ ������ ���� ���� ���� Ȯ���� �����Ƿ� ���⼭ ó��
	// Tick���� ȣ�� -> PlayerState�� nullptr�� �ƴ� ������ ����
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			OnPlayerStateInitialized();

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

void ABlasterCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr || OriginalMaterial == nullptr) return;

	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMatInst;
		break;
	}
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr) return false;
	return Combat->bHoldingTheFlag;
}

void ABlasterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat == nullptr) return;

	Combat->bHoldingTheFlag = bHolding;
}

void ABlasterCharacter::ClientSetName_Implementation(const FString& Name)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController != nullptr)
	{
		PlayerController->PlayerState->SetPlayerName(Name);
	}
}

void ABlasterCharacter::ServerSetPlayerName_Implementation(const FString& PlayerName)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	
	if (PlayerController)
	{
		PlayerController->PlayerState->SetPlayerName(PlayerName);
		ClientSetName(PlayerName);
	}
}
