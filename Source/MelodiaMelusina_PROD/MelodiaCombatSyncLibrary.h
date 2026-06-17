// Mirrors UMelodiaCombatStateComponent onto legacy Rhythm* controller properties for Blueprint debug hooks.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaCombatSyncLibrary.generated.h"

class UMelodiaCombatStateComponent;

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaCombatLegacyOverrides
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideLastDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float LastDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideEnemyHP = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float EnemyHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideEnemyMaxHP = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float EnemyMaxHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideLastToughnessDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float LastToughnessDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideLastFollowUpBonusDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float LastFollowUpBonusDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideLastPartyDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float LastPartyDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	bool bOverrideLastUltimateDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Combat Sync")
	float LastUltimateDamage = 0.0f;
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaCombatSyncLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** When combat state exists, copy its fields onto Rhythm* properties instead of duplicating math in the loop. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|Combat Sync")
	static void MirrorCombatStateToLegacyProperties(
		AActor* BattleController,
		const UMelodiaCombatStateComponent* CombatState,
		FMelodiaCombatLegacyOverrides Overrides);
};
