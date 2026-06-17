// Reflection bridge between the native rhythm model and the TurnBasedJRPG template units.
//
// The native rhythm slice (UMelodiaBattleLoopLibrary + UMelodiaCombatStateComponent) owns the
// authoritative SP/ultimate/toughness/HP math. The JRPG template owns the live unit actors
// (BP_UnitBase: currentHP, GetUnitStats.maxHP, IsUnitDead, SetHP) inside the controller's
// enemyUnits / playerUnits arrays. This library mirrors native results onto those real units and
// seeds the native model from them, using pure reflection so no Blueprint authoring is required.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaJRPGBridgeLibrary.generated.h"

USTRUCT(BlueprintType)
struct FMelodiaJRPGVitals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	bool bHasEnemies = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	bool bHasParty = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	float EnemyHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	float EnemyMaxHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	float PartyHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|JRPG Bridge")
	float PartyMaxHP = 0.0f;
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaJRPGBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// True if the controller exposes JRPG template unit arrays (enemyUnits / playerUnits) with at least one unit.
	UFUNCTION(BlueprintPure, Category="Melodia|JRPG Bridge")
	static bool HasJRPGUnits(AActor* BattleController);

	// Reads the live JRPG unit HP/MaxHP into a vitals snapshot (active enemy + aggregated party).
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static FMelodiaJRPGVitals ReadJRPGVitals(AActor* BattleController);

	// Sets the active (first alive) enemy unit's HP to the supplied absolute value and refreshes its HP bar.
	// Returns the aggregated enemy HP after the change, or -1 if no JRPG enemies exist.
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static float SetActiveEnemyHP(AActor* BattleController, float NewHP);

	// Applies absolute damage to the active enemy unit (clamped at 0). Returns remaining aggregated enemy HP, or -1.
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static float DamageActiveEnemy(AActor* BattleController, float Damage);

	// Applies damage to the first alive party unit (clamped at 0). Returns remaining aggregated party HP, or -1.
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static float DamageParty(AActor* BattleController, float Damage);

	// True if every JRPG enemy unit is dead (currentHP <= 0). False if no enemies exist.
	UFUNCTION(BlueprintPure, Category="Melodia|JRPG Bridge")
	static bool AreEnemiesDefeated(AActor* BattleController);

	// True if every JRPG party unit is dead (currentHP <= 0). False if no party units exist.
	UFUNCTION(BlueprintPure, Category="Melodia|JRPG Bridge")
	static bool IsPartyDefeated(AActor* BattleController);

	// Heals all live JRPG party units to max HP (defeat recovery / next-encounter seed).
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static void RestorePartyVitals(AActor* BattleController);

	/** Invokes the Phoenix template flee/run flow on the battle controller when available. */
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static bool TryFleeBattle(AActor* BattleController);

	/** Removes Phoenix BattleUI / skill dialogs and battle cameras so Melodia native HUD owns presentation. */
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge")
	static void TeardownPhoenixBattleUI(AActor* BattleController);

	/** Align Phoenix playerUnits[] visibility and vitals with UMelodiaPartySubsystem roster slots. */
	UFUNCTION(BlueprintCallable, Category="Melodia|JRPG Bridge", meta=(WorldContext="WorldContextObject"))
	static bool SyncPartyUnitsFromSubsystem(UObject* WorldContextObject, AActor* BattleController);

private:
	static bool TrySetUnitDisplayName(UObject* Unit, const FText& DisplayName);
	static bool GetUnitArray(AActor* BattleController, FName ArrayPropertyName, TArray<UObject*>& OutUnits);
	static UObject* GetCurrentBattleObject(AActor* BattleController);
	static bool GetUnitArrayFromObject(UObject* SourceObject, FName ArrayPropertyName, TArray<UObject*>& OutUnits);
	static UObject* GetActiveEnemyUnit(AActor* BattleController);
	static UObject* GetActivePartyUnit(AActor* BattleController);
	static bool SetObjectProperty(UObject* Object, FName PropertyName, UObject* Value);
	static bool GetNumericProperty(UObject* Object, FName PropertyName, double& OutValue);
	static bool SetNumericProperty(UObject* Object, FName PropertyName, double Value);
	static double GetUnitCurrentHP(UObject* Unit);
	static double GetUnitMaxHP(UObject* Unit);
	static void SetUnitHP(UObject* Unit, double NewHP);
	static bool CallUnitNumericFunction(UObject* Unit, FName FunctionName, double Value);
	static bool CallControllerDealDamage(AActor* BattleController, UObject* Attacker, UObject* Target, float Damage);
};
