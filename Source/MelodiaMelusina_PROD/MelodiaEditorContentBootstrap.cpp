#include "MelodiaEditorContentBootstrap.h"

#if WITH_EDITOR

#include "MelodiaPCGEditorLibrary.h"

#include "Containers/Ticker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "IAssetTools.h"
#include "MelodiaEncounterDataAsset.h"
#include "MelodiaGameplayLoopTestDirector.h"
#include "MelodiaSongSkillDataAsset.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaWorldEnemy.h"
#include "MelodiaPickableFlower.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaNPCBase.h"
#include "MelodiaRestPoint.h"
#include "MelodiaPortal.h"
#include "MelodiaReverieRunManager.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet2/KismetEditorUtilities.h"
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
			UMelodiaEditorContentBootstrap::EnsureTestLoopBlueprintAssets();
			UMelodiaEditorContentBootstrap::EnsureTestLoopLevelAsset();
			UMelodiaEditorContentBootstrap::RepopulateGameplayLoopTestLevel();
			UMelodiaEditorContentBootstrap::EnsurePCGDemoLevelAsset();
			UMelodiaEditorContentBootstrap::EnsurePortfolioTerraceLevelAsset();
			UMelodiaPCGEditorLibrary::EnsureBezierTestLevels();
			UMelodiaEditorContentBootstrap::EnsureMelodiaPortfolioMenuBridge();
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

bool UMelodiaEditorContentBootstrap::EnsureChildBlueprint(const FString& AssetPath, UClass* ParentClass)
{
	if (!ParentClass || UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
	const FString PackagePath = FPackageName::GetLongPackagePath(AssetPath);
	UPackage* Package = CreatePackage(*(PackagePath / AssetName));
	if (!Package)
	{
		return false;
	}

	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		FName(*AssetName),
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None);
	if (!Blueprint)
	{
		return false;
	}

	Blueprint->MarkPackageDirty();
	const bool bSaved = UEditorAssetLibrary::SaveLoadedAsset(Blueprint, false);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created blueprint %s"), *AssetPath);
	}
	return bSaved;
}

bool UMelodiaEditorContentBootstrap::EnsureTestLoopBlueprintAssets()
{
	const FString TestLoopFolder = TEXT("/Game/Melodia/TestLoop");
	EnsureContentFolder(TestLoopFolder);

	bool bCreatedAny = false;
	const TPair<FString, UClass*> BlueprintRows[] = {
		{TEXT("BP_TestLoop_EncounterGate"), AMelodiaEncounterTrigger::StaticClass()},
		{TEXT("BP_TestLoop_WorldEnemy"), AMelodiaWorldEnemy::StaticClass()},
		{TEXT("BP_TestLoop_QuestGiver"), AMelodiaNPCBase::StaticClass()},
		{TEXT("BP_TestLoop_RestBed"), AMelodiaRestPoint::StaticClass()},
		{TEXT("BP_TestLoop_Portal"), AMelodiaPortal::StaticClass()},
		{TEXT("BP_TestLoop_Flower"), AMelodiaPickableFlower::StaticClass()},
		{TEXT("BP_TestLoop_Director"), AMelodiaGameplayLoopTestDirector::StaticClass()},
	};

	for (const TPair<FString, UClass*>& Row : BlueprintRows)
	{
		if (EnsureChildBlueprint(TestLoopFolder / Row.Key, Row.Value))
		{
			bCreatedAny = true;
		}
	}

	if (bCreatedAny)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap finished test-loop blueprint stubs under %s."), *TestLoopFolder);
	}

	return bCreatedAny;
}

bool UMelodiaEditorContentBootstrap::EnsureTestLoopLevelAsset()
{
	const FString MapAssetPath = TEXT("/Game/Melodia/Levels/L_MelodiaGameplayLoopTest");
	if (UEditorAssetLibrary::DoesAssetExist(MapAssetPath))
	{
		return false;
	}

	if (!GEditor)
	{
		return false;
	}

	EnsureContentFolder(TEXT("/Game/Melodia/Levels"));

	UWorld* NewWorld = GEditor->NewMap(false);
	if (!NewWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to create blank map for gameplay loop test."));
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMelodiaGameplayLoopTestDirector* Director = NewWorld->SpawnActor<AMelodiaGameplayLoopTestDirector>(
		AMelodiaGameplayLoopTestDirector::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	if (Director)
	{
		Director->bAutoBuildLayout = true;
		Director->bApplyToGameModeOnBeginPlay = true;
		Director->Tags.Add(TEXT("Melodia.TestLoop.Director"));
	}

	BakeGameplayLoopTestLayout(NewWorld);

	if (UClass* QuestManagerClass = LoadClass<AActor>(nullptr, TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C")))
	{
		NewWorld->SpawnActor<AActor>(QuestManagerClass, FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator, Params);
	}

	if (ADirectionalLight* Sun = NewWorld->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(),
		FVector(0.0f, 0.0f, 800.0f),
		FRotator(-50.0f, 45.0f, 0.0f),
		Params))
	{
		if (UDirectionalLightComponent* Light = Sun->GetComponent())
		{
			Light->SetIntensity(8.0f);
		}
	}

	if (ASkyLight* Sky = NewWorld->SpawnActor<ASkyLight>(
		ASkyLight::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params))
	{
		if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
		{
			SkyComponent->SetIntensity(1.2f);
			SkyComponent->RecaptureSky();
		}
	}

	if (AWorldSettings* WorldSettings = NewWorld->GetWorldSettings())
	{
		if (UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Melodia/Core/BP_MelodiaRhythmGameMode.BP_MelodiaRhythmGameMode_C")))
		{
			WorldSettings->DefaultGameMode = GameModeClass;
		}
	}

	const FString MapFilename = FPackageName::LongPackageNameToFilename(MapAssetPath, FPackageName::GetMapPackageExtension());
	const bool bSaved = FEditorFileUtils::SaveLevel(NewWorld->PersistentLevel, MapFilename);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created test loop level %s"), *MapAssetPath);
	}
	return bSaved;
}

bool UMelodiaEditorContentBootstrap::EnsureMelodiaPortfolioMenuBridge()
{
	return EnsureContentFolder(TEXT("/Game/Melodia/Menu"));
}

bool UMelodiaEditorContentBootstrap::BakeGameplayLoopTestLayout(UWorld* World)
{
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMelodiaGameplayLoopTestDirector* Director = nullptr;
	for (TActorIterator<AMelodiaGameplayLoopTestDirector> It(World); It; ++It)
	{
		Director = *It;
		break;
	}

	if (!Director)
	{
		Director = World->SpawnActor<AMelodiaGameplayLoopTestDirector>(
			AMelodiaGameplayLoopTestDirector::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (Director)
		{
			Director->Tags.Add(TEXT("Melodia.TestLoop.Director"));
		}
	}

	if (!Director)
	{
		return false;
	}

	Director->bApplyToGameModeOnBeginPlay = false;
	const bool bBuilt = Director->BuildLayout(true);
	Director->bAutoBuildLayout = false;

	if (!bBuilt)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: BuildLayout returned false for gameplay loop test."));
	}

	int32 QuestManagerCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("QuestManager")))
		{
			++QuestManagerCount;
		}
	}
	if (QuestManagerCount == 0)
	{
		if (UClass* QuestManagerClass = LoadClass<AActor>(nullptr, TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C")))
		{
			World->SpawnActor<AActor>(QuestManagerClass, FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator, Params);
		}
	}

	bool bHasSun = false;
	bool bHasSky = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA<ADirectionalLight>())
		{
			bHasSun = true;
		}
		if (It->IsA<ASkyLight>())
		{
			bHasSky = true;
		}
	}

	if (!bHasSun)
	{
		if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			FVector(0.0f, 0.0f, 800.0f),
			FRotator(-50.0f, 45.0f, 0.0f),
			Params))
		{
			if (UDirectionalLightComponent* Light = Sun->GetComponent())
			{
				Light->SetIntensity(8.0f);
			}
		}
	}

	if (!bHasSky)
	{
		if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params))
		{
			if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
			{
				SkyComponent->SetIntensity(1.2f);
				SkyComponent->RecaptureSky();
			}
		}
	}

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		if (UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Melodia/Core/BP_MelodiaRhythmGameMode.BP_MelodiaRhythmGameMode_C")))
		{
			WorldSettings->DefaultGameMode = GameModeClass;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap baked gameplay loop test layout (gate=%s)."),
		Director->EncounterGate ? *Director->EncounterGate->GetActorLocation().ToString() : TEXT("missing"));
	return bBuilt;
}

bool UMelodiaEditorContentBootstrap::RepopulateGameplayLoopTestLevel()
{
	const FString MapAssetPath = TEXT("/Game/Melodia/Levels/L_MelodiaGameplayLoopTest");
	if (!UEditorAssetLibrary::DoesAssetExist(MapAssetPath) || !GEditor)
	{
		return false;
	}

	const FString MapFilename = FPackageName::LongPackageNameToFilename(MapAssetPath, FPackageName::GetMapPackageExtension());
	if (!FEditorFileUtils::LoadMap(MapFilename, false, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to load %s for repopulation."), *MapAssetPath);
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return false;
	}

	int32 EncounterCount = 0;
	int32 FlowerCount = 0;
	for (TActorIterator<AMelodiaEncounterTrigger> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("Melodia.TestLoop.Enemy")))
		{
			++EncounterCount;
		}
	}
	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		++FlowerCount;
	}

	if (EncounterCount > 0 && FlowerCount >= 3)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap repopulating sparse test level (encounters=%d flowers=%d)."),
		EncounterCount, FlowerCount);

	const bool bBaked = BakeGameplayLoopTestLayout(World);
	const bool bSaved = bBaked && FEditorFileUtils::SaveLevel(World->PersistentLevel, MapFilename);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap saved repopulated test level %s."), *MapAssetPath);
	}
	return bSaved;
}

bool UMelodiaEditorContentBootstrap::EnsurePCGDemoLevelAsset()
{
	const FString DestPath = TEXT("/Game/Melodia/Levels/L_MelodiaPCGDemo");
	const FString SourcePath = TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_TerraceGarden");

	EnsureContentFolder(TEXT("/Game/Melodia/Levels"));

	if (!UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: missing source PCG test map %s."), *SourcePath);
			return false;
		}

		if (!UEditorAssetLibrary::DuplicateAsset(SourcePath, DestPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to duplicate %s to %s."), *SourcePath, *DestPath);
			return false;
		}

		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created PCG demo level %s from Terrace Garden test map."), *DestPath);
	}

	return RepopulatePCGDemoLevel();
}

bool UMelodiaEditorContentBootstrap::RepopulatePCGDemoLevel()
{
	const FString MapAssetPath = TEXT("/Game/Melodia/Levels/L_MelodiaPCGDemo");
	if (!UEditorAssetLibrary::DoesAssetExist(MapAssetPath) || !GEditor)
	{
		return false;
	}

	const FString MapFilename = FPackageName::LongPackageNameToFilename(MapAssetPath, FPackageName::GetMapPackageExtension());
	if (!FEditorFileUtils::LoadMap(MapFilename, false, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to load %s for PCG demo repopulation."), *MapAssetPath);
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bTouched = false;

	bool bHasQuestManager = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("QuestManager")))
		{
			bHasQuestManager = true;
			break;
		}
	}
	if (!bHasQuestManager)
	{
		if (UClass* QuestManagerClass = LoadClass<AActor>(nullptr, TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C")))
		{
			World->SpawnActor<AActor>(QuestManagerClass, FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params);
			bTouched = true;
		}
	}

	bool bHasPlayerStart = false;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		bHasPlayerStart = true;
		break;
	}
	if (!bHasPlayerStart)
	{
		World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params);
		bTouched = true;
	}

	int32 TutorCount = 0;
	for (TActorIterator<AMelodiaNPCBase> It(World); It; ++It)
	{
		++TutorCount;
	}
	if (TutorCount == 0)
	{
		if (UClass* TutorClass = LoadClass<AMelodiaNPCBase>(nullptr, TEXT("/Game/Melodia/TestLoop/BP_TestLoop_QuestGiver.BP_TestLoop_QuestGiver_C")))
		{
			World->SpawnActor<AMelodiaNPCBase>(TutorClass, FVector(-400.0f, -500.0f, 120.0f), FRotator(0.0f, 45.0f, 0.0f), Params);
			bTouched = true;
		}
	}

	int32 FlowerCount = 0;
	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		++FlowerCount;
	}
	if (FlowerCount < 3)
	{
		if (UClass* FlowerClass = LoadClass<AMelodiaPickableFlower>(nullptr, TEXT("/Game/Melodia/TestLoop/BP_TestLoop_Flower.BP_TestLoop_Flower_C")))
		{
			const FVector FlowerOffsets[] = {
				FVector(-300.0f, -200.0f, 120.0f),
				FVector(-150.0f, -80.0f, 120.0f),
				FVector(0.0f, 120.0f, 120.0f),
			};
			for (const FVector& Offset : FlowerOffsets)
			{
				if (FlowerCount >= 3)
				{
					break;
				}
				World->SpawnActor<AMelodiaPickableFlower>(FlowerClass, Offset, FRotator::ZeroRotator, Params);
				++FlowerCount;
				bTouched = true;
			}
		}
	}

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		if (UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Melodia/Core/BP_MelodiaRhythmGameMode.BP_MelodiaRhythmGameMode_C")))
		{
			if (WorldSettings->DefaultGameMode != GameModeClass)
			{
				WorldSettings->DefaultGameMode = GameModeClass;
				bTouched = true;
			}
		}
	}

	if (!bTouched)
	{
		return false;
	}

	const bool bSaved = FEditorFileUtils::SaveLevel(World->PersistentLevel, MapFilename);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap saved PCG demo level %s."), *MapAssetPath);
	}
	return bSaved;
}

bool UMelodiaEditorContentBootstrap::EnsurePortfolioTerraceLevelAsset()
{
	const FString DestPath = TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace");
	const FString SourcePath = TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_Greybox");

	EnsureContentFolder(TEXT("/Game/Melodia/Levels"));

	if (!UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: missing source greybox map %s."), *SourcePath);
			return false;
		}

		if (!UEditorAssetLibrary::DuplicateAsset(SourcePath, DestPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to duplicate %s to %s."), *SourcePath, *DestPath);
			return false;
		}

		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap created portfolio terrace level %s."), *DestPath);
	}

	return RepopulatePortfolioTerraceLevel();
}

bool UMelodiaEditorContentBootstrap::RepopulatePortfolioTerraceLevel()
{
	const FString MapAssetPath = TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace");
	if (!UEditorAssetLibrary::DoesAssetExist(MapAssetPath) || !GEditor)
	{
		return false;
	}

	const FString MapFilename = FPackageName::LongPackageNameToFilename(MapAssetPath, FPackageName::GetMapPackageExtension());
	if (!FEditorFileUtils::LoadMap(MapFilename, false, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia editor bootstrap: failed to load %s for portfolio repopulation."), *MapAssetPath);
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bTouched = false;

	bool bHasPlayerStart = false;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		bHasPlayerStart = true;
		break;
	}
	if (!bHasPlayerStart)
	{
		World->SpawnActor<APlayerStart>(
			APlayerStart::StaticClass(),
			FVector(-2400.0f, -1800.0f, 140.0f),
			FRotator(0.0f, 35.0f, 0.0f),
			Params);
		bTouched = true;
	}

	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		if (UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Melodia/Core/BP_MelodiaRhythmGameMode.BP_MelodiaRhythmGameMode_C")))
		{
			if (WorldSettings->DefaultGameMode != GameModeClass)
			{
				WorldSettings->DefaultGameMode = GameModeClass;
				bTouched = true;
			}
		}
	}

	if (!bTouched)
	{
		return false;
	}

	const bool bSaved = FEditorFileUtils::SaveLevel(World->PersistentLevel, MapFilename);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia editor bootstrap saved portfolio terrace level %s."), *MapAssetPath);
	}
	return bSaved;
}

#endif
