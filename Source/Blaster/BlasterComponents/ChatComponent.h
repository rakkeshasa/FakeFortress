// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChatComponent.generated.h"

struct FChatMessage;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	void RegisterChatBox(class UChatBox* NewChatBox);

protected:
	virtual void BeginPlay() override;

public:	
	void InitChatComponent();
	FLinearColor ChatColor = FLinearColor::White;

	UPROPERTY()
	UChatBox* ChatBox;

	/*UPROPERTY(EditAnywhere, Category = "Chat")
	class UInputAction* ChatAction;*/

	void RandomizeChatColor();

	UFUNCTION()
	void OnChatActionPressed();

	void FocusChatBox();

	UFUNCTION()
	void SendMessage(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION(Server, Reliable)
	void ServerSendMessage(const FChatMessage& ChatMessage, FLinearColor TextColor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReceiveMessage(const FChatMessage& ChatMessage, FLinearColor TextColor);
		
};
