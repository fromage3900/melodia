#if WITH_DEV_AUTOMATION_TESTS

#include "MelodiaContentRegistrySubsystem.h"
#include "MelodiaSongSkillLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMelodiaContentRegistryTest,
	"Melodia.ContentRegistry.SongSkills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaContentRegistryTest::RunTest(const FString& Parameters)
{
	const TArray<FMelodiaSongSkillRecipe> Demo = UMelodiaSongSkillLibrary::BuildDemoSongSkills();
	TestEqual(TEXT("Demo catalog has 30 skills"), Demo.Num(), 30);
	TestTrue(TEXT("Demo Lv1 skill exists"), Demo[0].SkillId == FName(TEXT("Skill_Lv01_StarlitPing")));
	return true;
}

#endif
