// Native compatibility base for Melodia quest hooks used by battle Blueprints.

#include "MelodiaQuestManagerBase.h"

void AMelodiaQuestManagerBase::NotifyBattleWon_Implementation()
{
	++CompletedBattleCount;
	bLastBattleWonNotified = true;
	UE_LOG(LogTemp, Log, TEXT("Melodia quest manager received NotifyBattleWon."));
}
