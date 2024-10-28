// Fill out your copyright notice in the Description page of Project Settings.

#include "ChatTextBlock.h"
#include "Components/TextBlock.h"
#include "Blaster/BlasterStruct/ChatMessage.h"

void UChatTextBlock::SetMessage(const FChatMessage& ChatMessage)
{
	if (MessageText)
	{
		const FString Message = FString::Printf(TEXT("%s : %s"), *ChatMessage.Sender, *ChatMessage.Message);
		MessageText->SetText(FText::FromString(Message));
	}
}

void UChatTextBlock::SetColor(const FLinearColor& Color)
{
	if (MessageText)
	{
		MessageText->SetColorAndOpacity(Color);
	}
}
