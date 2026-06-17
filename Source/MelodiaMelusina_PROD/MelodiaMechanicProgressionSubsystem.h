// Runtime mechanic level progression (demo Lv 1–30) tied to quests and Reverie presets.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MelodiaMechanicProgressionTypes.h"
#include "MelodiaMechanicProgressionSubsystem.generated.h"

class AMelodiaReverieRunManager;
class UMelodiaSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMelodiaMechanicLevelChanged, int32, NewLevel, int32, PreviousLevel);

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaMechanicProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Mechanic")
	FMelodiaMechanicProgressionState State;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Mechanic")
	TArray<FMelodiaMechanicTierDefinition> TierCatalog;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Mechanic")
	TArray<FMelodiaLocationLevelPreset> LocationPresetCatalog;

	UPROPERTY(BlueprintAssignable, Category = "Melodia|Mechanic")
	FMelodiaMechanicLevelChanged OnMechanicLevelChanged;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void ResetToDemoDefaults();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	bool GrantMechanicXP(int32 Amount, const FString& Reason);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	int32 GetMechanicLevel() const { return State.MechanicLevel; }

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	int32 GetXPRequiredForNextLevel() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	EMelodiaMechanicTier GetCurrentTier() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	FString GetCurrentTierDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	bool IsPresetUnlocked(FName PresetId) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	TArray<FMelodiaLocationLevelPreset> GetUnlockedLocationPresets() const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void ApplyUnlockedPresetsToReverieRunManager(AMelodiaReverieRunManager* RunManager) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void NotifyQuestSystemOfProgress(class UWorld* World) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void SyncHUD(class UWorld* World) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void LoadFromSave(const UMelodiaSaveGame* SaveData);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void WriteToSave(UMelodiaSaveGame* SaveData) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void CycleActiveSkill();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Mechanic")
	void EquipKeyForElement(EMelodiaSpellElement Element);

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	bool IsSkillUnlocked(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	bool IsKeyUnlocked(FName KeyId) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Mechanic")
	FText GetActiveSkillDisplayName() const;

private:
	void UnlockContentUpToLevel(int32 MechanicLevel);
	void UnlockPresetsUpToLevel(int32 MechanicLevel);
};
