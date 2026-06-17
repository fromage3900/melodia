// PIE/dev console helpers for demo iteration (not available in shipping).

#include "MelodiaDevCheats.h"

#if !UE_BUILD_SHIPPING

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaBattleSession.h"
#include "MelodiaCombatStateComponent.h"
#include "MelodiaJRPGBridgeLibrary.h"
#include "MelodiaMenuBridgeLibrary.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaRhythmGameModeBase.h"

namespace MelodiaDevCheatsPrivate
{
IConsoleCommand* SetMechanicLevelCmd = nullptr;
IConsoleCommand* GrantXPCmd = nullptr;
IConsoleCommand* ResetProgressionCmd = nullptr;
IConsoleCommand* DumpBattleCmd = nullptr;
IConsoleCommand* WinBattleCmd = nullptr;

UMelodiaMechanicProgressionSubsystem* GetProgression(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>() : nullptr;
}

void LogUsage(const TCHAR* Command, const TCHAR* Example)
{
	UE_LOG(LogTemp, Warning, TEXT("%s — example: %s"), Command, Example);
}
}

void MelodiaDevCheats::SetMechanicLevel(const TArray<FString>& Args, UWorld* World)
{
	using namespace MelodiaDevCheatsPrivate;

	if (!World)
	{
		return;
	}

	if (Args.Num() < 1)
	{
		LogUsage(TEXT("Melodia.SetMechanicLevel"), TEXT("Melodia.SetMechanicLevel 15"));
		return;
	}

	UMelodiaMechanicProgressionSubsystem* Progression = GetProgression(World);
	if (!Progression)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia.SetMechanicLevel: no progression subsystem."));
		return;
	}

	const int32 Level = FMath::Clamp(FCString::Atoi(*Args[0]), 1, 30);
	Progression->SetMechanicLevelForDemo(Level);
	UE_LOG(LogTemp, Display, TEXT("Melodia mechanic level -> %d | active skill: %s"),
		Level, *Progression->State.ActiveSkillId.ToString());
}

void MelodiaDevCheats::GrantMechanicXP(const TArray<FString>& Args, UWorld* World)
{
	using namespace MelodiaDevCheatsPrivate;

	if (!World)
	{
		return;
	}

	if (Args.Num() < 1)
	{
		LogUsage(TEXT("Melodia.GrantXP"), TEXT("Melodia.GrantXP 120"));
		return;
	}

	UMelodiaMechanicProgressionSubsystem* Progression = GetProgression(World);
	if (!Progression)
	{
		return;
	}

	const int32 Amount = FMath::Max(1, FCString::Atoi(*Args[0]));
	Progression->GrantMechanicXP(Amount, TEXT("Dev console"));
}

void MelodiaDevCheats::ResetProgression(UWorld* World)
{
	using namespace MelodiaDevCheatsPrivate;

	if (UMelodiaMechanicProgressionSubsystem* Progression = GetProgression(World))
	{
		Progression->ResetToDemoDefaults();
		Progression->SyncHUD(World);
		UE_LOG(LogTemp, Display, TEXT("Melodia progression reset to Lv1 demo defaults."));
	}
}

void MelodiaDevCheats::DumpBattleState(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World));
	const UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(World);
	AActor* Controller = Session ? Session->GetActiveBattleController() : nullptr;

	UE_LOG(LogTemp, Display, TEXT("--- Melodia battle dump ---"));
	if (GameMode)
	{
		UE_LOG(LogTemp, Display, TEXT("Loop phase: %s | HSR=%s | Highway=%s | PhoenixUI=%s | PhoenixUnits=%s"),
			*GameMode->GetLoopPhaseText(),
			GameMode->bHSRStyleBattle ? TEXT("on") : TEXT("off"),
			GameMode->bUseRhythmHighway ? TEXT("on") : TEXT("off"),
			GameMode->bSuppressPhoenixBattleUI ? TEXT("hidden") : TEXT("visible"),
			GameMode->bKeepPhoenixBattlePresentation ? TEXT("kept") : TEXT("off"));
	}

	if (Session)
	{
		UE_LOG(LogTemp, Display, TEXT("Session phase=%d active=%s commands=%d"),
			static_cast<int32>(Session->GetBattlePhase()),
			Session->IsEncounterActive() ? TEXT("yes") : TEXT("no"),
			Session->CommandSubmitCount);
	}

	if (Controller)
	{
		const FMelodiaJRPGVitals Vitals = UMelodiaJRPGBridgeLibrary::ReadJRPGVitals(Controller);
		const UMelodiaCombatStateComponent* CombatState = Controller->FindComponentByClass<UMelodiaCombatStateComponent>();
		UE_LOG(LogTemp, Display, TEXT("Enemy HP %.0f/%.0f | Party %.0f/%.0f | SP %d | Ult %.0f | Victory=%s"),
			Vitals.EnemyHP,
			Vitals.EnemyMaxHP,
			CombatState ? CombatState->PartyHP : Vitals.PartyHP,
			CombatState ? CombatState->PartyMaxHP : Vitals.PartyMaxHP,
			UMelodiaBattleLoopLibrary::GetRhythmSkillPoints(Controller),
			UMelodiaBattleLoopLibrary::GetRhythmUltimateGauge(Controller),
			UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(Controller) ? TEXT("yes") : TEXT("no"));
	}

	if (UMelodiaMechanicProgressionSubsystem* Progression = MelodiaDevCheatsPrivate::GetProgression(World))
	{
		UE_LOG(LogTemp, Display, TEXT("Mechanic Lv%d XP %d | skills unlocked %d"),
			Progression->GetMechanicLevel(),
			Progression->State.MechanicXP,
			Progression->State.UnlockedSkillIds.Num());
	}
}

void MelodiaDevCheats::WinBattle(UWorld* World)
{
	if (!World)
	{
		return;
	}

	UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(World);
	if (!Session || !Session->IsEncounterActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia.WinBattle: no active encounter."));
		return;
	}

	AActor* Controller = Session->GetActiveBattleController();
	if (!Controller)
	{
		return;
	}

	for (int32 Attempt = 0; Attempt < 48 && Session->IsEncounterActive(); ++Attempt)
	{
		if (UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(Controller))
		{
			Session->ConfirmVictoryReward();
			UE_LOG(LogTemp, Display, TEXT("Melodia.WinBattle: encounter won after %d hits."), Attempt);
			return;
		}

		if (!Session->SubmitBasicCommand())
		{
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Melodia.WinBattle: could not resolve victory (try Melodia.DumpBattle)."));
}

void MelodiaDevCheats::PlayDemo(UWorld* World)
{
	UMelodiaMenuBridgeLibrary::LaunchGameplayLoopTest(World);
	UE_LOG(LogTemp, Log, TEXT("Melodia.PlayDemo: opening L_MelodiaGameplayLoopTest."));
}

void MelodiaDevCheats::PlayPCGDemo(UWorld* World)
{
	UMelodiaMenuBridgeLibrary::LaunchPCGDemo(World);
	UE_LOG(LogTemp, Log, TEXT("Melodia.PlayPCGDemo: opening L_MelodiaPCGDemo (Terrace Garden PCG)."));
}

void MelodiaDevCheats::RegisterConsoleCommands()
{
	using namespace MelodiaDevCheatsPrivate;

	if (SetMechanicLevelCmd)
	{
		return;
	}

	SetMechanicLevelCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.SetMechanicLevel"),
		TEXT("Set demo mechanic level 1-30. Usage: Melodia.SetMechanicLevel 15"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MelodiaDevCheats::SetMechanicLevel),
		ECVF_Cheat);

	GrantXPCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.GrantXP"),
		TEXT("Grant mechanic XP. Usage: Melodia.GrantXP 120"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MelodiaDevCheats::GrantMechanicXP),
		ECVF_Cheat);

	ResetProgressionCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.ResetProgression"),
		TEXT("Reset mechanic progression to Lv1 demo defaults."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { MelodiaDevCheats::ResetProgression(World); }),
		ECVF_Cheat);

	DumpBattleCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.DumpBattle"),
		TEXT("Log loop phase, session state, and combat vitals."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { MelodiaDevCheats::DumpBattleState(World); }),
		ECVF_Cheat);

	WinBattleCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.WinBattle"),
		TEXT("Instantly resolve the current encounter as a win (PIE testing)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { MelodiaDevCheats::WinBattle(World); }),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.PlayDemo"),
		TEXT("Open the Melodia gameplay loop test level (portfolio demo)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { MelodiaDevCheats::PlayDemo(World); }),
		ECVF_Cheat);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Melodia.PlayPCGDemo"),
		TEXT("Open the Melodia PCG Terrace Garden demo (procedural environment + gameplay loop)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { MelodiaDevCheats::PlayPCGDemo(World); }),
		ECVF_Cheat);

	UE_LOG(LogTemp, Log, TEXT("Melodia dev cheats registered (~ key console in PIE)."));
}

void MelodiaDevCheats::UnregisterConsoleCommands()
{
	using namespace MelodiaDevCheatsPrivate;

	auto Unregister = [](IConsoleCommand*& Cmd)
	{
		if (Cmd)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Cmd);
			Cmd = nullptr;
		}
	};

	Unregister(SetMechanicLevelCmd);
	Unregister(GrantXPCmd);
	Unregister(ResetProgressionCmd);
	Unregister(DumpBattleCmd);
	Unregister(WinBattleCmd);
}

#else

void MelodiaDevCheats::RegisterConsoleCommands() {}
void MelodiaDevCheats::UnregisterConsoleCommands() {}
void MelodiaDevCheats::SetMechanicLevel(const TArray<FString>&, UWorld*) {}
void MelodiaDevCheats::GrantMechanicXP(const TArray<FString>&, UWorld*) {}
void MelodiaDevCheats::ResetProgression(UWorld*) {}
void MelodiaDevCheats::DumpBattleState(UWorld*) {}
void MelodiaDevCheats::WinBattle(UWorld*) {}

#endif
