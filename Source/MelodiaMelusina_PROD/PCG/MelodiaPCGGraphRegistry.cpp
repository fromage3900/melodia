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
	Catalog.Reserve(16);

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::DreamWalls,
		TEXT("Dream Walls"),
		TEXT("[portfolio-ready] Escher hero — SM_wallhi 400cm grid, 3 orthogonal layers, scale 1.0."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_DreamWalls.PCG_DreamWalls"),
		TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace"),
		EMelodiaBezierLayoutPreset::EscherSwitchback, 42));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::PortfolioTerraceBezier,
		TEXT("Portfolio Terrace Bezier"),
		TEXT("[portfolio-ready] Hero terrace walk: path + tile landings + railings. Placeholder meshes only."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PortfolioTerraceBezier.PCG_PortfolioTerraceBezier"),
		TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace"),
		EMelodiaBezierLayoutPreset::PortfolioTerrace, 42));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierPathPortfolio,
		TEXT("Bezier Path + Sweep"),
		TEXT("[portfolio-ready] Arc-length path with railing sweep — 400cm module spacing."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierPathPortfolio.PCG_BezierPathPortfolio"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierPath"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 7));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierCloisterRing,
		TEXT("Bezier Cloister Ring"),
		TEXT("[portfolio-ready] Closed courtyard loop with inward terraces and corner columns."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierCloisterRing.PCG_BezierCloisterRing"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierCloister"),
		EMelodiaBezierLayoutPreset::CloisterRing, 11));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierColonnadeAvenue,
		TEXT("Bezier Colonnade Avenue"),
		TEXT("[portfolio-ready] Dual column rows along a grand axis at 400cm spacing."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierColonnadeAvenue.PCG_BezierColonnadeAvenue"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierColonnade"),
		EMelodiaBezierLayoutPreset::ColonnadeAvenue, 3));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierGardenPromenade,
		TEXT("Bezier Garden Promenade"),
		TEXT("[portfolio-ready] Organic walk path with railings; ornament jitter off in placeholder mode."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierGardenPromenade.PCG_BezierGardenPromenade"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierGarden"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 19));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierBridgeSpan,
		TEXT("Bezier Bridge Span"),
		TEXT("[portfolio-ready] Arched deck with balustrade sweeps — elevated crossing."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierBridgeSpan.PCG_BezierBridgeSpan"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierBridge"),
		EMelodiaBezierLayoutPreset::BridgeSpan, 5));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierCathedralAxis,
		TEXT("Bezier Cathedral Axis"),
		TEXT("[portfolio-ready] Long nave axis with colonnade rows and cornice sweep."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierCathedralAxis.PCG_BezierCathedralAxis"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierNave"),
		EMelodiaBezierLayoutPreset::CathedralNaveAxis, 13));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierVistaTerrace,
		TEXT("Bezier Vista Terrace"),
		TEXT("[portfolio-ready] Fewer, wider terrace landings for hero vista shots."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BezierVistaTerrace.PCG_BezierVistaTerrace"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_BezierVista"),
		EMelodiaBezierLayoutPreset::PortfolioTerrace, 42));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BezierOrnamentGallery,
		TEXT("Bezier Ornament Gallery"),
		TEXT("[portfolio-ready] Ornament gallery path; scatter jitter disabled in placeholder mode."),
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
		EMelodiaPCGGraphId::PortfolioEnvironment,
		TEXT("Portfolio Environment Scatter"),
		TEXT("Terrain-aware rock and ground-cover scatter around portfolio paths."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PortfolioEnvironment.PCG_PortfolioEnvironment"),
		TEXT("/Game/Melodia/Levels/L_MelodiaPortfolioTerrace"),
		EMelodiaBezierLayoutPreset::GardenPromenade, 77));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BaroqueColonnadeEx,
		TEXT("Baroque Colonnade (PCGEx)"),
		TEXT("PCGEx grid colonnade with path resample and column spawn."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCGEx/PCG_BaroqueColonnadeEx.PCG_BaroqueColonnadeEx"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_ColonnadeEx"),
		EMelodiaBezierLayoutPreset::ColonnadeAvenue, 5));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::GothicCorridorEx,
		TEXT("Gothic Corridor (PCGEx)"),
		TEXT("Spline corridor with wall modules on both sides."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCGEx/PCG_GothicCorridorEx.PCG_GothicCorridorEx"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_GothicCorridors"),
		EMelodiaBezierLayoutPreset::Custom, 13));

	Catalog.Add(Entry(
		EMelodiaPCGGraphId::BaroqueAtriumEx,
		TEXT("Baroque Atrium (PCGEx)"),
		TEXT("Rectangular courtyard perimeter columns and inner walls."),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCGEx/PCG_BaroqueAtriumEx.PCG_BaroqueAtriumEx"),
		TEXT("/Game/_PROJECT/PCG/TestLevels/L_PCGTest_AtriumEx"),
		EMelodiaBezierLayoutPreset::CloisterRing, 19));

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

FString UMelodiaPCGGraphRegistry::GetPCGExBuildScriptPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_pcgex_builder.py");
}

FString UMelodiaPCGGraphRegistry::GetPCGExCollectionsScriptPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_pcgex_collections.py");
}

FString UMelodiaPCGGraphRegistry::GetDreamWallsBuildScriptPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_dreamwalls_builder.py");
}

FString UMelodiaPCGGraphRegistry::GetPortfolioManifestPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_pcg_library_manifest.json");
}

FString UMelodiaPCGGraphRegistry::GetSimplePCGBuildScriptPath()
{
	return TEXT("G:/Melodia/Scripts/PCG/melodia_pcg_simple_builder.py");
}
