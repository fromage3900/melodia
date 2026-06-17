// Runtime mechanic level progression (demo Lv 1–30) tied to quests and Reverie presets.

#include "MelodiaMechanicProgressionSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "MelodiaKeySystemLibrary.h"
#include "MelodiaMechanicProgressionLibrary.h"
#include "MelodiaPartySubsystem.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaReverieRunManager.h"
#include "MelodiaRhythmHUDWidget.h"
#include "MelodiaSaveGame.h"
#include "MelodiaSongSkillLibrary.h"

void UMelodiaMechanicProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetToDemoDefaults();
}

void UMelodiaMechanicProgressionSubsystem::ResetToDemoDefaults()
{
	TierCatalog = UMelodiaMechanicProgressionLibrary::BuildDefaultTierDefinitions();
	LocationPresetCatalog = UMelodiaMechanicProgressionLibrary::BuildDemoLocationPresets();
	State = FMelodiaMechanicProgressionState();
	State.MechanicLevel = 1;
	State.MechanicXP = 0;
	State.UnlockedPresetIds.Reset();
	State.UnlockedSkillIds.Reset();
	State.UnlockedKeyIds.Reset();
	State.ActiveSkillId = UMelodiaSongSkillLibrary::GetSkillIdForMechanicLevel(1);
	State.EquippedKeyElement = EMelodiaSpellElement::Forte;
	State.bCompanionUnlocked = false;
	UnlockContentUpToLevel(State.MechanicLevel);
}

bool UMelodiaMechanicProgressionSubsystem::GrantMechanicXP(const int32 Amount, const FString& Reason)
{
	const int32 PreviousLevel = State.MechanicLevel;
	FString LevelUpSummary;
	const bool bLeveled = UMelodiaMechanicProgressionLibrary::GrantMechanicXP(State, Amount, LevelUpSummary);

	if (bLeveled)
	{
		OnMechanicLevelChanged.Broadcast(State.MechanicLevel, PreviousLevel);
		UnlockContentUpToLevel(State.MechanicLevel);
		UE_LOG(LogTemp, Log, TEXT("Melodia mechanic level %d -> %d (%s): %s"),
			PreviousLevel, State.MechanicLevel, *Reason, *LevelUpSummary);

		if (UGameInstance* GI = GetGameInstance())
		{
			NotifyQuestSystemOfProgress(GI->GetWorld());
			SyncHUD(GI->GetWorld());
		}
	}
	else if (Amount > 0)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			SyncHUD(GameInstance->GetWorld());
		}
	}

	return bLeveled;
}

int32 UMelodiaMechanicProgressionSubsystem::GetXPRequiredForNextLevel() const
{
	return UMelodiaMechanicProgressionLibrary::GetXPRequiredForNextLevel(State.MechanicLevel);
}

EMelodiaMechanicTier UMelodiaMechanicProgressionSubsystem::GetCurrentTier() const
{
	return UMelodiaMechanicProgressionLibrary::GetTierForMechanicLevel(State.MechanicLevel);
}

FString UMelodiaMechanicProgressionSubsystem::GetCurrentTierDisplayName() const
{
	return UMelodiaMechanicProgressionLibrary::GetTierDisplayName(GetCurrentTier());
}

bool UMelodiaMechanicProgressionSubsystem::IsPresetUnlocked(const FName PresetId) const
{
	if (PresetId.IsNone())
	{
		return false;
	}
	if (State.UnlockedPresetIds.Contains(PresetId))
	{
		return true;
	}
	for (const FMelodiaLocationLevelPreset& Preset : LocationPresetCatalog)
	{
		if (Preset.PresetId == PresetId)
		{
			return Preset.bUnlockedAtStart || Preset.MechanicLevelRequired <= State.MechanicLevel;
		}
	}
	return false;
}

TArray<FMelodiaLocationLevelPreset> UMelodiaMechanicProgressionSubsystem::GetUnlockedLocationPresets() const
{
	return UMelodiaMechanicProgressionLibrary::GetUnlockedPresetsForState(State);
}

void UMelodiaMechanicProgressionSubsystem::ApplyUnlockedPresetsToReverieRunManager(AMelodiaReverieRunManager* RunManager) const
{
	if (!RunManager)
	{
		return;
	}

	RunManager->AreaTemplates.Reset();
	for (const FMelodiaLocationLevelPreset& Preset : GetUnlockedLocationPresets())
	{
		RunManager->AreaTemplates.Add(UMelodiaMechanicProgressionLibrary::ToReverieAreaConfig(Preset));
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia mechanic progression applied %d unlocked location presets to ReverieRunManager (Lv %d)."),
		RunManager->AreaTemplates.Num(), State.MechanicLevel);
}

void UMelodiaMechanicProgressionSubsystem::NotifyQuestSystemOfProgress(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
	{
		It->NotifyMechanicLevelChanged(State.MechanicLevel);
		break;
	}
}

void UMelodiaMechanicProgressionSubsystem::SyncHUD(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->SetMechanicProgression(
			State.MechanicLevel,
			State.MechanicXP,
			GetXPRequiredForNextLevel(),
			GetCurrentTierDisplayName(),
			GetUnlockedLocationPresets().Num());
	}
}

void UMelodiaMechanicProgressionSubsystem::LoadFromSave(const UMelodiaSaveGame* SaveData)
{
	if (!SaveData)
	{
		ResetToDemoDefaults();
		return;
	}

	State.MechanicLevel = FMath::Clamp(SaveData->MechanicLevel, 1, UMelodiaMechanicProgressionLibrary::DemoMaxMechanicLevel);
	State.MechanicXP = FMath::Max(0, SaveData->MechanicXP);
	State.UnlockedPresetIds = SaveData->UnlockedLocationPresetIds;
	State.TotalLevelUps = SaveData->MechanicTotalLevelUps;
	State.UnlockedSkillIds = SaveData->UnlockedSkillIds;
	State.UnlockedKeyIds = SaveData->UnlockedKeyIds;
	State.ActiveSkillId = SaveData->ActiveSkillId.IsNone()
		? UMelodiaSongSkillLibrary::GetSkillIdForMechanicLevel(State.MechanicLevel)
		: SaveData->ActiveSkillId;
	State.EquippedKeyElement = SaveData->EquippedKeyElement;
	State.bCompanionUnlocked = SaveData->bCompanionUnlocked;
	UnlockContentUpToLevel(State.MechanicLevel);
}

void UMelodiaMechanicProgressionSubsystem::WriteToSave(UMelodiaSaveGame* SaveData) const
{
	if (!SaveData)
	{
		return;
	}

	SaveData->MechanicLevel = State.MechanicLevel;
	SaveData->MechanicXP = State.MechanicXP;
	SaveData->UnlockedLocationPresetIds = State.UnlockedPresetIds;
	SaveData->MechanicTotalLevelUps = State.TotalLevelUps;
	SaveData->UnlockedSkillIds = State.UnlockedSkillIds;
	SaveData->UnlockedKeyIds = State.UnlockedKeyIds;
	SaveData->ActiveSkillId = State.ActiveSkillId;
	SaveData->EquippedKeyElement = State.EquippedKeyElement;
	SaveData->bCompanionUnlocked = State.bCompanionUnlocked;
}

void UMelodiaMechanicProgressionSubsystem::CycleActiveSkill()
{
	const TArray<FName> Unlocked = UMelodiaSongSkillLibrary::GetSkillIdsUnlockedAtOrBelowLevel(State.MechanicLevel);
	if (Unlocked.Num() == 0)
	{
		return;
	}

	int32 CurrentIndex = Unlocked.IndexOfByKey(State.ActiveSkillId);
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = 0;
	}
	else
	{
		CurrentIndex = (CurrentIndex + 1) % Unlocked.Num();
	}

	State.ActiveSkillId = Unlocked[CurrentIndex];

	if (UGameInstance* GI = GetGameInstance())
	{
		for (TActorIterator<AMelodiaQuestManagerBase> It(GI->GetWorld()); It; ++It)
		{
			It->NotifySkillUsed(State.ActiveSkillId);
			break;
		}
		SyncHUD(GI->GetWorld());
	}
}

void UMelodiaMechanicProgressionSubsystem::EquipKeyForElement(const EMelodiaSpellElement Element)
{
	State.EquippedKeyElement = Element;

	if (UGameInstance* GI = GetGameInstance())
	{
		for (TActorIterator<AMelodiaQuestManagerBase> It(GI->GetWorld()); It; ++It)
		{
			It->NotifyKeyEquipped(Element);
			break;
		}
		SyncHUD(GI->GetWorld());
	}
}

bool UMelodiaMechanicProgressionSubsystem::IsSkillUnlocked(const FName SkillId) const
{
	return State.UnlockedSkillIds.Contains(SkillId);
}

bool UMelodiaMechanicProgressionSubsystem::IsKeyUnlocked(const FName KeyId) const
{
	return State.UnlockedKeyIds.Contains(KeyId);
}

FText UMelodiaMechanicProgressionSubsystem::GetActiveSkillDisplayName() const
{
	FMelodiaSongSkillRecipe Skill;
	if (UMelodiaSongSkillLibrary::FindSongSkill(State.ActiveSkillId, Skill))
	{
		return Skill.DisplayName;
	}
	return FText::FromName(State.ActiveSkillId);
}

void UMelodiaMechanicProgressionSubsystem::UnlockContentUpToLevel(const int32 MechanicLevel)
{
	UnlockPresetsUpToLevel(MechanicLevel);

	for (const FMelodiaSongSkillRecipe& Skill : UMelodiaSongSkillLibrary::BuildDemoSongSkills())
	{
		if (Skill.MechanicLevelRequired <= MechanicLevel)
		{
			State.UnlockedSkillIds.AddUnique(Skill.SkillId);
		}
	}

	for (const FMelodiaElementKeyDefinition& Key : UMelodiaKeySystemLibrary::BuildDemoElementKeys())
	{
		if (Key.MechanicLevelRequired <= MechanicLevel)
		{
			State.UnlockedKeyIds.AddUnique(Key.KeyId);
		}
	}

	if (MechanicLevel >= 3 && State.UnlockedKeyIds.Contains(TEXT("Key_Lv03_Forte")))
	{
		State.EquippedKeyElement = EMelodiaSpellElement::Forte;
	}

	if (MechanicLevel >= 8)
	{
		State.bCompanionUnlocked = true;
	}

	if (State.ActiveSkillId.IsNone() || !State.UnlockedSkillIds.Contains(State.ActiveSkillId))
	{
		State.ActiveSkillId = UMelodiaSongSkillLibrary::GetSkillIdForMechanicLevel(
			FMath::Clamp(MechanicLevel, 1, UMelodiaMechanicProgressionLibrary::DemoMaxMechanicLevel));
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMelodiaPartySubsystem* Party = GI->GetSubsystem<UMelodiaPartySubsystem>())
		{
			Party->SyncFromProgression();
		}
	}
}

void UMelodiaMechanicProgressionSubsystem::UnlockPresetsUpToLevel(const int32 MechanicLevel)
{
	for (const FMelodiaLocationLevelPreset& Preset : LocationPresetCatalog)
	{
		if (Preset.MechanicLevelRequired <= MechanicLevel || Preset.bUnlockedAtStart)
		{
			UMelodiaMechanicProgressionLibrary::EnsurePresetUnlocked(State, Preset.PresetId);
		}
	}
}
