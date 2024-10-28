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
서버측 재조정을 통해 얻은 결과값을 FServerSideRewindReuslt을 통해 서버는 ApplyDamage를 실행하고 헤드샷이면 헤드샷 데미지를, 그게 아니라면 일반 데미지를 ApplyDamage에 적용했습니다.</br>
