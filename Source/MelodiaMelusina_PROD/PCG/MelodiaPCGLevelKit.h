// Copyright Melodia Project. All Rights Reserved.
// Drop-in PCG volume actor for fast level building — pick a graph, set seed, generate.

#pragma once

#include "CoreMinimal.h"
#include "PCGVolume.h"
#include "MelodiaBezierTypes.h"
#include "MelodiaPCGLevelKit.generated.h"

class UPCGGraph;

UCLASS(Blueprintable, meta = (DisplayName = "Melodia PCG Level Kit"))
class MELODIAMELUSINA_PROD_API AMelodiaPCGLevelKit : public APCGVolume
{
	GENERATED_BODY()

public:
	AMelodiaPCGLevelKit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Catalog graph to assign to this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit", meta = (DisplayPriority = 0))
	EMelodiaPCGGraphId GraphId = EMelodiaPCGGraphId::DreamWalls;

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
	EMelodiaBezierLayoutPreset SuggestedLayoutPreset = EMelodiaBezierLayoutPreset::EscherSwitchback;

	/** Half-extent of the PCG generation volume in cm (full span = 2x). Portfolio graphs use ~±3000 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Kit", meta = (ClampMin = "100"))
	FVector GenerationBoundsHalfExtent = FVector(3500.f, 3500.f, 1200.f);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void AssignGraphFromCatalog();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void GenerateNow();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void SyncSuggestedPresetFromCatalog();

	/** Resize the volume brush to match GenerationBoundsHalfExtent. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Melodia|PCG Kit")
	void ApplyGenerationBounds();

	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Kit")
	FSoftObjectPath GetResolvedGraphPath() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void ApplyGraphInternal();
};
