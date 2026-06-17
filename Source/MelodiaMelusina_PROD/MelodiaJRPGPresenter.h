// Phoenix JRPG template adapter — unit init, presentation, optional UI suppression.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaBattleTypes.h"
#include "MelodiaJRPGPresenter.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaJRPGPresenter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Attach battle data and invoke Phoenix StartBattle / InitBattle (once). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter")
	static bool InitializeEncounter(UObject* WorldContextObject, const FMelodiaEncounterDefinition& Encounter, const bool bSuppressPhoenixBattleUI);

	/** Strategy B: strip Phoenix widgets, keep units + battle camera, apply Melodia battle view. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter", meta = (WorldContext = "WorldContextObject"))
	static bool PrepareBattlePresentation(UObject* WorldContextObject, AActor* BattleController, bool bStripPhoenixUI);

	/** Tear down Phoenix widgets/cameras when returning to exploration. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter")
	static void TeardownPresentation(AActor* BattleController);

	/** Wire Phoenix skill dialogue confirm to Melodia session (call after StartBattle). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter", meta = (WorldContext = "WorldContextObject"))
	static bool TryRoutePhoenixSkillMenuToSession(UObject* WorldContextObject, AActor* BattleController, int32 MenuSkillIndex);

	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter")
	static bool TryFleePresentation(AActor* BattleController);
};
