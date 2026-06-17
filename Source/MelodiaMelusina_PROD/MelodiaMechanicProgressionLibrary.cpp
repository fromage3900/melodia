// Deterministic mechanic level tables, XP curve, and demo location presets (Lv 1–30).

#include "MelodiaMechanicProgressionLibrary.h"

namespace
{
FMelodiaLocationLevelPreset MakePreset(
	const FName PresetId,
	const int32 Level,
	const EMelodiaMechanicTier Tier,
	const TCHAR* DisplayName,
	const TCHAR* BuildNotes,
	const TCHAR* GraphPath,
	const int32 MinEnc,
	const int32 MaxEnc,
	const float Difficulty,
	const EPCGArchitecturalRole Role,
	const bool bStartUnlocked)
{
	FMelodiaLocationLevelPreset Preset;
	Preset.PresetId = PresetId;
	Preset.MechanicLevelRequired = Level;
	Preset.MechanicTier = Tier;
	Preset.DisplayName = DisplayName;
	Preset.BuildNotes = BuildNotes;
	if (GraphPath && GraphPath[0] != 0)
	{
		Preset.PCGGraphAsset = FSoftObjectPath(GraphPath);
	}
	Preset.MinEncounters = MinEnc;
	Preset.MaxEncounters = MaxEnc;
	Preset.DifficultyMultiplier = Difficulty;
	Preset.QuestMarkerRole = Role;
	Preset.bUnlockedAtStart = bStartUnlocked;
	return Preset;
}

EMelodiaMechanicTier TierFromLevel(const int32 Level)
{
	return UMelodiaMechanicProgressionLibrary::GetTierForMechanicLevel(Level);
}
}

int32 UMelodiaMechanicProgressionLibrary::GetXPRequiredForNextLevel(const int32 CurrentLevel)
{
	const int32 Clamped = FMath::Clamp(CurrentLevel, 1, DemoMaxMechanicLevel);
	if (Clamped >= DemoMaxMechanicLevel)
	{
		return 0;
	}
	return 60 + Clamped * 25;
}

EMelodiaMechanicTier UMelodiaMechanicProgressionLibrary::GetTierForMechanicLevel(const int32 MechanicLevel)
{
	const int32 Clamped = FMath::Clamp(MechanicLevel, 1, DemoMaxMechanicLevel);
	const int32 TierIndex = (Clamped - 1) / 5;
	switch (TierIndex)
	{
	case 0: return EMelodiaMechanicTier::TierI_NoviceStar;
	case 1: return EMelodiaMechanicTier::TierII_MoonApprentice;
	case 2: return EMelodiaMechanicTier::TierIII_CometAdept;
	case 3: return EMelodiaMechanicTier::TierIV_AuroraVirtuoso;
	case 4: return EMelodiaMechanicTier::TierV_NebulaMaestro;
	default: return EMelodiaMechanicTier::TierVI_CelestialLegend;
	}
}

FString UMelodiaMechanicProgressionLibrary::GetTierDisplayName(const EMelodiaMechanicTier Tier)
{
	switch (Tier)
	{
	case EMelodiaMechanicTier::TierI_NoviceStar: return TEXT("Novice Star");
	case EMelodiaMechanicTier::TierII_MoonApprentice: return TEXT("Moon Apprentice");
	case EMelodiaMechanicTier::TierIII_CometAdept: return TEXT("Comet Adept");
	case EMelodiaMechanicTier::TierIV_AuroraVirtuoso: return TEXT("Aurora Virtuoso");
	case EMelodiaMechanicTier::TierV_NebulaMaestro: return TEXT("Nebula Maestro");
	case EMelodiaMechanicTier::TierVI_CelestialLegend:
	default: return TEXT("Celestial Legend");
	}
}

TArray<FMelodiaMechanicTierDefinition> UMelodiaMechanicProgressionLibrary::BuildDefaultTierDefinitions()
{
	TArray<FMelodiaMechanicTierDefinition> Tiers;
	const EMelodiaMechanicTier TierValues[] = {
		EMelodiaMechanicTier::TierI_NoviceStar,
		EMelodiaMechanicTier::TierII_MoonApprentice,
		EMelodiaMechanicTier::TierIII_CometAdept,
		EMelodiaMechanicTier::TierIV_AuroraVirtuoso,
		EMelodiaMechanicTier::TierV_NebulaMaestro,
		EMelodiaMechanicTier::TierVI_CelestialLegend
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TierValues); ++Index)
	{
		FMelodiaMechanicTierDefinition Def;
		Def.Tier = TierValues[Index];
		Def.MinMechanicLevel = Index * 5 + 1;
		Def.MaxMechanicLevel = FMath::Min((Index + 1) * 5, DemoMaxMechanicLevel);
		Def.TierDisplayName = GetTierDisplayName(TierValues[Index]);
		Def.UnlockBlurb = FString::Printf(
			TEXT("Levels %d–%d: new Reverie presets, songwriting skills, and harmonic keys unlock."),
			Def.MinMechanicLevel, Def.MaxMechanicLevel);
		Tiers.Add(Def);
	}
	return Tiers;
}

TArray<FMelodiaLocationLevelPreset> UMelodiaMechanicProgressionLibrary::BuildDemoLocationPresets()
{
	TArray<FMelodiaLocationLevelPreset> Presets;
	Presets.Reserve(DemoMaxMechanicLevel);

	const TCHAR* GraphPaths[] = {
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden.PCG_TerraceGarden"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_WallGardenPath.PCG_WallGardenPath"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_MelodiaForest.PCG_MelodiaForest"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_MeadowFalloff.PCG_MeadowFalloff"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BaroqueColonnade.PCG_BaroqueColonnade"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_FloatingStairways.PCG_FloatingStairways"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_DreamWalls.PCG_DreamWalls"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PenroseShrine.PCG_PenroseShrine"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_EscherDecks.PCG_EscherDecks"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BridgeArchipelago.PCG_BridgeArchipelago"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_Cloister.PCG_Cloister"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_CathedralNave.PCG_CathedralNave"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_OvergrownRuins.PCG_OvergrownRuins"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BridgeArchipelago.PCG_BridgeArchipelago"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden.PCG_TerraceGarden"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BaroqueColonnade.PCG_BaroqueColonnade"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_CathedralNave.PCG_CathedralNave"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_DreamWalls.PCG_DreamWalls"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PenroseShrine.PCG_PenroseShrine"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_Cloister.PCG_Cloister"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_BaroqueRuins.PCG_BaroqueRuins"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_SplinePath.PCG_SplinePath"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_GreyboxBlockout.PCG_GreyboxBlockout"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_FloatingStairways.PCG_FloatingStairways"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_Cloister.PCG_Cloister"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_EscherDecks.PCG_EscherDecks"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PenroseShrine.PCG_PenroseShrine"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_CathedralNave.PCG_CathedralNave"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_CathedralNave.PCG_CathedralNave"),
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PenroseShrine.PCG_PenroseShrine"),
	};

	struct FPresetRow
	{
		FName Id;
		const TCHAR* Name;
		const TCHAR* Notes;
	};

	const FPresetRow Rows[DemoMaxMechanicLevel] = {
		{ TEXT("Lv01_StarlitAtrium"), TEXT("星灯 Atrium"), TEXT("Tier I starter — terrace garden blockout.") },
		{ TEXT("Lv02_MoonCourtyard"), TEXT("月庭 Moon Courtyard"), TEXT("Wall garden path preset.") },
		{ TEXT("Lv03_CherryGate"), TEXT("樱门 Cherry Gate"), TEXT("Forest scatter + song gate markers.") },
		{ TEXT("Lv04_WhisperPool"), TEXT("细语池 Whisper Pool"), TEXT("Meadow falloff + rest point role.") },
		{ TEXT("Lv05_GoldStuccoAlley"), TEXT("金粉巷 Gold Stucco Alley"), TEXT("Colonnade rhythm corridor.") },
		{ TEXT("Lv06_EscherEntry"), TEXT("埃舍尔阶梯 Escher Entry"), TEXT("Tier II — floating stair custom PCG nodes.") },
		{ TEXT("Lv07_GravityTerrace"), TEXT("重力露台 Gravity Terrace"), TEXT("Gravity Zone attribute writer.") },
		{ TEXT("Lv08_TessellationPlaza"), TEXT("镶嵌广场 Tessellation Plaza"), TEXT("Tessellation custom element.") },
		{ TEXT("Lv09_RecursiveArch"), TEXT("递归拱门 Recursive Arch"), TEXT("Recursive Arch custom element.") },
		{ TEXT("Lv10_SpiralStarTower"), TEXT("螺旋星塔 Spiral Star Tower"), TEXT("Bridge archipelago vertical.") },
		{ TEXT("Lv11_BaroqueAtrium"), TEXT("巴洛克中庭 Baroque Atrium"), TEXT("Tier III cloister courtyard.") },
		{ TEXT("Lv12_CathedralNave"), TEXT("大教堂 Nave"), TEXT("Long nave + encounter spawns.") },
		{ TEXT("Lv13_VineCloister"), TEXT("藤蔓回廊 Vine Cloister"), TEXT("Overgrown ruins dressing.") },
		{ TEXT("Lv14_FloatingStarBridge"), TEXT("浮岛星桥 Floating Star Bridge"), TEXT("Archipelago connectors.") },
		{ TEXT("Lv15_MoonConservatory"), TEXT("月光温室 Moon Conservatory"), TEXT("Terrace + quest Door role.") },
		{ TEXT("Lv16_AuroraColonnade"), TEXT("极光柱廊 Aurora Colonnade"), TEXT("Tier IV colonnade variant.") },
		{ TEXT("Lv17_CometChapel"), TEXT("彗星礼拜堂 Comet Chapel"), TEXT("Nave + break-gauge tutorial.") },
		{ TEXT("Lv18_NebulaMaze"), TEXT("星云迷宫 Nebula Maze"), TEXT("Dream walls impossible geometry.") },
		{ TEXT("Lv19_ConstellationGrid"), TEXT("星座棋盘 Constellation Grid"), TEXT("Penrose shrine lattice.") },
		{ TEXT("Lv20_HarmonicDome"), TEXT("谐波穹顶 Harmonic Dome"), TEXT("Cloister radial encounters.") },
		{ TEXT("Lv21_DragonscaleCourt"), TEXT("龙鳞庭 Dragonscale Court"), TEXT("Tier V ruined baroque.") },
		{ TEXT("Lv22_PianoCascade"), TEXT("琴键瀑布 Piano Cascade"), TEXT("Spline path melody line.") },
		{ TEXT("Lv23_MusicBoxAtelier"), TEXT("八音盒工坊 Music Box Atelier"), TEXT("Greybox for skill authoring.") },
		{ TEXT("Lv24_ImaginaryStairs"), TEXT("虚数阶梯 Imaginary Stairs"), TEXT("Escher staircase stress test.") },
		{ TEXT("Lv25_GalaxyCloister"), TEXT("银河回廊 Galaxy Cloister"), TEXT("High-density cloister run.") },
		{ TEXT("Lv26_CelestialObservatory"), TEXT("天体台 Celestial Observatory"), TEXT("Tier VI — deck platforms.") },
		{ TEXT("Lv27_FinalStarGate"), TEXT("终焉星门 Final Star Gate"), TEXT("Penrose gate + portal hook.") },
		{ TEXT("Lv28_DreamCathedral"), TEXT("幻梦大教堂 Dream Cathedral"), TEXT("Full nave boss room.") },
		{ TEXT("Lv29_HallOfHarmonics"), TEXT("万象谐波 Hall of Harmonics"), TEXT("Multi-encounter cathedral.") },
		{ TEXT("Lv30_MaestroSanctum"), TEXT("天音圣域 Maestro Sanctum"), TEXT("Demo cap — assign ultimate PCG graph.") },
	};

	for (int32 Level = 1; Level <= DemoMaxMechanicLevel; ++Level)
	{
		const FPresetRow& Row = Rows[Level - 1];
		const int32 GraphIndex = Level - 1;
		const float Difficulty = 0.85f + static_cast<float>(Level) * 0.05f;
		const int32 MaxEnc = FMath::Clamp(1 + Level / 10, 1, 4);
		Presets.Add(MakePreset(
			Row.Id,
			Level,
			TierFromLevel(Level),
			Row.Name,
			Row.Notes,
			GraphPaths[GraphIndex],
			1,
			MaxEnc,
			Difficulty,
			Level % 3 == 0 ? EPCGArchitecturalRole::Door : EPCGArchitecturalRole::Floor,
			Level == 1));
	}
	return Presets;
}

bool UMelodiaMechanicProgressionLibrary::FindLocationPreset(const FName PresetId, FMelodiaLocationLevelPreset& OutPreset)
{
	for (const FMelodiaLocationLevelPreset& Preset : BuildDemoLocationPresets())
	{
		if (Preset.PresetId == PresetId)
		{
			OutPreset = Preset;
			return true;
		}
	}
	return false;
}

FReverieAreaConfig UMelodiaMechanicProgressionLibrary::ToReverieAreaConfig(const FMelodiaLocationLevelPreset& Preset)
{
	FReverieAreaConfig Area;
	Area.PCGGraphAsset = Preset.PCGGraphAsset;
	Area.MinEncounters = Preset.MinEncounters;
	Area.MaxEncounters = Preset.MaxEncounters;
	Area.DifficultyMultiplier = Preset.DifficultyMultiplier;
	Area.AreaDisplayName = Preset.DisplayName;
	return Area;
}

bool UMelodiaMechanicProgressionLibrary::GrantMechanicXP(FMelodiaMechanicProgressionState& State, const int32 Amount, FString& OutLevelUpSummary)
{
	if (Amount <= 0 || State.MechanicLevel >= DemoMaxMechanicLevel)
	{
		return false;
	}

	State.MechanicXP += Amount;
	bool bLeveled = false;
	OutLevelUpSummary.Reset();

	while (State.MechanicLevel < DemoMaxMechanicLevel)
	{
		const int32 Required = GetXPRequiredForNextLevel(State.MechanicLevel);
		if (Required <= 0 || State.MechanicXP < Required)
		{
			break;
		}

		State.MechanicXP -= Required;
		++State.MechanicLevel;
		++State.TotalLevelUps;
		bLeveled = true;

		const FName PresetId = GetPresetIdForMechanicLevel(State.MechanicLevel);
		EnsurePresetUnlocked(State, PresetId);

		FMelodiaLocationLevelPreset Preset;
		if (FindLocationPreset(PresetId, Preset))
		{
			OutLevelUpSummary = FString::Printf(
				TEXT("Mechanic Lv %d | %s unlocked: %s"),
				State.MechanicLevel,
				*GetTierDisplayName(Preset.MechanicTier),
				*Preset.DisplayName);
		}
		else
		{
			OutLevelUpSummary = FString::Printf(TEXT("Mechanic Lv %d"), State.MechanicLevel);
		}
	}

	return bLeveled;
}

TArray<FMelodiaLocationLevelPreset> UMelodiaMechanicProgressionLibrary::GetUnlockedPresetsForState(const FMelodiaMechanicProgressionState& State)
{
	TArray<FMelodiaLocationLevelPreset> Unlocked;
	for (const FMelodiaLocationLevelPreset& Preset : BuildDemoLocationPresets())
	{
		const bool bUnlocked = Preset.bUnlockedAtStart
			|| Preset.MechanicLevelRequired <= State.MechanicLevel
			|| State.UnlockedPresetIds.Contains(Preset.PresetId);
		if (bUnlocked)
		{
			Unlocked.Add(Preset);
		}
	}
	return Unlocked;
}

void UMelodiaMechanicProgressionLibrary::EnsurePresetUnlocked(FMelodiaMechanicProgressionState& State, const FName PresetId)
{
	if (PresetId.IsNone())
	{
		return;
	}
	State.UnlockedPresetIds.AddUnique(PresetId);
}

FName UMelodiaMechanicProgressionLibrary::GetPresetIdForMechanicLevel(const int32 MechanicLevel)
{
	const int32 Clamped = FMath::Clamp(MechanicLevel, 1, DemoMaxMechanicLevel);
	for (const FMelodiaLocationLevelPreset& Preset : BuildDemoLocationPresets())
	{
		if (Preset.MechanicLevelRequired == Clamped)
		{
			return Preset.PresetId;
		}
	}
	return NAME_None;
}
