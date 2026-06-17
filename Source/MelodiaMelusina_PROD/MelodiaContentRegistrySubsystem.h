// Loads song/encounter DataAssets; falls back to C++ demo catalogs.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MelodiaEncounterDataAsset.h"
#include "MelodiaSongSkillDataAsset.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaContentRegistrySubsystem.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaContentRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Content path scanned on init (Asset Registry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Content")
	FName SongSkillContentPath = TEXT("/Game/Melodia/Data/Skills");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|Content")
	FName EncounterContentPath = TEXT("/Game/Melodia/Data/Encounters");

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Content")
	TArray<FMelodiaSongSkillRecipe> CachedSongSkills;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Content")
	TArray<TObjectPtr<UMelodiaEncounterDataAsset>> CachedEncounters;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content", meta = (WorldContext = "WorldContextObject"))
	static UMelodiaContentRegistrySubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	void RefreshFromAssetRegistry();

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	TArray<FMelodiaSongSkillRecipe> GetAllSongSkills() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	bool FindSongSkill(FName SkillId, FMelodiaSongSkillRecipe& OutSkill) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	TArray<FName> GetSkillIdsUnlockedAtOrBelowLevel(int32 MechanicLevel) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	FName GetSkillIdForMechanicLevel(int32 MechanicLevel) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	UMelodiaEncounterDataAsset* FindEncounterById(FName EncounterId) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Content")
	UMelodiaEncounterDataAsset* GetDefaultEncounter() const;
};
