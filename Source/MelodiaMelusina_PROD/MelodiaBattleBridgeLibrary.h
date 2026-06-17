// Blueprint-friendly Phoenix Option B entry points (wraps UMelodiaBattleSession).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaBattleTypes.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaBattleBridgeLibrary.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaBattleBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static class UMelodiaBattleSession* GetBattleSession(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool IsMelodiaBattleActive(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixSubmitAttack(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixSubmitSkill(const UObject* WorldContextObject, FName SkillId);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixSubmitUltimate(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixSubmitFlee(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixConfirmVictory(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static TArray<FName> PhoenixGetUnlockedSkillIds(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static TArray<FMelodiaSongSkillRecipe> PhoenixGetUnlockedSkills(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixCanUseSkill(const UObject* WorldContextObject, FName SkillId);

	/** Maps Phoenix template skill slot index (0-based) to Melodia SkillId for the current mechanic level. */
	UFUNCTION(BlueprintPure, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static FName PhoenixResolveSkillIdFromMenuIndex(const UObject* WorldContextObject, int32 MenuIndex);

	/** Call from BP_SkillUseDialogue when the player confirms a skill row (index from Phoenix UI). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|Battle Bridge", meta = (WorldContext = "WorldContextObject"))
	static bool PhoenixSubmitSkillByMenuIndex(const UObject* WorldContextObject, int32 MenuIndex);
};
