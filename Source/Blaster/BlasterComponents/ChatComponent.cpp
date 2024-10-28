// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatComponent.h"
#include "Blaster/HUD/ChatBox.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerState.h"
#include "Blaster/BlasterStruct/ChatMessage.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/HUD/ChatBox.h"

void UChatComponent::RegisterChatBox(UChatBox* NewChatBox)
{
	ChatBox = NewChatBox;
	ChatBox->GetChatEntryBox()->OnTextCommitted.AddDynamic(this, &UChatComponent::SendMessage);
}

void UChatComponent::BeginPlay()
{
	Super::BeginPlay();

	InitChatComponent();
	RandomizeChatColor();
}

void UChatComponent::InitChatComponent()
{
	const ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetOwner());
	//UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	//EnhacedInputCoponent->BindAction(ChatAction, ETriggerEvent::Started, this, &UChatComponent::OnChatActionPressed);
	
	UInputComponent* InputComponent = PlayerController->InputComponent;

	if (InputComponent)
	{
		InputComponent->BindAction("Chat", IE_Pressed, this, &UChatComponent::OnChatActionPressed);
	}
}

void UChatComponent::RandomizeChatColor()
{
	ChatColor = FLinearColor(FMath::FRandRange(0.0f, 1.0f), FMath::FRandRange(0.0f, 1.0f), FMath::FRandRange(0.0f, 1.0f));
}

void UChatComponent::OnChatActionPressed()
{
	FocusChatBox();
}

void UChatComponent::FocusChatBox()
{
	if (ChatBox)
	{
		ChatBox->GetChatEntryBox()->SetKeyboardFocus();
	}
}

void UChatComponent::SendMessage(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (!ChatBox) return;

	const FString Message = ChatBox->GetChatEntryBox()->GetText().ToString();
	if (Message.IsEmpty() && CommitMethod != ETextCommit::OnEnter) return;

	FString PlayerName = *Cast<ABlasterPlayerController>(GetOwner())->GetPlayerState<ABlasterPlayerState>()->GetPlayerName();
	if (PlayerName.IsEmpty())
	{
		PlayerName = "Player";
	}

	FChatMessage ChatMessage;
	ChatMessage.Sender = PlayerName;
	ChatMessage.Message = Message;

	if (!ChatMessage.Message.IsEmpty())
	{
		ServerSendMessage(ChatMessage, ChatColor);
	}

	ChatBox->GetChatEntryBox()->SetText(FText::GetEmpty());
	ChatBox->GetChatEntryBox()->SetClearKeyboardFocusOnCommit(false);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
	{
		const FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
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