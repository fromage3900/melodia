#include "MelodiaMenuBridgeLibrary.h"

#include "Kismet/GameplayStatics.h"

namespace MelodiaMenuBridgePrivate
{
	static const FString GameModeOptions =
		TEXT("game=/Game/Melodia/Core/BP_MelodiaRhythmGameMode.BP_MelodiaRhythmGameMode_C");
}

void UMelodiaMenuBridgeLibrary::LaunchGameplayLoopTest(UObject* WorldContextObject)
{
	UGameplayStatics::OpenLevel(
		WorldContextObject,
		GetGameplayLoopTestMapName(),
		true,
		MelodiaMenuBridgePrivate::GameModeOptions);
}

void UMelodiaMenuBridgeLibrary::LaunchPCGDemo(UObject* WorldContextObject)
{
	static const FString Options = MelodiaMenuBridgePrivate::GameModeOptions + TEXT("?PCGDemo");
	UGameplayStatics::OpenLevel(WorldContextObject, GetPCGDemoMapName(), true, Options);
}

void UMelodiaMenuBridgeLibrary::LaunchPortfolioBezierDemo(UObject* WorldContextObject)
{
	static const FString Options = MelodiaMenuBridgePrivate::GameModeOptions + TEXT("?PortfolioBezier");
	UGameplayStatics::OpenLevel(WorldContextObject, GetPortfolioBezierMapName(), true, Options);
}

void UMelodiaMenuBridgeLibrary::LaunchMainMenu(UObject* WorldContextObject)
{
	UGameplayStatics::OpenLevel(WorldContextObject, GetOGMainMenuMapName());
}

FName UMelodiaMenuBridgeLibrary::GetGameplayLoopTestMapName()
{
	return FName(TEXT("L_MelodiaGameplayLoopTest"));
}

FName UMelodiaMenuBridgeLibrary::GetPCGDemoMapName()
{
	return FName(TEXT("L_MelodiaPCGDemo"));
}

FName UMelodiaMenuBridgeLibrary::GetPortfolioBezierMapName()
{
	return FName(TEXT("L_MelodiaPortfolioTerrace"));
}

FName UMelodiaMenuBridgeLibrary::GetOGMainMenuMapName()
{
	return FName(TEXT("MainMenu"));
}
