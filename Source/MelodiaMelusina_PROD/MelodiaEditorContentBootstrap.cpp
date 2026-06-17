#include "MelodiaEditorContentBootstrap.h"

#if WITH_EDITOR

#include "Containers/Ticker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "IAssetTools.h"
#include "MelodiaEncounterDataAsset.h"
#include "MelodiaSongSkillDataAsset.h"
#include "MelodiaSongSkillLibrary.h"
#include "UObject/SavePackage.h"

void UMelodiaEditorContentBootstrap::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Editor startup can crash if we create/save assets before engine startup module loading completes
	// (AssetRegistry may be unable to tick/complete yet). Defer until the editor is fully initialized.
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([](float)
		{
			if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
			{
				return true;
			}

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			if (AssetRegistryModule.Get().IsLoadingAssets())
			{
				return true;
			}

			UMelodiaEditorContentBootstrap::EnsureDefaultContentAssets();
			return false;
		}),
		5.0f);
}

bool UMelodiaEditorContentBootstrap::EnsureContentFolder(const FString& FolderPath)
{
	if (UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
	{
		return true;
	}

	return UEditorAssetLibrary::MakeDirectory(FolderPath);
}

bool UMelodiaEditorContentBootstrap::SaveNewDataAsset(UObject* Asset, const FString& ObjectPath)
{
	if (!Asset)
	{
		return false;
	}

	Asset->MarkPackageDirty();
	return UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
}

bool UMelodiaEditorContentBootstrap::EnsureDefaultContentAssets()
{
	const FString SkillsFolder = TEXT("/Game/Melodia/Data/Skills");
	const FString EncountersFolder = TEXT("/Game/Melodia/Data/Encounters");
	EnsureContentFolder(TEXT("/Game/Melodia/Data"));
	EnsureContentFolder(SkillsFolder);
	EnsureContentFolder(EncountersFolder);

	bool bCreatedAny = false;

	const FString SkillAssetPath = SkillsFolder / TEXT("DA_Skill_Lv01_StarlitPing");
	if (!UEditorAssetLibrary::DoesAssetExist(SkillAssetPath))
	{
		FMelodiaSongSkillRecipe Recipe;
		if (UMelodiaSongSkillLibrary::FindSongSkill(TEXT("Skill_Lv01_StarlitPing"), Recipe))
		{
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			UMelodiaSongSkillDataAsset* SkillAsset = Cast<UMelodiaSongSkillDataAsset>(
				AssetTools.CreateAsset(
					TEXT("DA_Skill_Lv01_StarlitPing"),
					SkillsFolder,
					UMelodiaSongSkillDataAsset::StaticClass(),
					nullptr));
			if (SkillAsset)
			{
				SkillAsset->Recipe = Recipe;
				if (SaveNewDataAsset(SkillAsset, SkillAssetPath))
				{
					bCreatedAny = true;
					UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created %s"), *SkillAssetPath);
				}
			}
		}
	}

	const FString EncounterAssetPath = EncountersFolder / TEXT("DA_Encounter_DefaultSlime");
	if (!UEditorAssetLibrary::DoesAssetExist(EncounterAssetPath))
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		UMelodiaEncounterDataAsset* EncounterAsset = Cast<UMelodiaEncounterDataAsset>(
			AssetTools.CreateAsset(
				TEXT("DA_Encounter_DefaultSlime"),
				EncountersFolder,
				UMelodiaEncounterDataAsset::StaticClass(),
				nullptr));
		if (EncounterAsset)
		{
			EncounterAsset->EncounterId = TEXT("DefaultSlime");
			EncounterAsset->DisplayName = INVTEXT("Garden Slime");
			EncounterAsset->EncounterLevel = 0;
			EncounterAsset->EnemyElement = EMelodiaSpellElement::Forte;
			if (SaveNewDataAsset(EncounterAsset, EncounterAssetPath))
			{
				bCreatedAny = true;
				UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created %s"), *EncounterAssetPath);
			}
		}
	}

	if (UEditorAssetLibrary::DoesAssetExist(EncounterAssetPath))
	{
		if (UMelodiaEncounterDataAsset* EncounterAsset = Cast<UMelodiaEncounterDataAsset>(UEditorAssetLibrary::LoadAsset(EncounterAssetPath)))
		{
			if (EncounterAsset->EncounterLevel > 0)
			{
				EncounterAsset->EncounterLevel = 0;
				if (SaveNewDataAsset(EncounterAsset, EncounterAssetPath))
				{
					bCreatedAny = true;
					UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap set %s EncounterLevel=0 (use player mechanic level)."), *EncounterAssetPath);
				}
			}
		}
	}

	if (bCreatedAny)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap finished (new or patched DataAssets)."));
	}

	return bCreatedAny;
}

#endif
