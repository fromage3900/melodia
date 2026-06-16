// Automation tests for Melodia's deterministic combat and songcraft rules.

#if WITH_DEV_AUTOMATION_TESTS

#include "MelodiaCoreRulesLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMelodiaRhythmRulesTest, "Melodia.CoreRules.Rhythm", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaRhythmRulesTest::RunTest(const FString& Parameters)
{
	const FMelodiaRhythmWindows Windows;

	const FMelodiaRhythmGradeResult Perfect = UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(20.0f, Windows);
	TestEqual(TEXT("20 ms grades Perfect"), Perfect.Grade, EMelodiaRhythmGrade::Perfect);
	TestEqual(TEXT("Perfect multiplier is capped at 1.5"), Perfect.CombatMultiplier, 1.5f);
	TestTrue(TEXT("Perfect counts as hit"), Perfect.bCountsAsHit);

	const FMelodiaRhythmGradeResult Great = UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(75.0f, Windows);
	TestEqual(TEXT("75 ms grades Great"), Great.Grade, EMelodiaRhythmGrade::Great);

	const FMelodiaRhythmGradeResult Good = UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(140.0f, Windows);
	TestEqual(TEXT("140 ms grades Good"), Good.Grade, EMelodiaRhythmGrade::Good);

	const FMelodiaRhythmGradeResult Miss = UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(220.0f, Windows);
	TestEqual(TEXT("220 ms grades Miss"), Miss.Grade, EMelodiaRhythmGrade::Miss);
	TestEqual(TEXT("Miss multiplier is 0.4"), Miss.CombatMultiplier, 0.4f);
	TestFalse(TEXT("Miss does not count as hit"), Miss.bCountsAsHit);

	const float ComboCapped = UMelodiaCoreRulesLibrary::GetRhythmCombatMultiplier(EMelodiaRhythmGrade::Good, 20);
	TestEqual(TEXT("Combo bonus cannot exceed max combat multiplier"), ComboCapped, 1.5f);

	const int32 Crescendo = UMelodiaCoreRulesLibrary::ApplyCrescendoGain(98, EMelodiaRhythmGrade::Perfect, 100);
	TestEqual(TEXT("Crescendo gain clamps to max"), Crescendo, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMelodiaTurnEconomyRulesTest, "Melodia.CoreRules.TurnEconomy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaTurnEconomyRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("100 SPD costs 100 AV"), UMelodiaCoreRulesLibrary::CalculateAVCost(100), 100);
	TestEqual(TEXT("200 SPD costs 50 AV"), UMelodiaCoreRulesLibrary::CalculateAVCost(200), 50);
	TestEqual(TEXT("Speed clamps away from divide by zero"), UMelodiaCoreRulesLibrary::CalculateAVCost(0), 10000);

	TestEqual(TEXT("Shared SP clamps at max"), UMelodiaCoreRulesLibrary::ApplySharedSPDelta(4, 3, 5), 5);
	TestEqual(TEXT("Shared SP clamps at zero"), UMelodiaCoreRulesLibrary::ApplySharedSPDelta(1, -3, 5), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMelodiaSongcraftRulesTest, "Melodia.CoreRules.Songcraft", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaSongcraftRulesTest::RunTest(const FString& Parameters)
{
	TArray<int32> Notes;
	Notes.Add(60);
	Notes.Add(64);
	Notes.Add(67);
	Notes.Add(72);

	TArray<float> Durations;
	Durations.Add(1.0f);
	Durations.Add(1.0f);
	Durations.Add(1.0f);
	Durations.Add(1.0f);

	TArray<FMelodiaSongMaterialInput> Materials;
	FMelodiaSongMaterialInput Crystal;
	Crystal.MaterialId = TEXT("MoonCrystal");
	Crystal.RarityTier = 3;
	Crystal.PowerModifier = 1.2f;
	Materials.Add(Crystal);

	const FMelodiaGeneratedSpell First = UMelodiaCoreRulesLibrary::GenerateSpellFromSong(Notes, Durations, EMelodiaInstrument::MusicBox, Materials);
	const FMelodiaGeneratedSpell Second = UMelodiaCoreRulesLibrary::GenerateSpellFromSong(Notes, Durations, EMelodiaInstrument::MusicBox, Materials);
	TestEqual(TEXT("Same composition produces same hash"), First.CompositionHash, Second.CompositionHash);
	TestEqual(TEXT("Same composition produces same spell id"), First.SpellId, Second.SpellId);
	TestTrue(TEXT("Generated spell has usable power"), First.Power > 0.0f);
	TestTrue(TEXT("Generated spell has legal SP cost"), First.SPCost >= 1 && First.SPCost <= 5);

	const int32 TrumpetHash = UMelodiaCoreRulesLibrary::MakeCompositionHash(Notes, Durations, EMelodiaInstrument::Trumpet, Materials);
	TestNotEqual(TEXT("Instrument changes composition hash"), First.CompositionHash, TrumpetHash);

	return true;
}

#endif
