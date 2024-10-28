// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatTextBlock.generated.h"

struct FChatMessage;
/**
 * 
 */
UCLASS()
class BLASTER_API UChatTextBlock : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetMessage(const FChatMessage& ChatMessage);
	void SetColor(const FLinearColor& Color);

private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MessageText;
};
