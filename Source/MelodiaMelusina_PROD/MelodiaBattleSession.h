// Authoritative battle encounter orchestrator (combat kernel entry point).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaBattleTypes.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaBattleSession.generated.h"

class UMelodiaRhythmHUDWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMelodiaBattlePhaseChanged, EMelodiaBattlePhase, NewPhase, EMelodiaBattlePhase, PreviousPhase);

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaBattleSession : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Melodia|Battle Session")
	FMelodiaBattlePhaseChanged OnBattlePhaseChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Battle Session")
	EMelodiaBattlePhase BattlePhase = EMelodiaBattlePhase::None;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Battle Session")
	EMelodiaHUDMode HUDMode = EMelodiaHUDMode::Exploration;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Battle Session")
	TObjectPtr<AActor> ActiveBattleController;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Battle Session")
	FMelodiaEncounterDefinition ActiveEncounter;

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session", meta = (WorldContext = "WorldContextObject"))
	static UMelodiaBattleSession* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session")
	bool IsEncounterActive() const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool BeginEncounter(const FMelodiaEncounterDefinition& Encounter, const bool bSuppressPhoenixBattleUI = false);

	/** Phoenix skill menu (Option B) — call when the player confirms a songwriting skill. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool SubmitSkillCommand(FName SkillId);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool SubmitBasicCommand();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool SubmitUltimateCommand();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool SubmitFleeCommand();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	bool ConfirmVictoryReward();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	void NotifyRhythmExecutionStarted();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	void NotifyRhythmExecutionFinished();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Session")
	void EndEncounter(EMelodiaEncounterResult Result);

	/** Skill IDs unlocked for the current mechanic level (for Phoenix skill menus). */
	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session")
	TArray<FName> GetUnlockedSkillIds() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session")
	TArray<FMelodiaSongSkillRecipe> GetUnlockedSkills() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session")
	bool IsSkillUnlocked(FName SkillId) const;

	/** Returns false if skill is locked or encounter is not accepting commands. */
	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Session")
	bool CanSubmitSkillCommand(FName SkillId) const;

private:
	void SetBattlePhase(EMelodiaBattlePhase NewPhase);
	void SyncHUDMode();
	void SyncGameLoopPhase();
	AActor* ResolveBattleController() const;
	class UMelodiaRhythmExecutionComponent* ResolveExecutionComponent() const;
	bool SubmitLoopCommand(EMelodiaRhythmBattleCommand Command, float Grade = 1.0f);
	int32 GetPlayerMechanicLevel() const;
};
