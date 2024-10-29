# FakeFortress

![main](https://github.com/user-attachments/assets/9239b301-9e84-46f9-9b81-58c306b5660c)

Unreal Engine과 Steam Session을 이용한 멀티플레이 FPS 게임
</BR>

목차
---
- [간단한 소개](#간단한-소개)
- [플레이 영상](#플레이-영상)
- [기능 구현](#기능-구현)
- 

## 간단한 소개
![play](https://github.com/user-attachments/assets/f0393b7c-e9db-4e97-aae6-5efa71ab02f7)

</BR>
멀티플레이 FPS 게임인 Fake Fortress입니다.</BR>

게임 모드는 개인전, 단체전, 탈취전이 있으며 메인 메뉴에서 모드를 선택하고 게임을 생성할 수 있습니다.</br>
게임을 생성하는 유저는 리슨 서버 유저가 되어 서버를 호스트하게 되고 나머지 참여자들은 호스트 서버에 참여하여 게임을 진행하게 됩니다.</br>
게임 시작시 기본 무기로 권총이 주어지며, 맵을 돌아다니며 다양한 총기들을 파밍할 수 있습니다.</br>
총기를 파밍해 더 강력한 총을 가지고 여러 버프를 먹으면서 상대 플레이어를 최대한 많이 처치하는 게임입니다.</br>
</br>

## 플레이 영상
[![Video Label](http://img.youtube.com/vi/EDj4PKdRmoo/0.jpg)](https://youtu.be/EDj4PKdRmoo)
</br>
👀Link: https://youtu.be/EDj4PKdRmoo</BR>
이미지나 주소 클릭하시면 영상을 보실 수 있습니다. </BR>

## 기능 구현

### [세션 구현]
Steam의 매치메이킹 기능을 사용하여 서버의 IP를 알아올 필요 없이 접속이 가능하도록 했습니다.</BR>
<STRONG>Unreal Engine Subsystem</STRONG>은 언리얼 엔진에서 Steam의 매치메이킹 기능을 사용할 수 있게 추상 레이어를 제공합니다.</br>
<STRONG>Online Subsystem</STRONG>은 매치메이킹 세션을 지원하는 자체 서비스가 있으며 이러한 서비스를 처리하도록 설계된 Session Interface가 포함되어 있습니다.</br>
<STRONG>세션 인터페이스(Session Interface)</STRONG>는 게임 세션을 생성, 관리, 파괴하는 것을 담당하며 세션 검색과 메치메이킹 기능 검색을 통해 특정 세션에 접속할 수 있도록 해줍니다.</br>

```
// OnlineSubsystem 클래스
const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
{
  MultiplayerOnCreateSessionComplete.Broadcast(false);
}

// MainMenu 클래스
UWorld* World = GetWorld();
if (World)
{
	World->ServerTravel(PathToLobby);
}
```
메인 메뉴에서 '서버 생성하기' 버튼을 누르면 세션 생성을 시도하게 됩니다.</br>
LocalPlayer는 '서버 생성하기'를 누른 플레이어의 컨트롤러 값을 가지고 있으며 해당 컨트롤러를 통해 플레이어의 고유 네트워크 ID를 GetPreferredUniqueNetId 함수를 통해 알 수 있습니다.</br>
CreateSession함수에 서버 플레이어의 IP, 세션 이름, 세션 세팅값이 입력값으로 들어가고 세션 생성을 요청합니다.</BR>
세션이 성공적으로 생성되면 CreateSessionCompleteDelegate를 통해 콜백함수를 호출하여 true값을 브로드캐스트 해주고 세션 생성이 실패라면 바로 false를 메인 메뉴의 위젯에게 브로드캐스트합니다.</br>
메인 메뉴 위젯은 세션 생성 성공의 의미로 true값을 받으면 ServerTravel 함수를 통해 서버 유저를 게임 레벨로 이동시키게 됩니다.</br></br>

```
// OnlineSubsystem 클래스
FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
{
  MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
}

// MainMenu 클래스
for (auto Result : SessionResults)
{
	FString SettingsValue;
	Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
	if (SettingsValue == MatchType)
	{
		MultiplayerSessionsSubsystem->JoinSession(Result);
		return;
	}
}
```
세션에 접속할 클라이언트 플레이어는 메인 메뉴에서 '서버 참여하기' 버튼을 누르면 세션 검색을 시작하게됩니다.</br>
클라이언트 플레이어의 IP 주소를 가져와 Session Interface클래스의 FindSessions를 실행하며 검색 조건에 만족하는 세션은 SessionSearch 타입 변수인 LastSessionSearch에 저장이됩니다.</br>
담겨진 세션은 FindSessionsCompleteDelegate의 콜백함수에서 브로드캐스트 되며 실패 시 빈 배열 값과 실패했다는 의미의 false값을 브로드캐스트합니다.</br>
메인 메뉴 위젯은 찾은 세션들과 true값을 받으면 세션들을 순회하며 게임모드인(개인전, 단체전, 탈취전) 'MatchType'의 값이 서버와 같다면 세션 접속을 시도합니다.</br>

```
FString Address;
SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
if (PlayerController)
{
	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}
```
Online Subsystem 클래스에서 JoinSession을 통해 세션에 성공적으로 들어가 true값을 메인 메뉴에게 브로드캐스트하면 MainMenu 클래스는 GerResolvedConnectString을 통해 클라이언트측 IP 주소를 얻어옵니다.</BR>
클라이언트의 컨트롤러를 통해 클라이언트 IP주소를 가진채 ClientTravel을 통해 서버 플레이어의 세션에 참가하게 됩니다.</br></br>

### [총기 줍기]

```
void ABlasterCharacter::EquipButtonPressed()
{
		if (Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquipButtonPressed();
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
  EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);

	EquippedWeapon->SetHUDAmmo();
	EquippedWeapon->SetHUDGun();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(WeaponToEquip);
	ReloadEmptyWeapon();
}

```

플레이어가 땅에 떨어진 총을 파밍하면 서버 환경에서 해당 플레이어에게 총을 장착시켜주고 다른 클라이언트에게 해당 내용을 복제해 전달해줍니다.</br>
우선 플레이어가 장착 버튼을 누르면 서버 RPC를 통해 서버 환경에서 해당 플레이어의 ActorComponent인 CombatComponent에 접근해 EquipWeapon함수를 실행합니다.</br>
EquipWeapon은 주운 총의 상태와 주인을 세팅하고 플레이어의 오른손에 부착되며 부착과 동시에 화면상에 출력되는 총기의 종류와 총알의 개수가 바뀌게 됩니다.</br>
EquippedWeapon에는 Relicated 속성을 부여해 서버 환경에서 장착 버튼을 누른 플레이어에게 무기를 쥐어주면 클라이언트 환경에서도 장착된 총을 복제해 클라이언트에게 전달해줄 수 있게 했습니다.</br>

```
void UCombatComponent::OnRep_EquippedWeapon()
{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);

		PlayEquipWeaponSound(EquippedWeapon);
		EquippedWeapon->SetHUDAmmo();
		EquippedWeapon->SetHUDGun();
}
```
장착된 총인 EquippedWeapon이 복제되면 콜백함수를 통해 클라이언트 환경에서도 플레이어에게 총기를 쥐어주게 되며, HUD도 장착한 총에 맞는 내용이 출력되도록 했습니다.</BR>
복제 속성과 RPC를 통해 서버 환경에서 클라이언트 플레이어에게 총을 장착시키고 장착된 총을 복제시켜 다른 모든 클라이언트 환경에서도 해당 플레이어가 총을 장착한 모습이 보이도록 했습니다.</BR></BR>

### [격발하기]

```
if (CanFire())
{
  bCanFire = false;

  FHitResult HitResult;
  TraceUnderCrosshairs(HitResult);
  HitTarget = HitResult.ImpactPoint;

  if (!Character->HasAuthority()) LocalFire(HitTarget);
  ServerFire(HitTarget, EquippedWeapon->FireDelay);

  StartFireTimer();
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}
```

격발 버튼이 눌리면 Fire 함수가 호출되며 Fire 함수는 격발 간 간격을 주기 위한 타이머와 격발을 처리하기 위해 RPC를 이용합니다.</BR>
FireTimer는 총기가 가지고 있는 연사 딜레이 시간에 따라 bCanFire값을 세팅해주며 자동화기 무기일 경우 다시 Fire 함수를 호출하여 자동으로 총알이 나가도록 해줬습니다.</br>
서버 유저가 아닌 클라이언트 유저가 격발 버튼을 누르면 즉시 격발 애니메이션이 재생되도록하고 다른 클라이언트나 서버 환경에서는 RPC를 통해 격발 애니메이션이 출력되도록 하여 핑이 높은 상황에서도 격발 키를 누르면 자신의 화면에서는 바로 애니메이션이 재생됩니다.</BR>
격발한 클라이언트를 제외한 나머지 플레이어들은 ServerFire RPC를 통해 서버 환경에서 멀티캐스트 RPC인 MulticastFire를 호출하고 서버와 클라이언트 모두의 환경에서 플레이어가 격발을 한 모습을 볼 수 있게 했습니다.</br>
격발 후 데미지 처리는 Weapon클래스의 Fire함수가 담당하며 데미지는 서버 환경에서 총구위치와 HitTarget을 라인트레이싱한 후 플레이어한테 명중했다면 데미지를 적용시킵니다.</br>
리슨 서버 유저면 라인트레이싱 결과를 토대로 바로 ApplyDamage를 실행해 피격자한테 데미지를 주지만, 클라이언트 유저면 ServerScoreRequest RPC를 통해 서버환경에서 서버측 재조정를 거쳐 라인 트레이싱을 진행하게 됩니다.</br></br>

### [서버측 재조정]

```
void ULagCompensationComponent::SaveFramePackage()
{
	float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	while (HistoryLength > MaxRecordTime)
	{
		FrameHistory.RemoveNode(FrameHistory.GetTail());
		HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
	}

	FFramePackage Package;
	Package.Time = GetWorld()->GetTimeSeconds();
	for (auto& BoxPair : Character->HitCollisionBoxes)
	{
		FBoxInformation BoxInformation;
		BoxInformation.Location = BoxPair.Value->GetComponentLocation();
		BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
		Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
	}
}
```
SaveFramePackage는 서버측 재조정을 할 때 사용할 과거 시간대의 플레이어 히트박스를 저장하는 역할을 하는 함수입니다.</br>
FFramePackage는 히트박스를 저장한 시간대와 플레이어 히트박스를 갖는 구조체이며 매 틱마다 새로운 FFRamePackage를 저장하기 위해 연결리스트를 이용해 FrameHistory를 만들었습니다.</br>
FrameHistory에 저장된 시간이 서버측 재조정에 사용할 시간인 MaxRecordTime보다 길다면 뒤에서부터 오래된 노드를 삭제하고 새로운 시간대의 FFramePackage를 저장합니다.</br>
새로운 시간대의 FrameHistory 노드를 저장시 FFramePackage에 현재 틱의 시간과 플레이어가 갖고 있는 히트박스를 순회하여 모든 히트박스의 위치와 회전을 기록합니다.</br></br>


```
FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr) break;

		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	return FrameToCheck;
}
```
GetFrameToCheck는 클라이언트 환경에서 격발을 해 Weapon 클래스의 Fire을 호출하게 될 시 ServerScoreRequest RPC가 제일 먼저 실행하는 함수로 되돌린 시간대에 있는 피격자의 히트박스를 가져옵니다.</br>
History 변수는 피격자의 FFramePackage가 저장된 연결리스트인 FrameHistory로 지난 시간대의 히트박스가 저장되어 있습니다.</br>
FrameHistory는 매 틱마다 FFramePackage를 저장하므로 피격된 시간대랑 저장된 시간대가 일치할 확률이 적으며 일치하지 않을 경우 히트 박스의 위치와 회전을 보간해줘야합니다.</br>
우선 격발한 플레이어가 상대 플레이어를 명중시킨 시간대가 어느 FFramePacakge 사이에 있는지 찾기 위해 While문을 통해 명중시킨 시간대의 바로 앞과 뒤의 노드를 찾습니다.</br>
찾은 노드의 시간대가 상대 플레이어를 명중시킨 시간대와 같다면 찾은 FFramePackage를 반환하고 그렇지 않다면 InterpBetweenFrames를 통해 두 개의 FFramePackage 사이를 비율에 따라 보간하여 반환합니다.</br>

```
FFramePackage CurrentFrame;
for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
{
	FBoxInformation BoxInfo;
	BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
	BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
	CurrentFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
}

for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
{
	HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
	HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
}
```

피격된 시간대의 FFramePackage를 구한 후에는 ConfirmHit 함수를 호출해 피격자의 히트박스에 맞았는지 라인트레이싱을 진행합니다.</br>
라인 트레이싱을 진행하기 전에 우선 현재 피격자의 히트 박스의 정보를 저장하고 과거 시간의 FFramePacakge의 정보를 토대로 히트박스를 이동시킵니다.</br>
첫번째 for문은 피격자인 HitCharacter의 현재 히트 박스를 CurrentFrame에 저장하고, 두번째 for문은 GetFrameToCheck에서 구한 피격 시간대의 FramePackage의 정보를 토대로 피격자의 히트박스를 세팅하고 있습니다.</br>

```
World->LineTraceSingleByChannel(
	ConfirmHitResult,
	TraceStart,
	TraceEnd,
	ECC_HitBox
);

if (ConfirmHitResult.bBlockingHit)
{
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	
	return FServerSideRewindResult{ true, true };
}
else 
{
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	World->LineTraceSingleByChannel(
		ConfirmHitResult,
		TraceStart,
		TraceEnd,
		ECC_HitBox
	);

	if (ConfirmHitResult.bBlockingHit)
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		
		return FServerSideRewindResult{ true, false };
	}
}
```

피격 위치가 머리 히트박스에 맞았는지 체크하기 위해 피격자 캐릭터의 Mesh콜리전을 해제하고 머리 히트박스의 콜리전만 활성화시켜 먼저 라인트레이싱을 진행합니다.</br>
if문의 ConfirmHitResult.bBlockingHit조건에 만족한다면 머리 히트박스만 콜리전을 활성화시켰기때문에 헤드샷으로 판정내리기 위해 FServerSideRewindResult에 bHeadShot값으로 true와 bHitConfirmed값으로 true를 리턴해줬습니다.</br>
만약 if문의 조건에 만족하지 않으면 머리에 맞지 않았으므로 나머지 히트박스의 콜리전을 활성화시켜 라인 트레이싱을 한 번 더 진행했습니다.</br>
다시 진행한 라인 트레이싱에서 bBlockingHit이 true라면 머리에 맞지 않고 몸에 맞은 샷이므로 false와 true값을 반환했습니다.</br>
명중 판정이 끝났다면 과거 시간대에 있는 피격자의 히트박스를 SetWorldLocation와 SetWorldRotation을 통해 다시 현재 시간대의 위치와 회전으로 복구시켰습니다.</br>
서버측 재조정을 통해 얻은 결과값을 FServerSideRewindReuslt을 통해 서버는 ApplyDamage를 실행하고 헤드샷이면 헤드샷 데미지를, 그게 아니라면 일반 데미지를 ApplyDamage에 적용했습니다.</br></br>

```
if (PlayerState)
{
	if (PlayerState->GetPing() * 4 > HighPingThreshold)
	{
		HighPingWarning();
		ServerReportPingStatus(true);
	}
	else
	{
		ServerReportPingStatus(false);
	}
}
```
높은 핑으로 인해 지연이 너무 오래 될시 서버측 재조정은 공격자에게 좋은 플레이 경험을 주지 않기 때문에 일정 핑보다 높다면 서버측 재조정을 사용하지 않게 해주도록 했습니다.</BR>
PlayerController에서 매 틱마다 PlayerState를 통해 핑을 체크하고 HighPingThreshold보다 높다면 경고 화면과 ServerReportPingStatus를 통해 bool값을 브로드캐스트 했습니다.</br>
브로드 캐스트 해주는 HighPingDelegate 델리게이트는 무기를 장착할 때 바인딩해주고 무기를 바닥에 떨어뜨리면 해제시키도록 설계했습니다.</br>
HighPingDelegate를 통해 콜백되는 함수에서는 bool값에 따라 무기 옵션인 bUseServerSideRewind의 값을 세팅해 현재 네트워크 환경에 따라 서버측 재조정을 사용할지 결정했습니다.</br></br>

### [클라이언트측 예측]
지연 시간이 긴 환경에서 총을 격발한 후에 HUD에서 남은 총알의 수가 클라이언트 환경에 맞게 줄었다가 아직 서버환경에서 복제되지 않아 늘어나는 문제가 있었습니다.</BR>
또한 남은 총알의 수가 늦게 복제되면서 총알을 다 소모하면 Reload함수를 호출해야하는데 정상적으로 호출되지 않아 자동재장전이 되지 않았습니다.</br>
이를 해결하기 위해서 더 이상 총알 수를 담당했던 Ammo 변수를 복제 속성으로 두지 않고 클라이언트에서 직접 Ammo를 소모하고 모든 Ammo가 소진 시 바로 Reload가 실행되도록 했습니다.</br>

```
void AWeapon::SpendAmmo()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();

	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else if (BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled())
	{
		++Sequence;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;

	Ammo = ServerAmmo;
	--Sequence;

	Ammo -= Sequence;
	SetHUDAmmo();
}
```

클라이언트 플레이어는 격발하면 SpendAmmo를 통해 HUD상에 남은 총알을 서버의 응답을 기다릴 필요 없이 즉시 화면에 업데이트 할 수 있게됐습니다.</br>
대신 클라이언트가 갖고 있는 총알의 수가 옳은 지 검증하기 위해서 서버 환경에서 ClientUpdateAmmo RPC를 통해 확인할 수 있도록 했습니다.</BR></BR>

코드의 전체적인 흐름은 클라이언트가 2발을 소모하여 8발이 남았다면 서버의 응답을 받기 전에 Ammo값을 계산하여 남은 총알이 8발이라는 것을 예측을 하고 예측한 탄알 만큼 Sequence를 더해줍니다.</br>
서버에서는 소모된 1발씩 ClientUpdateAmmo RPC를 통해 클라이언트 환경에서 플레이어가 올바른 총알 수를 갖고 있는지 체크하며 서버는 지연시간으로 인해 플레이어가 9발을 갖고 있는 상황입니다.</br>
클라이언트의 Ammo값이였던 8발을 서버의 Ammo값인 9발로 바꾸게 되어 다시 클라이언트의 HUD에는 남은 총알이 8발이 아닌 9발로 출력이 되어 탄을 소모했음에도 적용되지 않은 상태입니다.</BR>
이를 해결하기 위해 Sequence값을 이용해 서버로부터 받을 응답 수가 몇 개 남았는지 체크하여 예측했던 8발로 다시 클라이언트의 Ammo값을 재조정해줬습니다.</br></br>

```
void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled()) HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	}
}
```
클라이언트가 Ammo값을 예측하면서 모든 총알을 소모하면 제때 Reload를 호출할 수 있게 됐습니다.</br>
기존 Reload 함수에서는 ServerReload RPC를 통해 HandleReload를 호출해 서버의 응답이 와야 재장전 애니메이션이 출력됐습니다.</br>
즉각적인 재장전 애니메이션을 출력하기 위해 RPC를 통하지 않고 바로 HandleReload를 호출했으며 재장전 중 사격이 되지 않도록 bLocallyReloading 변수로 조건을 추가했습니다.</br>
서버 환경이나 다른 플레이어들한테는 RPC와 콜백 함수를 통해 애니메이션이 출력되고 자기 자신의 캐릭터는 재장전 동작을 반복하지 않기 위해 제외시켰습니다.</BR></br>

### [시간 동기화]
게임에 참여한 플레이어들이 남은 시간을 동일하게 확인하기 위해서 서버 시간을 기준으로 했습니다.</BR>
클라이언트는 5초마다 RPC를 통해 서버한테 시간을 알아와서 자신의 시간을 맞추게해서 모든 플레이어들이 동일한 시간을 확인할 수 있게했습니다.</BR>

```
void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}
```

클라이언트가 서버의 시간을 확인하기 위해 자신이 요청을 보낸 시간과 함께 ServerRequestServerTime RPC를 호출해 서버 환경에서 현재 시간을 확인합니다.</BR>
서버는 다시 클라이언트 측으로 클라이언트에게 요청 받은 시간과 현재 서버 시간을 RPC를 통해 보내주고 클라이언트는 응답받은 시간에서 요청한 시간을 빼 RTT(Round Trip Time)을 계산합니다.</br>
서버가 자신의 시간을 클라이언트한테 보내는데 1/2 RTT만큼 시간이 소모됐으므로 클라이언트에서 서버의 시간은 서버 시간 + 1/2 RTT로 계산하여 클라이언트의 시간을 조정했습니다.</BR>

### [채팅 시스템]

```
void UChatComponent::SendMessage(const FText& Text, ETextCommit::Type CommitMethod)
{
	const FString Message = ChatBox->GetChatEntryBox()->GetText().ToString();
	FString PlayerName = *Cast<ABlasterPlayerController>(GetOwner())->GetPlayerState<ABlasterPlayerState>()->GetPlayerName();

	FChatMessage ChatMessage;
	ChatMessage.Sender = PlayerName;
	ChatMessage.Message = Message;

	if (!ChatMessage.Message.IsEmpty())
	{
		ServerSendMessage(ChatMessage, ChatColor);
	}
}

void UChatComponent::ServerSendMessage_Implementation(const FChatMessage& ChatMessage, FLinearColor TextColor)
{
	TArray<AActor*> PlayerControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), PlayerControllers);

	for (AActor* PlayerController : PlayerControllers)
	{
		ABlasterPlayerController* BlasterController = Cast<ABlasterPlayerController>(PlayerController);
		if (BlasterController)
		{
			BlasterController->GetChatComponent()->MulticastReceiveMessage(ChatMessage, TextColor);
		}
	}
}

void UChatComponent::MulticastReceiveMessage_Implementation(const FChatMessage& ChatMessage, FLinearColor TextColor)
{
	if (ChatBox && Cast<ABlasterPlayerController>(GetOwner())->IsLocalController())
	{
		ChatBox->AddChatMessage(ChatMessage, TextColor);
	}
}
```

채팅 위젯의 TextBox에 글을 적고 엔터를 누르면 OnTextCommitted 델리게이트를 통해 SendMessage가 호출됩니다.</br>
SendMessage에서는 델리게이트를 통해 받은 Text를 ChatMessage 구조체의 Message변수에 저장하고 현재 플레이어의 이름을 Sender 변수에 저장해 ChatMessage를 ServerSendMessage RPC에게 넘깁니다.</BR>
ServerSendMessage는 플레이어가 보낸 채팅 내용을 다른 모든 플레이어에게 보여주기 위해 서버에 접속한 모든 플레이어 컨트롤러에 접근해 MulticastReceiveMessage RPC를 호출합니다.</BR>
MulticastReceiveMessage를 통해 모든 클라이언트는 자신의 채팅 위젯의 ChatBox에 다른 플레이어가 보낸 채팅을 볼 수 있게 했습니다.</br>
</br>

