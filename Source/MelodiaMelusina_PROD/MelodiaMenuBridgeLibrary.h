// Blueprint helpers for launching Melodia maps from marketplace menu packs.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaMenuBridgeLibrary.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaMenuBridgeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Melodia|Menu", meta = (WorldContext = "WorldContextObject"))
	static void LaunchGameplayLoopTest(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Menu", meta = (WorldContext = "WorldContextObject"))
	static void LaunchPCGDemo(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Menu", meta = (WorldContext = "WorldContextObject"))
	static void LaunchPortfolioBezierDemo(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Menu", meta = (WorldContext = "WorldContextObject"))
	static void LaunchMainMenu(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Melodia|Menu")
	static FName GetGameplayLoopTestMapName();

	UFUNCTION(BlueprintPure, Category = "Melodia|Menu")
	static FName GetPCGDemoMapName();

	UFUNCTION(BlueprintPure, Category = "Melodia|Menu")
	static FName GetPortfolioBezierMapName();

	UFUNCTION(BlueprintPure, Category = "Melodia|Menu")
	static FName GetOGMainMenuMapName();
};
