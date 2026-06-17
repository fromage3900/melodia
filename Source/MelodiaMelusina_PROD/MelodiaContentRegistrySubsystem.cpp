#include "MelodiaContentRegistrySubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

void UMelodiaContentRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RefreshFromAssetRegistry();
}

UMelodiaContentRegistrySubsystem* UMelodiaContentRegistrySubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UMelodiaContentRegistrySubsystem>() : nullptr;
}

void UMelodiaContentRegistrySubsystem::RefreshFromAssetRegistry()
{
	CachedSongSkills = UMelodiaSongSkillLibrary::BuildDemoSongSkills();
	CachedEncounters.Reset();

	TMap<FName, int32> SkillIndexById;
	for (int32 Index = 0; Index < CachedSongSkills.Num(); ++Index)
	{
		SkillIndexById.Add(CachedSongSkills[Index].SkillId, Index);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	auto LoadAssetsInPath = [&AssetRegistry](const FName PackagePath, const UClass* AssetClass, TArray<FAssetData>& OutAssets)
	{
		FARFilter Filter;
		Filter.PackagePaths.Add(PackagePath);
		Filter.bRecursivePaths = true;
		Filter.ClassPaths.Add(AssetClass->GetClassPathName());
		AssetRegistry.GetAssets(Filter, OutAssets);
	};

	TArray<FAssetData> SkillAssets;
	LoadAssetsInPath(SongSkillContentPath, UMelodiaSongSkillDataAsset::StaticClass(), SkillAssets);
	for (const FAssetData& AssetData : SkillAssets)
	{
		if (const UMelodiaSongSkillDataAsset* SkillAsset = Cast<UMelodiaSongSkillDataAsset>(AssetData.GetAsset()))
		{
			const FMelodiaSongSkillRecipe& Recipe = SkillAsset->Recipe;
			if (Recipe.SkillId.IsNone())
			{
				continue;
			}

			if (const int32* ExistingIndex = SkillIndexById.Find(Recipe.SkillId))
			{
				CachedSongSkills[*ExistingIndex] = Recipe;
			}
			else
			{
				SkillIndexById.Add(Recipe.SkillId, CachedSongSkills.Num());
				CachedSongSkills.Add(Recipe);
			}
		}
	}

	TArray<FAssetData> EncounterAssets;
	LoadAssetsInPath(EncounterContentPath, UMelodiaEncounterDataAsset::StaticClass(), EncounterAssets);
	for (const FAssetData& AssetData : EncounterAssets)
	{
		if (UMelodiaEncounterDataAsset* EncounterAsset = Cast<UMelodiaEncounterDataAsset>(AssetData.GetAsset()))
		{
			CachedEncounters.Add(EncounterAsset);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia content registry: %d skills, %d encounters (demo fallback merged)."),
		CachedSongSkills.Num(), CachedEncounters.Num());
}

TArray<FMelodiaSongSkillRecipe> UMelodiaContentRegistrySubsystem::GetAllSongSkills() const
{
	return CachedSongSkills;
}

bool UMelodiaContentRegistrySubsystem::FindSongSkill(const FName SkillId, FMelodiaSongSkillRecipe& OutSkill) const
{
	if (SkillId.IsNone())
	{
		return false;
	}

	for (const FMelodiaSongSkillRecipe& Skill : CachedSongSkills)
	{
		if (Skill.SkillId == SkillId)
		{
			OutSkill = Skill;
			return true;
		}
	}
	return false;
}

TArray<FName> UMelodiaContentRegistrySubsystem::GetSkillIdsUnlockedAtOrBelowLevel(const int32 MechanicLevel) const
{
	TArray<FName> Ids;
	for (const FMelodiaSongSkillRecipe& Skill : CachedSongSkills)
	{
		if (Skill.MechanicLevelRequired <= MechanicLevel)
		{
			Ids.Add(Skill.SkillId);
		}
	}
	return Ids;
}

FName UMelodiaContentRegistrySubsystem::GetSkillIdForMechanicLevel(const int32 MechanicLevel) const
{
	const int32 Clamped = FMath::Clamp(MechanicLevel, 1, 30);
	for (const FMelodiaSongSkillRecipe& Skill : CachedSongSkills)
	{
		if (Skill.MechanicLevelRequired == Clamped)
		{
			return Skill.SkillId;
		}
	}
	return NAME_None;
}

UMelodiaEncounterDataAsset* UMelodiaContentRegistrySubsystem::FindEncounterById(const FName EncounterId) const
{
	for (UMelodiaEncounterDataAsset* Encounter : CachedEncounters)
	{
		if (Encounter && Encounter->EncounterId == EncounterId)
		{
			return Encounter;
		}
	}
	return nullptr;
}

UMelodiaEncounterDataAsset* UMelodiaContentRegistrySubsystem::GetDefaultEncounter() const
{
	if (UMelodiaEncounterDataAsset* DefaultEncounter = FindEncounterById(TEXT("DefaultSlime")))
	{
		return DefaultEncounter;
	}
	return CachedEncounters.Num() > 0 ? CachedEncounters[0] : nullptr;
}
