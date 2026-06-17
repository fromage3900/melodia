// Demo songwriting skill catalog (Lv1–30) for rhythm highway execution.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaSpellTypes.h"
#include "MelodiaSongSkillLibrary.generated.h"

/** Authored songwriting skill — drives rhythm highway note patterns in battle. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaSongSkillRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	FName SkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	EMelodiaSpellElement Element = EMelodiaSpellElement::Forte;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	EMelodiaInstrument Instrument = EMelodiaInstrument::MusicBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill", meta = (ClampMin = "1"))
	int32 MechanicLevelRequired = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	TArray<int32> NotePitches;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	TArray<float> NoteDurations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	TArray<FMelodiaSongMaterialInput> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill", meta = (ClampMin = "0", ClampMax = "5"))
	int32 SPCostOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Song Skill")
	float PowerScalar = 1.0f;
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaSongSkillLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills")
	static TArray<FMelodiaSongSkillRecipe> BuildDemoSongSkills();

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills")
	static bool FindSongSkill(FName SkillId, FMelodiaSongSkillRecipe& OutSkill);

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills")
	static FName GetSkillIdForMechanicLevel(int32 MechanicLevel);

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills")
	static TArray<FName> GetSkillIdsUnlockedAtOrBelowLevel(int32 MechanicLevel);

	/** Prefer content registry when WorldContext has a game instance; falls back to demo catalog. */
	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills", meta = (WorldContext = "WorldContextObject"))
	static bool ResolveSongSkill(const UObject* WorldContextObject, FName SkillId, FMelodiaSongSkillRecipe& OutSkill);

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills", meta = (WorldContext = "WorldContextObject"))
	static FName ResolveSkillIdForMechanicLevel(const UObject* WorldContextObject, int32 MechanicLevel);

	UFUNCTION(BlueprintPure, Category = "Melodia|Song Skills", meta = (WorldContext = "WorldContextObject"))
	static TArray<FName> ResolveUnlockedSkillIds(const UObject* WorldContextObject, int32 MechanicLevel);
};
