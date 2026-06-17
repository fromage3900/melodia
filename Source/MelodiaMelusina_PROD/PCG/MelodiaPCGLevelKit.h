// Copyright Melodia Project. All Rights Reserved.
// Drop-in PCG volume actor for fast level building — pick a graph, set seed, generate.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaBezierTypes.h"
#include "MelodiaPCGLevelKit.generated.h"

class UPCGComponent;
class UPCGGraph;

UCLASS(Blueprintable, meta = (DisplayName = "Melodia PCG Level Kit"))
class MELODIAMELUSINA_PROD_API AMelodiaPCGLevelKit : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGLevelKit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|PCG Kit")
	TObjectPtr<UPCGComponent> PCGComponent;

	/** Catalog graph to assign to this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit", meta = (DisplayPriority = 0))
	EMelodiaPCGGraphId GraphId = EMelodiaPCGGraphId::PortfolioTerraceBezier;

	/** Used when GraphId is Custom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit",
		meta = (EditCondition = "GraphId == EMelodiaPCGGraphId::Custom", EditConditionHides))
	FSoftObjectPath CustomGraphAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit", meta = (ClampMin = "0"))
	int32 GenerationSeed = 42;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit")
	bool bAutoGenerateOnBeginPlay = true;

	/** Hint for which layout preset to use on Bezier nodes inside the graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit")
	EMelodiaBezierLayoutPreset SuggestedLayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void AssignGraphFromCatalog();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void GenerateNow();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void SyncSuggestedPresetFromCatalog();

	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Kit")
	FSoftObjectPath GetResolvedGraphPath() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void ApplyGraphInternal();
};
