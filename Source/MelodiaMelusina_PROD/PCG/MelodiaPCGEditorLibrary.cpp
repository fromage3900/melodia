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
	UE_LOG(LogTemp, Log, TEXT("Build graphs: Melodia.BuildPCGGraphs (console) or run Scripts/PCG/melodia_pcg_bezier_builder.py"));
	UE_LOG(LogTemp, Log, TEXT("Level building: place AMelodiaPCGLevelKit, pick GraphId, click Generate Now."));
}

#if WITH_EDITOR

bool UMelodiaPCGEditorLibrary::BuildAllBezierGraphs()
{
	const FString ScriptPath = UMelodiaPCGGraphRegistry::GetGraphBuildScriptPath();
	const FString Python = FString::Printf(
		TEXT("import importlib.util\n")
		TEXT("spec = importlib.util.spec_from_file_location('melodia_pcg_bezier_builder', r'%s')\n")
		TEXT("mod = importlib.util.module_from_spec(spec)\n")
		TEXT("spec.loader.exec_module(mod)\n")
		TEXT("mod.build_all()\n"),
		*ScriptPath);

	if (IPythonScriptPlugin* PythonPlugin = FModuleManager::LoadModulePtr<IPythonScriptPlugin>("PythonScriptPlugin"))
	{
		const bool bOk = PythonPlugin->ExecPythonCommand(*Python);
		UE_LOG(LogTemp, Log, TEXT("MelodiaPCGEditorLibrary::BuildAllBezierGraphs %s"), bOk ? TEXT("OK") : TEXT("FAILED"));
		return bOk;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("PythonScriptPlugin not available. Run manually:\n%s"),
		*ScriptPath);
	return false;
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

int32 UMelodiaPCGEditorLibrary::EnsureBezierTestLevels()
{
	return 0;
}

#endif
