// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGEditorLibrary.h"
#include "MelodiaPCGGraphRegistry.h"

#if WITH_EDITOR
#include "EditorAssetLibrary.h"
#include "IPythonScriptPlugin.h"
#include "Modules/ModuleManager.h"
#endif

void UMelodiaPCGEditorLibrary::PrintPCGGraphCatalogHelp()
{
	UE_LOG(LogTemp, Log, TEXT("=== Melodia PCG Graph Catalog ==="));
	for (const FMelodiaPCGGraphCatalogEntry& Entry : UMelodiaPCGGraphRegistry::GetGraphCatalog())
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s — %s"),
			static_cast<int32>(Entry.GraphId),
			*Entry.DisplayName.ToString(),
			*Entry.GraphAsset.ToString());
	}
	UE_LOG(LogTemp, Log, TEXT("Build Bezier graphs: Melodia.BuildPCGGraphs"));
	UE_LOG(LogTemp, Log, TEXT("Build simple scatter graphs: Melodia.BuildSimplePCG"));
	UE_LOG(LogTemp, Log, TEXT("Build portfolio (DreamWalls+Bezier): Melodia.BuildAllPCG"));
	UE_LOG(LogTemp, Log, TEXT("Portfolio manifest: %s"), *UMelodiaPCGGraphRegistry::GetPortfolioManifestPath());
	UE_LOG(LogTemp, Log, TEXT("Build PCGEx graphs: Melodia.BuildPCGExGraphs"));
	UE_LOG(LogTemp, Log, TEXT("Level building: place AMelodiaPCGLevelKit, pick GraphId, click Generate Now."));
}

#if WITH_EDITOR

namespace MelodiaPCGEditorLibraryPrivate
{
	static bool RunPythonScriptFile(const FString& ScriptPath, const FString& EntryCall)
	{
		const FString Python = FString::Printf(
			TEXT("import importlib.util, sys\n")
			TEXT("spec = importlib.util.spec_from_file_location('melodia_pcg_script', r'%s')\n")
			TEXT("mod = importlib.util.module_from_spec(spec)\n")
			TEXT("sys.modules['melodia_pcg_script'] = mod\n")
			TEXT("spec.loader.exec_module(mod)\n")
			TEXT("%s\n"),
			*ScriptPath,
			*EntryCall);

		if (IPythonScriptPlugin* PythonPlugin = FModuleManager::LoadModulePtr<IPythonScriptPlugin>("PythonScriptPlugin"))
		{
			const bool bOk = PythonPlugin->ExecPythonCommand(*Python);
			UE_LOG(LogTemp, Log, TEXT("MelodiaPCGEditorLibrary python %s %s"), *ScriptPath, bOk ? TEXT("OK") : TEXT("FAILED"));
			return bOk;
		}

		UE_LOG(LogTemp, Warning, TEXT("PythonScriptPlugin not available. Run manually: %s"), *ScriptPath);
		return false;
	}
}

bool UMelodiaPCGEditorLibrary::BuildAllBezierGraphs()
{
	return MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		UMelodiaPCGGraphRegistry::GetGraphBuildScriptPath(),
		TEXT("mod.build_all()"));
}

bool UMelodiaPCGEditorLibrary::BuildAllPCGExGraphs()
{
	const bool bCollections = MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		UMelodiaPCGGraphRegistry::GetPCGExCollectionsScriptPath(),
		TEXT("mod.build_all()"));
	const bool bGraphs = MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		UMelodiaPCGGraphRegistry::GetPCGExBuildScriptPath(),
		TEXT("mod.build_all()"));
	return bCollections && bGraphs;
}

bool UMelodiaPCGEditorLibrary::BuildAllPCG()
{
	const bool bDreamWalls = MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		UMelodiaPCGGraphRegistry::GetDreamWallsBuildScriptPath(),
		TEXT("mod.build_dream_walls()"));
	const bool bBezier = BuildAllBezierGraphs();
	const bool bVanilla = MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		TEXT("G:/Melodia/Scripts/PCG/melodia_vanilla_bootstrap.py"),
		TEXT("pass"));
	const bool bEscher = MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		TEXT("G:/Melodia/Scripts/PCG/escher_nikki_builder.py"),
		TEXT("mod.build_all()"));
	const bool bPcgEx = BuildAllPCGExGraphs();
	EnsureBezierTestLevels();
	return bDreamWalls && bBezier && bVanilla && bEscher && bPcgEx;
}

bool UMelodiaPCGEditorLibrary::BuildPortfolioPCG()
{
	return BuildAllPCG();
}

bool UMelodiaPCGEditorLibrary::BuildSimplePCGGraphs()
{
	return MelodiaPCGEditorLibraryPrivate::RunPythonScriptFile(
		UMelodiaPCGGraphRegistry::GetSimplePCGBuildScriptPath(),
		TEXT("mod.build_all()"));
}

int32 UMelodiaPCGEditorLibrary::EnsureBezierTestLevels()
{
	const FString Greybox = TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_Greybox");
	int32 Created = 0;

	for (const FMelodiaPCGGraphCatalogEntry& Entry : UMelodiaPCGGraphRegistry::GetGraphCatalog())
	{
		if (Entry.GraphId == EMelodiaPCGGraphId::Custom || Entry.GraphId == EMelodiaPCGGraphId::TerraceGarden)
		{
			continue;
		}

		const FString Dest = Entry.SuggestedTestLevel.ToString();
		if (Dest.IsEmpty() || UEditorAssetLibrary::DoesAssetExist(Dest))
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesAssetExist(Greybox))
		{
			UE_LOG(LogTemp, Warning, TEXT("Missing greybox source %s"), *Greybox);
			continue;
		}

		if (UEditorAssetLibrary::DuplicateAsset(Greybox, Dest))
		{
			++Created;
			UE_LOG(LogTemp, Log, TEXT("Created test level %s"), *Dest);
		}
	}

	return Created;
}

#else

bool UMelodiaPCGEditorLibrary::BuildAllBezierGraphs()
{
	return false;
}

bool UMelodiaPCGEditorLibrary::BuildAllPCGExGraphs()
{
	return false;
}

bool UMelodiaPCGEditorLibrary::BuildAllPCG()
{
	return false;
}

bool UMelodiaPCGEditorLibrary::BuildPortfolioPCG()
{
	return false;
}

bool UMelodiaPCGEditorLibrary::BuildSimplePCGGraphs()
{
	return false;
}

int32 UMelodiaPCGEditorLibrary::EnsureBezierTestLevels()
{
	return 0;
}

#endif
