// Copyright Melodia Project. All Rights Reserved.
// Editor utilities for building and discovering Melodia PCG graphs.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaPCGEditorLibrary.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPCGEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Log catalog + Python build instructions to the Output Log. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static void PrintPCGGraphCatalogHelp();

#if WITH_EDITOR
	/** Attempt to run melodia_pcg_bezier_builder.py (requires PythonScriptPlugin). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static bool BuildAllBezierGraphs();

	/** Run melodia_pcgex_collections.py then melodia_pcgex_builder.py (requires PCGExtendedToolkit). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static bool BuildAllPCGExGraphs();

	/** DreamWalls + Bezier portfolio graphs (placeholder meshes, no legacy/PCGEx). */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static bool BuildAllPCG();

	/** Alias for BuildAllPCG — portfolio-safe rebuild. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static bool BuildPortfolioPCG();

	/** Volume/grid scatter graphs under /Game/_PROJECT/PCG/Graphs/Simple/ */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static bool BuildSimplePCGGraphs();

	/** Duplicate greybox test maps for each catalog graph if missing. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static int32 EnsureBezierTestLevels();
#endif
};
