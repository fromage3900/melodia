// Mechanic level progression: unlock tiers and location presets for the demo (Lv 1–30).

#pragma once

#include "CoreMinimal.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaSpellTypes.h"
#include "MelodiaMechanicProgressionTypes.generated.h"

/** Six unlock bands across demo levels 1–30 (five levels per tier). */
UENUM(BlueprintType)
enum class EMelodiaMechanicTier : uint8
{
	TierI_NoviceStar     UMETA(DisplayName = "Novice Star"),
	TierII_MoonApprentice UMETA(DisplayName = "Moon Apprentice"),
	TierIII_CometAdept   UMETA(DisplayName = "Comet Adept"),
	TierIV_AuroraVirtuoso UMETA(DisplayName = "Aurora Virtuoso"),
	TierV_NebulaMaestro  UMETA(DisplayName = "Nebula Maestro"),
	TierVI_CelestialLegend UMETA(DisplayName = "Celestial Legend")
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaMechanicTierDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	EMelodiaMechanicTier Tier = EMelodiaMechanicTier::TierI_NoviceStar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	int32 MinMechanicLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	int32 MaxMechanicLevel = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	FString TierDisplayName = TEXT("Novice Star");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	FString UnlockBlurb = TEXT("New location presets available in Reverie runs.");
};

/** Authoring preset unlocked at a specific mechanic level — assign PCG graph in editor or use default soft path. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaLocationLevelPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	FName PresetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	int32 MechanicLevelRequired = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	EMelodiaMechanicTier MechanicTier = EMelodiaMechanicTier::TierI_NoviceStar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	FString DisplayName = TEXT("Starlit Atrium");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	FString BuildNotes = TEXT("Starter PCG blockout — place PCG Volume and assign graph.");

	/** Soft path to PCG graph asset (e.g. /Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	FSoftObjectPath PCGGraphAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset", meta = (ClampMin = "0"))
	int32 MinEncounters = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset", meta = (ClampMin = "1"))
	int32 MaxEncounters = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset", meta = (ClampMin = "0.1"))
	float DifficultyMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	EPCGArchitecturalRole QuestMarkerRole = EPCGArchitecturalRole::Door;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Location Preset")
	bool bUnlockedAtStart = false;
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaMechanicProgressionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	int32 MechanicLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	int32 MechanicXP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	TArray<FName> UnlockedPresetIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	int32 TotalLevelUps = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	TArray<FName> UnlockedSkillIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	TArray<FName> UnlockedKeyIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	FName ActiveSkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	EMelodiaSpellElement EquippedKeyElement = EMelodiaSpellElement::Forte;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Mechanic")
	bool bCompanionUnlocked = false;
};
