// Deterministic mechanic level tables, XP curve, and demo location presets (Lv 1–30).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaMechanicProgressionTypes.h"
#include "MelodiaReverieRunManager.h"
#include "MelodiaMechanicProgressionLibrary.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaMechanicProgressionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 DemoMaxMechanicLevel = 30;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static int32 GetDemoMaxMechanicLevel() { return DemoMaxMechanicLevel; }

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static int32 GetXPRequiredForNextLevel(int32 CurrentLevel);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static EMelodiaMechanicTier GetTierForMechanicLevel(int32 MechanicLevel);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static FString GetTierDisplayName(EMelodiaMechanicTier Tier);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static TArray<FMelodiaMechanicTierDefinition> BuildDefaultTierDefinitions();

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static TArray<FMelodiaLocationLevelPreset> BuildDemoLocationPresets();

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static bool FindLocationPreset(FName PresetId, FMelodiaLocationLevelPreset& OutPreset);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static FReverieAreaConfig ToReverieAreaConfig(const FMelodiaLocationLevelPreset& Preset);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic Progression")
	static bool GrantMechanicXP(UPARAM(ref) FMelodiaMechanicProgressionState& State, int32 Amount, FString& OutLevelUpSummary);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static TArray<FMelodiaLocationLevelPreset> GetUnlockedPresetsForState(const FMelodiaMechanicProgressionState& State);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic Progression")
	static void EnsurePresetUnlocked(FMelodiaMechanicProgressionState& State, FName PresetId);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic Progression")
	static FName GetPresetIdForMechanicLevel(int32 MechanicLevel);
};
