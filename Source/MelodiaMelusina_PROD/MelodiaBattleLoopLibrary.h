// Native glue for the rhythm-battle vertical slice.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaSpellTypes.h"
#include "MelodiaBattleLoopLibrary.generated.h"

struct FMelodiaHighwayNote;

UENUM(BlueprintType)
enum class EMelodiaRhythmBattleCommand : uint8
{
	Basic UMETA(DisplayName="Basic"),
	Skill UMETA(DisplayName="Skill"),
	Ultimate UMETA(DisplayName="Ultimate")
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaBattleLoopLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ApplyRhythmBattleHit(UObject* WorldContextObject, AActor* BattleController, float Grade, int32 ComboToWin = 3);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ApplyRhythmSkillAction(UObject* WorldContextObject, AActor* BattleController, float Grade, int32 ComboToWin = 3);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ExecuteRhythmBattleCommand(UObject* WorldContextObject, AActor* BattleController, EMelodiaRhythmBattleCommand Command, float Grade = 1.0f, int32 ComboToWin = 3);

	/** Traditional JRPG skill execution (instant): applies a skill recipe as a single action (no rhythm highway). */
	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ExecuteInstantSkillRecipe(UObject* WorldContextObject, AActor* BattleController, const FMelodiaSongSkillRecipe& Recipe, float Grade = 1.0f);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool HasRhythmVictoryResolved(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool HasRhythmRewardBeenConfirmed(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool IsRhythmExplorationReady(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool IsRhythmUltimateReady(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static float GetRhythmUltimateGauge(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static int32 GetRhythmSkillPoints(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static int32 GetRhythmSkillPointMax(AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool CanUseRhythmSkill(AActor* BattleController);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool TriggerRhythmUltimate(UObject* WorldContextObject, AActor* BattleController);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ApplyRhythmExecutionResult(UObject* WorldContextObject, AActor* BattleController, const TArray<FMelodiaHighwayNote>& NoteResults, bool bSkillAction, float SkillScalar, int32 SkillCost);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool TryFleeRhythmBattle(UObject* WorldContextObject, AActor* BattleController);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop", meta=(DefaultToSelf="WorldContextObject"))
	static bool ExecuteEnemyTurn(UObject* WorldContextObject, AActor* BattleController);

	UFUNCTION(BlueprintPure, Category="Melodia|Battle Loop")
	static bool IsPartyDefeated(AActor* BattleController);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop")
	static void ResetRhythmBattleEncounter(AActor* BattleController, int32 OverrideEncounterLevel = 0);

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Loop")
	static void ConfirmRhythmVictoryReward(AActor* BattleController);

private:
	static const FName RhythmVictoryResolvedTag;
	static const FName RhythmRewardConfirmedTag;
	static const FName RhythmExplorationReadyTag;
	static const FName RhythmUltimateReadyTag;
	static float GetFloatPropertyValue(AActor* Actor, FName PropertyName, float FallbackValue);
	static void SetFloatPropertyValue(AActor* Actor, FName PropertyName, float Value);
	static int32 GetIntPropertyValue(AActor* Actor, FName PropertyName, int32 FallbackValue);
	static void SetIntPropertyValue(AActor* Actor, FName PropertyName, int32 Value);
	static void SetBoolPropertyValue(AActor* Actor, FName PropertyName, bool bValue);
	static bool ApplyRhythmBattleAction(UObject* WorldContextObject, AActor* BattleController, float Grade, int32 ComboToWin, bool bSkillAction, EMelodiaSpellElement AttackElement = EMelodiaSpellElement::Forte, int32 SkillCost = 1, float SkillScalar = 1.0f);
	static void PublishReactiveCommandState(AActor* BattleController, const FString& CommandName, const FString& IntentName, float IntentPower, bool bUltimateWindow, bool bUltimateInterrupt);
	static void ResolveRhythmVictory(UObject* WorldContextObject, AActor* BattleController, float RemainingHP);
};
