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

	/** Tear down Phoenix widgets/cameras when Melodia owns all battle UI. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter")
	static void TeardownPresentation(AActor* BattleController);

	UFUNCTION(BlueprintCallable, Category = "Melodia|JRPG Presenter")
	static bool TryFleePresentation(AActor* BattleController);
};
