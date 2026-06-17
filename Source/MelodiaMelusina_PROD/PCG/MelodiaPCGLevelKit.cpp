// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGLevelKit.h"
#include "MelodiaPCGGraphRegistry.h"
#include "MelodiaBezierPresets.h"
#include "MelodiaPCGLibrary.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Components/SceneComponent.h"

AMelodiaPCGLevelKit::AMelodiaPCGLevelKit()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
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
	AssignGraphFromCatalog();
	UMelodiaPCGLibrary::GeneratePCGComponent(PCGComponent);
}

void AMelodiaPCGLevelKit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AssignGraphFromCatalog();
}

void AMelodiaPCGLevelKit::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGenerateOnBeginPlay)
	{
		GenerateNow();
	}
}
