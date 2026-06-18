// Copyright Melodia Project. All Rights Reserved.
// Central registry of Melodia PCG graphs for level-kit dropdowns and editor discovery.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBezierTypes.h"
#include "MelodiaPCGGraphRegistry.generated.h"

/** One entry in the Melodia PCG graph catalog. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaPCGGraphCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	EMelodiaPCGGraphId GraphId = EMelodiaPCGGraphId::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	FSoftObjectPath GraphAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	FSoftObjectPath SuggestedTestLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	EMelodiaBezierLayoutPreset SuggestedLayoutPreset = EMelodiaBezierLayoutPreset::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melodia|PCG")
	int32 DefaultSeed = 42;
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPCGGraphRegistry : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static TArray<FMelodiaPCGGraphCatalogEntry> GetGraphCatalog();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static bool ResolveGraphEntry(EMelodiaPCGGraphId GraphId, FMelodiaPCGGraphCatalogEntry& OutEntry);

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FSoftObjectPath GetGraphAssetPath(EMelodiaPCGGraphId GraphId);

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetGraphBuildScriptPath();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetPCGExBuildScriptPath();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetPCGExCollectionsScriptPath();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetDreamWallsBuildScriptPath();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetPortfolioManifestPath();

	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Catalog")
	static FString GetSimplePCGBuildScriptPath();
};
