#if WITH_DEV_AUTOMATION_TESTS

#include "MelodiaMechanicProgressionLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMelodiaMechanicProgressionTest,
	"Melodia.MechanicProgression.DemoLevel30",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaMechanicProgressionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Demo max level is 30"), UMelodiaMechanicProgressionLibrary::GetDemoMaxMechanicLevel(), 30);

	const TArray<FMelodiaMechanicTierDefinition> Tiers = UMelodiaMechanicProgressionLibrary::BuildDefaultTierDefinitions();
	TestEqual(TEXT("Six unlock tiers"), Tiers.Num(), 6);
	TestEqual(TEXT("Tier I starts at 1"), Tiers[0].MinMechanicLevel, 1);
	TestEqual(TEXT("Tier VI ends at 30"), Tiers[5].MaxMechanicLevel, 30);

	const TArray<FMelodiaLocationLevelPreset> Presets = UMelodiaMechanicProgressionLibrary::BuildDemoLocationPresets();
	TestEqual(TEXT("Thirty location presets"), Presets.Num(), 30);
	TestEqual(TEXT("Level 1 preset id"), Presets[0].PresetId, FName(TEXT("Lv01_StarlitAtrium")));
	TestEqual(TEXT("Level 30 preset id"), Presets[29].PresetId, FName(TEXT("Lv30_MaestroSanctum")));

	TestEqual(TEXT("Level 6 is Tier II"), UMelodiaMechanicProgressionLibrary::GetTierForMechanicLevel(6), EMelodiaMechanicTier::TierII_MoonApprentice);
	TestEqual(TEXT("Level 30 is Tier VI"), UMelodiaMechanicProgressionLibrary::GetTierForMechanicLevel(30), EMelodiaMechanicTier::TierVI_CelestialLegend);

	FMelodiaMechanicProgressionState State;
	State.MechanicLevel = 1;
	State.MechanicXP = 0;
	UMelodiaMechanicProgressionLibrary::EnsurePresetUnlocked(State, Presets[0].PresetId);

	FString Summary;
	const int32 XPBurst = 5000;
	const bool bLeveled = UMelodiaMechanicProgressionLibrary::GrantMechanicXP(State, XPBurst, Summary);
	TestTrue(TEXT("Large XP grant levels up"), bLeveled);
	TestTrue(TEXT("Reached at least level 5"), State.MechanicLevel >= 5);
	TestTrue(TEXT("Preset Lv05 unlocked"), State.UnlockedPresetIds.Contains(FName(TEXT("Lv05_GoldStuccoAlley"))));

	const TArray<FMelodiaLocationLevelPreset> Unlocked = UMelodiaMechanicProgressionLibrary::GetUnlockedPresetsForState(State);
	TestEqual(TEXT("Unlocked preset count matches level"), Unlocked.Num(), State.MechanicLevel);

	FReverieAreaConfig Area = UMelodiaMechanicProgressionLibrary::ToReverieAreaConfig(Presets[0]);
	TestEqual(TEXT("Reverie config display name"), Area.AreaDisplayName, Presets[0].DisplayName);

	return true;
}

#endif
