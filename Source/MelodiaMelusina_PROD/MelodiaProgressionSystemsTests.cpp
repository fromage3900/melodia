#if WITH_DEV_AUTOMATION_TESTS

#include "MelodiaKeySystemLibrary.h"
#include "MelodiaSongSkillLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMelodiaSongSkillCatalogTest,
	"Melodia.Progression.SongSkills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaSongSkillCatalogTest::RunTest(const FString& Parameters)
{
	const TArray<FMelodiaSongSkillRecipe> Skills = UMelodiaSongSkillLibrary::BuildDemoSongSkills();
	TestEqual(TEXT("Thirty songwriting skills"), Skills.Num(), 30);
	TestEqual(TEXT("Lv1 skill id"), Skills[0].SkillId, FName(TEXT("Skill_Lv01_StarlitPing")));
	TestEqual(TEXT("Lv30 skill id"), Skills[29].SkillId, FName(TEXT("Skill_Lv30_MaestroFinale")));
	TestEqual(TEXT("Lv1 requires level 1"), Skills[0].MechanicLevelRequired, 1);
	TestEqual(TEXT("Lv30 requires level 30"), Skills[29].MechanicLevelRequired, 30);

	FMelodiaSongSkillRecipe Found;
	TestTrue(TEXT("Find Lv8 companion skill"), UMelodiaSongSkillLibrary::FindSongSkill(FName(TEXT("Skill_Lv08_TessellationWave")), Found));
	TestEqual(TEXT("Lv8 skill has four notes"), Found.NotePitches.Num(), 4);

	TArray<FName> UnlockedAt8 = UMelodiaSongSkillLibrary::GetSkillIdsUnlockedAtOrBelowLevel(8);
	TestEqual(TEXT("Eight skills unlocked at Lv8"), UnlockedAt8.Num(), 8);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMelodiaElementKeySystemTest,
	"Melodia.Progression.ElementKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaElementKeySystemTest::RunTest(const FString& Parameters)
{
	const TArray<FMelodiaElementKeyDefinition> Keys = UMelodiaKeySystemLibrary::BuildDemoElementKeys();
	TestEqual(TEXT("Seven harmonic keys"), Keys.Num(), 7);
	TestEqual(TEXT("First key at Lv3"), Keys[0].MechanicLevelRequired, 3);

	TestTrue(
		TEXT("Forte hits Gale weakness"),
		UMelodiaKeySystemLibrary::IsWeaknessHit(EMelodiaSpellElement::Forte, EMelodiaSpellElement::Gale));
	TestTrue(
		TEXT("Forte resists Arcane"),
		UMelodiaKeySystemLibrary::IsResistanceHit(EMelodiaSpellElement::Forte, EMelodiaSpellElement::Arcane));

	const float BaseWeakness = UMelodiaKeySystemLibrary::GetElementDamageMultiplier(
		EMelodiaSpellElement::Forte, EMelodiaSpellElement::Gale, false);
	TestEqual(TEXT("Weakness multiplier"), BaseWeakness, UMelodiaKeySystemLibrary::WeaknessMultiplier);

	const float KeyBoosted = UMelodiaKeySystemLibrary::GetElementDamageMultiplier(
		EMelodiaSpellElement::Forte, EMelodiaSpellElement::Gale, true);
	TestEqual(
		TEXT("Matching key on weakness"),
		KeyBoosted,
		UMelodiaKeySystemLibrary::WeaknessMultiplier * UMelodiaKeySystemLibrary::MatchingKeyWeaknessBonus);

	TestEqual(
		TEXT("Encounter Lv1 enemy element"),
		UMelodiaKeySystemLibrary::GetEnemyElementForEncounterLevel(1),
		EMelodiaSpellElement::Forte);

	return true;
}

#endif
