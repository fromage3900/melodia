// Native quest manager with starter quests, progress tracking, and HUD hooks.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaQuestTypes.h"
#include "MelodiaQuestManagerBase.generated.h"

class UMelodiaInventoryComponent;
class UMelodiaRhythmHUDWidget;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaQuestManagerBase : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaQuestManagerBase();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 CompletedBattleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	bool bLastBattleWonNotified = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	TArray<FMelodiaQuestDefinition> QuestCatalog;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	TArray<FMelodiaQuestProgress> QuestProgress;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 ActiveQuestCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 CompletedQuestCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	FString LastQuestToast;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Quest")
	void NotifyBattleWon();
	virtual void NotifyBattleWon_Implementation();

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void RegisterStarterQuests(const FVector& SongGateLocation);

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyReachedSongGate();

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyPerfectNotesInSkill(int32 PerfectCount);

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyEnemyBroken();

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyUltimateVictory();

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyItemCollected(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void SyncHUD(UWorld* World) const;

	UFUNCTION(BlueprintPure, Category="Melodia|Quest")
	TArray<FMelodiaMinimapMarker> BuildQuestMarkers() const;

	/**
	 * Rebuild quest markers using PCG data. Called when PCG is regenerated.
	 * Quests with AssociatedPCGRole set will auto-discover positions.
	 */
	UFUNCTION(BlueprintCallable, Category="Melodia|Quest")
	void NotifyPCGRebuilt();

protected:
	void AddQuestDefinition(const FMelodiaQuestDefinition& Definition);
	FMelodiaQuestProgress* FindProgress(FName QuestId);
	const FMelodiaQuestDefinition* FindDefinition(FName QuestId) const;
	void ActivateQuest(FName QuestId);
	void AdvanceQuest(FName QuestId, int32 Delta = 1);
	void CompleteQuest(FName QuestId);
	void GrantQuestRewards(FName QuestId, APawn* PlayerPawn);
	void RefreshCounts();
	void PushQuestToast(const FString& ToastText) const;
};
