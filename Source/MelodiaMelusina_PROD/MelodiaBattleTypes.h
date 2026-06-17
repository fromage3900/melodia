// Shared battle session types — kernel phases, encounter payload, HUD modes.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBattleTypes.generated.h"

UENUM(BlueprintType)
enum class EMelodiaBattlePhase : uint8
{
	None UMETA(DisplayName = "None"),
	AwaitingPlayerCommand UMETA(DisplayName = "Awaiting Player Command"),
	RhythmExecution UMETA(DisplayName = "Rhythm Execution"),
	EnemyTurn UMETA(DisplayName = "Enemy Turn"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat"),
	Fled UMETA(DisplayName = "Fled")
};

UENUM(BlueprintType)
enum class EMelodiaHUDMode : uint8
{
	Exploration UMETA(DisplayName = "Exploration"),
	BattleCompact UMETA(DisplayName = "Battle Compact"),
	BattleHighway UMETA(DisplayName = "Battle Highway"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

UENUM(BlueprintType)
enum class EMelodiaEncounterResult : uint8
{
	None UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat"),
	Fled UMETA(DisplayName = "Fled")
};

/** Payload passed from encounter triggers into the battle session. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaEncounterDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Melodia|Battle")
	TObjectPtr<AActor> BattleController = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Melodia|Battle")
	TObjectPtr<AActor> BattleData = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Melodia|Battle")
	int32 EncounterLevel = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Melodia|Battle")
	FText EncounterDisplayName;
};
