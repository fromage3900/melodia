// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaBezierPresets.h"
#include "MelodiaPCGBezierMath.h"

namespace MelodiaBezierPresets
{
	namespace
	{
		FMelodiaBezierAnchorPoint Anchor(const FVector& Pos, const FVector& Arrive, const FVector& Leave)
		{
			FMelodiaBezierAnchorPoint Point;
			Point.Position = Pos;
			Point.ArriveTangent = Arrive;
			Point.LeaveTangent = Leave;
			return Point;
		}

		FMelodiaBezierLayoutPresetData MakeCloisterRing()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.bClosedLoop = true;
			Data.ControlPoints = {
				Anchor(FVector(-1200, -1200, 0), FVector(-400, 0, 0), FVector(400, 0, 0)),
				Anchor(FVector(1200, -1200, 0), FVector(0, -400, 0), FVector(0, 400, 0)),
				Anchor(FVector(1200, 1200, 0), FVector(400, 0, 0), FVector(-400, 0, 0)),
				Anchor(FVector(-1200, 1200, 0), FVector(0, 400, 0), FVector(0, -400, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeColonnadeAvenue()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-3000, 0, 0), FVector(-800, 0, 0), FVector(1200, 0, 0)),
				Anchor(FVector(0, 0, 0), FVector(-600, 0, 0), FVector(600, 0, 0)),
				Anchor(FVector(3000, 0, 0), FVector(-1200, 0, 0), FVector(800, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeGardenPromenade()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-1800, -600, 0), FVector(-500, 200, 0), FVector(700, 300, 0)),
				Anchor(FVector(-400, 200, 20), FVector(-300, -100, 0), FVector(500, 200, 0)),
				Anchor(FVector(900, 600, 40), FVector(-400, 100, 0), FVector(600, -100, 0)),
				Anchor(FVector(2200, 300, 30), FVector(-700, 0, 0), FVector(500, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeBridgeSpan()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-2000, 0, 80), FVector(-600, 0, 0), FVector(800, 0, 40)),
				Anchor(FVector(0, 0, 160), FVector(-800, 0, 20), FVector(800, 0, -20)),
				Anchor(FVector(2000, 0, 80), FVector(-800, 0, -40), FVector(600, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeCathedralNaveAxis()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-4000, 0, 0), FVector(-1000, 0, 0), FVector(1500, 0, 0)),
				Anchor(FVector(0, 0, 0), FVector(-1500, 0, 0), FVector(1500, 0, 0)),
				Anchor(FVector(4000, 0, 0), FVector(-1500, 0, 0), FVector(1000, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeFloatingBalcony()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-1200, 400, 200), FVector(-400, 0, 0), FVector(500, -100, 0)),
				Anchor(FVector(0, 200, 240), FVector(-500, 50, 0), FVector(500, 50, 0)),
				Anchor(FVector(1200, 400, 200), FVector(-500, -100, 0), FVector(400, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakePenroseApproach()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-1600, -800, 0), FVector(-400, 300, 0), FVector(600, -200, 0)),
				Anchor(FVector(-200, 400, 40), FVector(-500, -200, 0), FVector(500, 200, 0)),
				Anchor(FVector(1200, -200, 60), FVector(-600, 300, 0), FVector(500, -300, 0)),
				Anchor(FVector(2400, 600, 80), FVector(-500, 0, 0), FVector(400, 0, 0)),
			};
			return Data;
		}

		FMelodiaBezierLayoutPresetData MakeEscherSwitchback()
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = {
				Anchor(FVector(-1500, -1500, 0), FVector(-400, 400, 0), FVector(500, 500, 80)),
				Anchor(FVector(-500, -500, 120), FVector(-300, 300, 40), FVector(400, 400, 80)),
				Anchor(FVector(500, 500, 240), FVector(-400, 400, 40), FVector(400, -400, -80)),
				Anchor(FVector(1500, 1500, 120), FVector(-500, -500, -80), FVector(400, -400, 0)),
			};
			return Data;
		}
	}

	FMelodiaBezierLayoutPresetData ResolveLayoutPreset(const EMelodiaBezierLayoutPreset Preset)
	{
		switch (Preset)
		{
		case EMelodiaBezierLayoutPreset::PortfolioTerrace:
		{
			FMelodiaBezierLayoutPresetData Data;
			Data.ControlPoints = MelodiaPCGBezierMath::MakePortfolioTerraceDefaults();
			return Data;
		}
		case EMelodiaBezierLayoutPreset::CloisterRing:
			return MakeCloisterRing();
		case EMelodiaBezierLayoutPreset::ColonnadeAvenue:
			return MakeColonnadeAvenue();
		case EMelodiaBezierLayoutPreset::GardenPromenade:
			return MakeGardenPromenade();
		case EMelodiaBezierLayoutPreset::BridgeSpan:
			return MakeBridgeSpan();
		case EMelodiaBezierLayoutPreset::CathedralNaveAxis:
			return MakeCathedralNaveAxis();
		case EMelodiaBezierLayoutPreset::FloatingBalcony:
			return MakeFloatingBalcony();
		case EMelodiaBezierLayoutPreset::PenroseApproach:
			return MakePenroseApproach();
		case EMelodiaBezierLayoutPreset::EscherSwitchback:
			return MakeEscherSwitchback();
		default:
			return FMelodiaBezierLayoutPresetData();
		}
	}

	FText GetLayoutPresetDisplayName(const EMelodiaBezierLayoutPreset Preset)
	{
		if (const UEnum* Enum = StaticEnum<EMelodiaBezierLayoutPreset>())
		{
			return Enum->GetDisplayNameTextByValue(static_cast<int64>(Preset));
		}
		return FText::GetEmpty();
	}

	FText GetLayoutPresetDescription(const EMelodiaBezierLayoutPreset Preset)
	{
		switch (Preset)
		{
		case EMelodiaBezierLayoutPreset::PortfolioTerrace:
			return INVTEXT("Rising S-curve with terrace landings — primary environment-art reel path.");
		case EMelodiaBezierLayoutPreset::CloisterRing:
			return INVTEXT("Closed courtyard loop; pair with Cloister or Colonnade nodes.");
		case EMelodiaBezierLayoutPreset::ColonnadeAvenue:
			return INVTEXT("Long straight-ish axis for column runs and nave shots.");
		case EMelodiaBezierLayoutPreset::GardenPromenade:
			return INVTEXT("Gentle organic walk with flower-scatter-friendly curves.");
		case EMelodiaBezierLayoutPreset::BridgeSpan:
			return INVTEXT("Arched elevation change — bridge deck + railing sweeps.");
		case EMelodiaBezierLayoutPreset::CathedralNaveAxis:
			return INVTEXT("Extended central axis for nave / vault compositions.");
		case EMelodiaBezierLayoutPreset::FloatingBalcony:
			return INVTEXT("Mid-air balcony path with railings at height.");
		case EMelodiaBezierLayoutPreset::PenroseApproach:
			return INVTEXT("Rhythmic S-waves for surreal garden approaches.");
		case EMelodiaBezierLayoutPreset::EscherSwitchback:
			return INVTEXT("Vertical switchback with rising terraces.");
		default:
			return INVTEXT("Hand-placed Bezier anchors.");
		}
	}

	EMelodiaBezierLayoutPreset GetSuggestedPresetForGraph(const EMelodiaPCGGraphId GraphId)
	{
		switch (GraphId)
		{
		case EMelodiaPCGGraphId::PortfolioTerraceBezier:
		case EMelodiaPCGGraphId::BezierVistaTerrace:
			return EMelodiaBezierLayoutPreset::PortfolioTerrace;
		case EMelodiaPCGGraphId::BezierCloisterRing:
			return EMelodiaBezierLayoutPreset::CloisterRing;
		case EMelodiaPCGGraphId::BezierColonnadeAvenue:
		case EMelodiaPCGGraphId::BezierCathedralAxis:
			return EMelodiaBezierLayoutPreset::ColonnadeAvenue;
		case EMelodiaPCGGraphId::BezierGardenPromenade:
		case EMelodiaPCGGraphId::BezierOrnamentGallery:
		case EMelodiaPCGGraphId::BezierSplineGarden:
			return EMelodiaBezierLayoutPreset::GardenPromenade;
		case EMelodiaPCGGraphId::BezierBridgeSpan:
			return EMelodiaBezierLayoutPreset::BridgeSpan;
		case EMelodiaPCGGraphId::BezierPathPortfolio:
			return EMelodiaBezierLayoutPreset::GardenPromenade;
		default:
			return EMelodiaBezierLayoutPreset::Custom;
		}
	}
}
