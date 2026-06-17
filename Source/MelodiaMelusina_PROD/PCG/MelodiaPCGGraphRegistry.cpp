// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGGraphRegistry.h"
#include "MelodiaBezierPresets.h"

namespace MelodiaPCGGraphRegistryPrivate
{
	static FMelodiaPCGGraphCatalogEntry Entry(
		EMelodiaPCGGraphId GraphId,
		const TCHAR* DisplayName,
		const TCHAR* Description,
		const TCHAR* GraphPath,
		const TCHAR* TestLevelPath,
		EMelodiaBezierLayoutPreset Preset,
		int32 Seed)
	{
		FMelodiaPCGGraphCatalogEntry Item;
		Item.GraphId = GraphId;
		Item.DisplayName = FText::FromString(DisplayName);
		Item.Description = FText::FromString(Description);
		Item.GraphAsset = FSoftObjectPath(GraphPath);
		Item.SuggestedTestLevel = FSoftObjectPath(TestLevelPath);
		Item.SuggestedLayoutPreset = Preset;
		Item.DefaultSeed = Seed;
		return Item;
	}

	static const TCHAR* GraphRoot = TEXT("/Game/_PROJECT/PCG/Graphs/");
	static const TCHAR* TestRoot = TEXT("/Game/_PROJECT/PCG/TestLevels/");
}

TArray<FMelodiaPCGGraphCatalogEntry> UMelodiaPCGGraphRegistry::GetGraphCatalog()
{
	using namespace MelodiaPCGGraphRegistryPrivate;

	TArray<FMelodiaPCGGraphCatalogEntry> Catalog;
	Catalog.Reserve(12);

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::PortfolioTerraceBezier,
		TEXT("Portfolio Terrace Bezier"),
		TEXT("Hero terrace walk: path + tile landings + railings. Best for environment reel."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PortfolioTerraceBezier.PCG_PortfolioTerraceBezier"),
		TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace"),
		EMelodiaBezierLayoutPreset::PortfolioTerrace, 42));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierPathPortfolio,
		TEXT("Bezier Path + Sweep"),
		TEXT("Arc-length path with railing sweep — quick path/railing iteration."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierPathPortfolio.PCG_BezierPathPortfolio"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierPath"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 7));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierCloisterRing,
		TEXT("Bezier Cloister Ring"),
		TEXT("Closed courtyard loop with inward terraces and corner columns."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierCloisterRing.PCG_BezierCloisterRing"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierCloister"),
		EMelodiaBezierLayoutPreset::CloisterRing, 11));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierColonnadeAvenue,
		TEXT("Bezier Colonnade Avenue"),
		TEXT("Dual column rows along a grand axis."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierColonnadeAvenue.PCG_BezierColonnadeAvenue"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierColonnade"),
		EMelodiaBezierLayoutPreset::ColonnadeAvenue, 3));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierGardenPromenade,
		TEXT("Bezier Garden Promenade"),
		TEXT("Organic walk path with ornament scatter and low railings."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierGardenPromenade.PCG_BezierGardenPromenade"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierGarden"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 19));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierBridgeSpan,
		TEXT("Bezier Bridge Span"),
		TEXT("Arched deck with balustrade sweeps — elevated crossing."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierBridgeSpan.PCG_BezierBridgeSpan"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierBridge"),
		EMelodiaBezierLayoutPreset::BridgeSpan, 5));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierCathedralAxis,
		TEXT("Bezier Cathedral Axis"),
		TEXT("Long nave axis with colonnade rows and cornice sweep."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierCathedralAxis.PCG_BezierCathedralAxis"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierNave"),
		EMelodiaBezierLayoutPreset::CathedralNaveAxis, 13));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierVistaTerrace,
		TEXT("Bezier Vista Terrace"),
		TEXT("Fewer, wider terrace landings for hero vista shots."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierVistaTerrace.PCG_BezierVistaTerrace"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierVista"),
		EMelodiaBezierLayoutPreset::PortfolioTerrace, 42));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierOrnamentGallery,
		TEXT("Bezier Ornament Gallery"),
		TEXT("Dense ornament scatter along a gallery path."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierOrnamentGallery.PCG_BezierOrnamentGallery"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierOrnaments"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 23));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierSplineGarden,
		TEXT("Bezier Spline Garden"),
		TEXT("Sample a level spline by tag — art-direct path without recompiling."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierSplineGarden.PCG_BezierSplineGarden"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_SplinePath"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 0));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::TerraceGarden,
		TEXT("Terrace Garden (classic)"),
		TEXT("Original Melodia terrace garden PCG graph."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden.PCG_TerraceGarden"),
		TEXT("/Game/Melodia/Levels/L_MelodiaPCGDemo"),
		EMelodiaBezierLayoutPreset::Custom, 0));

	return Catalog;
}

bool UMelodiaPCGGraphRegistry::ResolveGraphEntry(const EMelodiaPCGGraphId GraphId, FMelodiaPCGGraphCatalogEntry& OutEntry)
{
	for (const FMelodiaPCGGraphCatalogEntry& Entry : GetGraphCatalog())
	{
		if (Entry.GraphId == GraphId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

FSoftObjectPath UMelodiaPCGGraphRegistry::GetGraphAssetPath(const EMelodiaPCGGraphId GraphId)
{
	FMelodiaPCGGraphCatalogEntry Entry;
	return ResolveGraphEntry(GraphId, Entry) ? Entry.GraphAsset : FSoftObjectPath();
}

FString UMelodiaPCGGraphRegistry::GetGraphBuildScriptPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_pcg_bezier_builder.py");
}
