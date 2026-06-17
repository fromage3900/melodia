// Authorable encounter definition for battle session bootstrap.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MelodiaBattleTypes.h"
#include "MelodiaSpellTypes.h"
#include "MelodiaEncounterDataAsset.generated.h"

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UMelodiaEncounterDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Encounter")
	FName EncounterId = TEXT("DefaultSlime");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Encounter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Encounter")
	FSoftClassPath BattleDataClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Encounter", meta = (ClampMin = "0"))
	int32 EncounterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Encounter")
	EMelodiaSpellElement EnemyElement = EMelodiaSpellElement::Forte;

	UFUNCTION(BlueprintPure, Category = "Melodia|Encounter")
	FMelodiaEncounterDefinition MakeEncounterDefinition(AActor* BattleController, AActor* BattleData) const;
};
