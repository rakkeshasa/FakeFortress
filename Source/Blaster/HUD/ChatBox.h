// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatBox.generated.h"

/**
 * 
 */
class UEditableTextBox;
struct FChatMessage;

UCLASS()
class BLASTER_API UChatBox : public UUserWidget
{
	GENERATED_BODY()

public:
	FORCEINLINE UEditableTextBox* GetChatEntryBox() const { return ChatEntryBox; }

	void AddChatMessage(const FChatMessage& InChatMessage, const FLinearColor& TextColor);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ChatMessageContainer;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* ChatEntryBox;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ChatTextBoxClass;
};
