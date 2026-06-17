#if WITH_DEV_AUTOMATION_TESTS

#include "MelodiaBattleSession.h"
#include "MelodiaBattleTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMelodiaBattleSessionPhaseTest,
	"Melodia.BattleSession.Phases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMelodiaBattleSessionPhaseTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("None phase is default"), static_cast<uint8>(EMelodiaBattlePhase::None), static_cast<uint8>(EMelodiaBattlePhase::None));
	TestNotEqual(TEXT("Awaiting command differs from execution"), static_cast<uint8>(EMelodiaBattlePhase::AwaitingPlayerCommand), static_cast<uint8>(EMelodiaBattlePhase::RhythmExecution));
	TestEqual(TEXT("Exploration HUD mode default"), static_cast<uint8>(EMelodiaHUDMode::Exploration), static_cast<uint8>(EMelodiaHUDMode::Exploration));
	return true;
}

#endif
