// PIE/dev console helpers (stripped in shipping builds).

#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace MelodiaDevCheats
{
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	void SetMechanicLevel(const TArray<FString>& Args, UWorld* World);
	void GrantMechanicXP(const TArray<FString>& Args, UWorld* World);
	void ResetProgression(UWorld* World);
	void DumpBattleState(UWorld* World);
	void WinBattle(UWorld* World);
}
