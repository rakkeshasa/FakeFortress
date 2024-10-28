// Fill out your copyright notice in the Description page of Project Settings.

#include "ChatBox.h"
#include "ChatTextBlock.h"
#include "Components/ScrollBox.h"
#include "Blaster/BlasterComponents/ChatComponent.h"

void UChatBox::AddChatMessage(const FChatMessage& InChatMessage, const FLinearColor& TextColor)
{
	if (!ChatMessageContainer && !ChatTextBoxClass) return;

	UChatTextBlock* ChatTextBlock = CreateWidget<UChatTextBlock>(this, ChatTextBoxClass);
	ChatTextBlock->SetMessage(InChatMessage);
	ChatTextBlock->SetColor(TextColor);
	ChatMessageContainer->AddChild(ChatTextBlock);

	if (ChatMessageContainer->GetChildrenCount() > 5)
	{
		ChatMessageContainer->ScrollToEnd();
	}
}

void UChatBox::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (UChatComponent* ChatComponent = PlayerController->FindComponentByClass<UChatComponent>())
		{
			ChatComponent->RegisterChatBox(this);
		}
	}
}
