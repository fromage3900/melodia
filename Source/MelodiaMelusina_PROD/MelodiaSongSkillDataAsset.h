// Authorable songwriting skill — create assets under /Game/Melodia/Data/Skills/.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaSongSkillDataAsset.generated.h"

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UMelodiaSongSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Song Skill")
	FMelodiaSongSkillRecipe Recipe;

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skill")
	FMelodiaSongSkillRecipe GetRecipe() const { return Recipe; }
};
