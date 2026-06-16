// Native compatibility base for Melodia quest hooks used by battle Blueprints.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaQuestManagerBase.generated.h"

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaQuestManagerBase : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 CompletedBattleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	bool bLastBattleWonNotified = false;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Quest")
	void NotifyBattleWon();
	virtual void NotifyBattleWon_Implementation();
};
