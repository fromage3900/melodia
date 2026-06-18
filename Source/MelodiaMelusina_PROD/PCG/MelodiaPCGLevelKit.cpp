// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGLevelKit.h"
#include "MelodiaPCGGraphRegistry.h"
#include "MelodiaBezierPresets.h"
#include "MelodiaPCGLibrary.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Components/BrushComponent.h"

namespace MelodiaPCGLevelKitPrivate
{
	/** Default AVolume builder brush span before scaling (uu). */
	constexpr float BaseBrushSpanUU = 200.f;
}

AMelodiaPCGLevelKit::AMelodiaPCGLevelKit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	if (PCGComponent)
	{
		PCGComponent->bActivated = true;
		PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnLoad;
	}

	ApplyGenerationBounds();
}

void AMelodiaPCGLevelKit::ApplyGenerationBounds()
{
	using namespace MelodiaPCGLevelKitPrivate;

	if (UBrushComponent* BrushComp = GetBrushComponent())
	{
		const FVector FullExtentCm = GenerationBoundsHalfExtent * 2.f;
		const FVector Scale(
			FullExtentCm.X / BaseBrushSpanUU,
			FullExtentCm.Y / BaseBrushSpanUU,
			FullExtentCm.Z / BaseBrushSpanUU);
		BrushComp->SetRelativeScale3D(Scale);
	}
}

FSoftObjectPath AMelodiaPCGLevelKit::GetResolvedGraphPath() const
{
	if (GraphId == EMelodiaPCGGraphId::Custom)
	{
		return CustomGraphAsset;
	}
	return UMelodiaPCGGraphRegistry::GetGraphAssetPath(GraphId);
}

void AMelodiaPCGLevelKit::SyncSuggestedPresetFromCatalog()
{
	FMelodiaPCGGraphCatalogEntry Entry;
	if (UMelodiaPCGGraphRegistry::ResolveGraphEntry(GraphId, Entry))
	{
		SuggestedLayoutPreset = Entry.SuggestedLayoutPreset;
		GenerationSeed = Entry.DefaultSeed;
	}
}

void AMelodiaPCGLevelKit::AssignGraphFromCatalog()
{
	SyncSuggestedPresetFromCatalog();
	ApplyGraphInternal();
}

void AMelodiaPCGLevelKit::ApplyGraphInternal()
{
	if (!PCGComponent)
	{
		return;
	}

	const FSoftObjectPath GraphPath = GetResolvedGraphPath();
	if (GraphPath.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("MelodiaPCGLevelKit: no graph path resolved for GraphId."));
		return;
	}

	UMelodiaPCGLibrary::AssignGraphToComponent(PCGComponent, GraphPath, GenerationSeed);
}

void AMelodiaPCGLevelKit::GenerateNow()
{
	ApplyGenerationBounds();
	AssignGraphFromCatalog();
	UMelodiaPCGLibrary::GeneratePCGComponent(PCGComponent, true, 60.f);
	UE_LOG(LogTemp, Log, TEXT("MelodiaPCGLevelKit '%s': graph=%s ISM=%d — fly camera to actor origin if viewport looks empty."),
		*GetActorLabel(), *GetResolvedGraphPath().ToString(), UMelodiaPCGLibrary::CountInstancedMeshInstances(this));
}

void AMelodiaPCGLevelKit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyGenerationBounds();
	AssignGraphFromCatalog();

#if WITH_EDITOR
	if (PCGComponent && GetWorld() && !GetWorld()->IsGameWorld())
	{
		UMelodiaPCGLibrary::GeneratePCGComponent(PCGComponent, true);
	}
#endif
}

void AMelodiaPCGLevelKit::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGenerateOnBeginPlay)
	{
		GenerateNow();
	}
}
