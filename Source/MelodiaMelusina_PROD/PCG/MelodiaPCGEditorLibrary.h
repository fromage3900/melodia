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

	/** Duplicate greybox test maps for each catalog graph if missing. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Editor", CallInEditor)
	static int32 EnsureBezierTestLevels();
#endif
};
