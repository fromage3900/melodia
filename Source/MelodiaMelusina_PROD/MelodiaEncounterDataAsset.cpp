#include "MelodiaEncounterDataAsset.h"

FMelodiaEncounterDefinition UMelodiaEncounterDataAsset::MakeEncounterDefinition(AActor* BattleController, AActor* BattleData) const
{
	FMelodiaEncounterDefinition Def;
	Def.BattleController = BattleController;
	Def.BattleData = BattleData;
	Def.EncounterLevel = EncounterLevel;
	Def.EncounterDisplayName = DisplayName;
	return Def;
}
