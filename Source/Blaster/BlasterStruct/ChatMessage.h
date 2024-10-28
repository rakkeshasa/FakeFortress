#pragma once

#include "CoreMinimal.h"
#include "ChatMessage.generated.h"


USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

	FChatMessage() : Sender(TEXT("")), Message(TEXT(""))
	{

	}

	FChatMessage(const FString& InSender, const FString& InMessage)
		: Sender(InSender), Message(InMessage)
	{

	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString Sender;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chat")
	FString Message;
};