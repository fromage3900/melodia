// Native base for rhythm HUD widgets so Blueprint callers have stable hooks.

#include "MelodiaRhythmHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

namespace
{
FSlateBrush MakeTintBrush(const FLinearColor& Tint)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

FVector2f V2f(const float X, const float Y)
{
	return FVector2f(X, Y);
}

FPaintGeometry PaintBox(const FGeometry& Geometry, const FVector2f Position, const FVector2f Size)
{
	return Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position));
}

void PaintLabel(FSlateWindowElementList& OutDrawElements, const FGeometry& Geometry, const int32 LayerId, const FVector2f Position, const FString& Text, const int32 FontSize, const FLinearColor& Tint)
{
	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId,
		PaintBox(Geometry, Position, V2f(260.0f, static_cast<float>(FontSize + 8))),
		FText::FromString(Text),
		FontInfo,
		ESlateDrawEffect::None,
		Tint);
}

FString InitialForName(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return TEXT("?");
	}
	return Name.Left(1).ToUpper();
}
}

void UMelodiaRhythmHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SyncMutablePaintStats();
}

int32 UMelodiaRhythmHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bDrawNativeCuteCombatHUD || !bCuteCombatThemeApplied)
	{
		SyncMutablePaintStats();
		return CurrentLayer;
	}

	++MutableNativePaintFrameCount;

	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X < 320.0f || LocalSize.Y < 180.0f)
	{
		SyncMutablePaintStats();
		return CurrentLayer;
	}

	++CurrentLayer;
	const float PanelWidth = FMath::Clamp(LocalSize.X * 0.46f, 420.0f, 760.0f);
	const float PanelHeight = 126.0f;
	const FVector2f PanelPosition(28.0f, LocalSize.Y - PanelHeight - 26.0f);
	const FVector2f PanelSize(PanelWidth, PanelHeight);

	const FSlateBrush PanelBrush = MakeTintBrush(PanelTint);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer,
		PaintBox(AllottedGeometry, PanelPosition, PanelSize),
		&PanelBrush,
		ESlateDrawEffect::None,
		PanelTint);

	++CurrentLayer;
	const FLinearColor OuterFiligree = FiligreeTint;
	TArray<FVector2f> FramePoints;
	FramePoints.Add(PanelPosition + V2f(18.0f, 8.0f));
	FramePoints.Add(PanelPosition + V2f(PanelSize.X - 18.0f, 8.0f));
	FramePoints.Add(PanelPosition + V2f(PanelSize.X - 8.0f, 18.0f));
	FramePoints.Add(PanelPosition + V2f(PanelSize.X - 8.0f, PanelSize.Y - 18.0f));
	FramePoints.Add(PanelPosition + V2f(PanelSize.X - 18.0f, PanelSize.Y - 8.0f));
	FramePoints.Add(PanelPosition + V2f(18.0f, PanelSize.Y - 8.0f));
	FramePoints.Add(PanelPosition + V2f(8.0f, PanelSize.Y - 18.0f));
	FramePoints.Add(PanelPosition + V2f(8.0f, 18.0f));
	FramePoints.Add(PanelPosition + V2f(18.0f, 8.0f));
	FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer, AllottedGeometry.ToPaintGeometry(), FramePoints, ESlateDrawEffect::None, OuterFiligree, true, 2.0f);

	const float Time = GetHUDTimeSeconds();
	const float Curl = FMath::Sin(Time * 2.4f) * 6.0f;
	for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
	{
		const bool bRight = CornerIndex == 1 || CornerIndex == 2;
		const bool bBottom = CornerIndex >= 2;
		const FVector2f Corner = PanelPosition + V2f(bRight ? PanelSize.X - 30.0f : 30.0f, bBottom ? PanelSize.Y - 30.0f : 30.0f);
		const float XSign = bRight ? -1.0f : 1.0f;
		const float YSign = bBottom ? -1.0f : 1.0f;
		TArray<FVector2f> CurlPoints;
		CurlPoints.Add(Corner);
		CurlPoints.Add(Corner + V2f(XSign * (18.0f + Curl), YSign * 4.0f));
		CurlPoints.Add(Corner + V2f(XSign * 34.0f, YSign * (18.0f - Curl * 0.4f)));
		CurlPoints.Add(Corner + V2f(XSign * 18.0f, YSign * 32.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer, AllottedGeometry.ToPaintGeometry(), CurlPoints, ESlateDrawEffect::None, FiligreeTint, true, 2.2f);
	}
	++MutableNativeFiligreePaintCount;

	++CurrentLayer;
	const float GaugePercent = LastUltimateGaugeMax > 0.0f ? FMath::Clamp(LastUltimateGaugeValue / LastUltimateGaugeMax, 0.0f, 1.0f) : 0.0f;
	const FVector2f GaugePosition = PanelPosition + V2f(28.0f, PanelSize.Y - 34.0f);
	const FVector2f GaugeSize(PanelSize.X - 56.0f, 12.0f);
	const FSlateBrush GaugeBackBrush = MakeTintBrush(FLinearColor(0.23f, 0.16f, 0.30f, 0.92f));
	const FSlateBrush GaugeFillBrush = MakeTintBrush(bUltimateReadyVisible ? FiligreeTint : UltimateGaugeTint);
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, GaugePosition, GaugeSize), &GaugeBackBrush, ESlateDrawEffect::None, FLinearColor(0.23f, 0.16f, 0.30f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 1, PaintBox(AllottedGeometry, GaugePosition, V2f(GaugeSize.X * GaugePercent, GaugeSize.Y)), &GaugeFillBrush, ESlateDrawEffect::None, bUltimateReadyVisible ? FiligreeTint : UltimateGaugeTint);
	CurrentLayer += 2;
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, PanelPosition + V2f(30.0f, 18.0f), TEXT("MELUSINA"), 14, FLinearColor(0.98f, 0.88f, 1.0f, 0.94f));
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, PanelPosition + V2f(30.0f, 44.0f), LastActionPromptText.IsEmpty() ? TEXT("Basic | Skill | Ultimate") : LastActionPromptText, 13, FLinearColor(0.82f, 0.74f, 0.96f, 0.92f));
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, GaugePosition + V2f(0.0f, -22.0f), FString::Printf(TEXT("ULT %d%%"), FMath::RoundToInt(GaugePercent * 100.0f)), 12, bUltimateReadyVisible ? FiligreeTint : UltimateGaugeTint);
	++MutableNativeLabelPaintCount;

	const int32 SkillPointMax = FMath::Clamp(LastSkillPointMax, 1, 5);
	const int32 SkillPoints = FMath::Clamp(LastSkillPoints, 0, SkillPointMax);
	const FVector2f SkillPipStart = PanelPosition + V2f(PanelSize.X - 154.0f, 24.0f);
	const FVector2f SkillPipSize(18.0f, 18.0f);
	for (int32 PipIndex = 0; PipIndex < SkillPointMax; ++PipIndex)
	{
		const bool bFilled = PipIndex < SkillPoints;
		const FVector2f PipPosition = SkillPipStart + V2f(static_cast<float>(PipIndex) * 24.0f, 0.0f);
		const FLinearColor PipTint = bFilled ? FLinearColor(0.56f, 0.88f, 1.0f, 0.95f) : FLinearColor(0.24f, 0.18f, 0.30f, 0.82f);
		const FSlateBrush PipBrush = MakeTintBrush(PipTint);
		FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, PipPosition, SkillPipSize), &PipBrush, ESlateDrawEffect::None, PipTint);
		TArray<FVector2f> PipLine;
		PipLine.Add(PipPosition + V2f(4.0f, 13.0f));
		PipLine.Add(PipPosition + V2f(9.0f, 5.0f));
		PipLine.Add(PipPosition + V2f(14.0f, 13.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer + 1, AllottedGeometry.ToPaintGeometry(), PipLine, ESlateDrawEffect::None, bFilled ? PanelTint : FiligreeTint, true, 1.5f);
	}
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer + 1, SkillPipStart + V2f(-26.0f, -2.0f), TEXT("SP"), 12, FLinearColor(0.74f, 0.92f, 1.0f, 0.92f));
	CurrentLayer += 2;
	++MutableNativeLabelPaintCount;
	++MutableNativeSkillPointPaintCount;

	const float EnemyHPPercent = LastEnemyMaxHP > 0.0f ? FMath::Clamp(LastEnemyHP / LastEnemyMaxHP, 0.0f, 1.0f) : 0.0f;
	const FVector2f EnemyPanelSize(FMath::Clamp(LocalSize.X * 0.34f, 360.0f, 560.0f), 66.0f);
	const FVector2f EnemyPanelPosition((LocalSize.X - EnemyPanelSize.X) * 0.5f, 28.0f);
	const FSlateBrush EnemyPanelBrush = MakeTintBrush(FLinearColor(0.12f, 0.07f, 0.12f, 0.72f));
	const FSlateBrush EnemyHPBackBrush = MakeTintBrush(FLinearColor(0.24f, 0.10f, 0.18f, 0.92f));
	const FSlateBrush EnemyHPFillBrush = MakeTintBrush(FLinearColor(0.96f, 0.28f, 0.58f, 0.95f));
	const float BreakPercent = LastEnemyToughnessMax > 0.0f ? FMath::Clamp(LastEnemyToughness / LastEnemyToughnessMax, 0.0f, 1.0f) : 0.0f;
	const FSlateBrush BreakBackBrush = MakeTintBrush(FLinearColor(0.10f, 0.14f, 0.24f, 0.92f));
	const FSlateBrush BreakFillBrush = MakeTintBrush(bEnemyBreakVisible ? FiligreeTint : FLinearColor(0.42f, 0.82f, 1.0f, 0.95f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, EnemyPanelPosition, EnemyPanelSize), &EnemyPanelBrush, ESlateDrawEffect::None, FLinearColor(0.12f, 0.07f, 0.12f, 0.72f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 1, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 43.0f), V2f(EnemyPanelSize.X - 36.0f, 8.0f)), &EnemyHPBackBrush, ESlateDrawEffect::None, FLinearColor(0.24f, 0.10f, 0.18f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 2, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 43.0f), V2f((EnemyPanelSize.X - 36.0f) * EnemyHPPercent, 8.0f)), &EnemyHPFillBrush, ESlateDrawEffect::None, FLinearColor(0.96f, 0.28f, 0.58f, 0.95f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 3, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 54.0f), V2f(EnemyPanelSize.X - 36.0f, 5.0f)), &BreakBackBrush, ESlateDrawEffect::None, FLinearColor(0.10f, 0.14f, 0.24f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 4, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 54.0f), V2f((EnemyPanelSize.X - 36.0f) * BreakPercent, 5.0f)), &BreakFillBrush, ESlateDrawEffect::None, bEnemyBreakVisible ? FiligreeTint : FLinearColor(0.42f, 0.82f, 1.0f, 0.95f));
	CurrentLayer += 5;
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition + V2f(20.0f, 7.0f), TEXT("MELODY SLIME"), 13, FLinearColor(1.0f, 0.82f, 0.92f, 0.94f));
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition + V2f(EnemyPanelSize.X - 116.0f, 7.0f), FString::Printf(TEXT("%.0f/%.0f"), LastEnemyHP, LastEnemyMaxHP), 12, FLinearColor(1.0f, 0.74f, 0.82f, 0.92f));
	const FLinearColor IntentTint = bLastUltimateInterrupted ? FLinearColor(1.0f, 0.82f, 0.32f, 0.95f) : (bLastUltimateWindow ? FiligreeTint : FLinearColor(0.78f, 0.84f, 1.0f, 0.90f));
	const FString IntentText = FString::Printf(TEXT("%s -> %s %.0f"), LastCommandName.IsEmpty() ? TEXT("Ready") : *LastCommandName, *LastEnemyIntentName, LastEnemyIntentPower);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition + V2f(20.0f, 25.0f), IntentText, 12, IntentTint);
	++MutableNativeLabelPaintCount;
	++MutableNativeEnemyVitalsPaintCount;
	++MutableNativeIntentPaintCount;
	++MutableNativeBreakGaugePaintCount;

	const bool bFollowUpActive = bBreakFollowUpAvailableVisible || bBreakFollowUpConsumedVisible;
	const float BadgePulse = 0.5f + 0.5f * FMath::Sin(Time * 5.0f);
	const FVector2f BadgeSize(166.0f, 28.0f);
	const FVector2f BadgePosition = EnemyPanelPosition + V2f((EnemyPanelSize.X - BadgeSize.X) * 0.5f, EnemyPanelSize.Y + 8.0f);
	const FLinearColor BadgeTint = bBreakFollowUpAvailableVisible
		? FLinearColor(0.98f, 0.78f + 0.12f * BadgePulse, 0.36f, 0.94f)
		: (bBreakFollowUpConsumedVisible ? FLinearColor(0.68f, 0.52f, 0.96f, 0.88f) : FLinearColor(0.18f, 0.14f, 0.24f, 0.58f));
	const FLinearColor BadgeTextTint = bFollowUpActive ? PanelTint : FLinearColor(0.62f, 0.55f, 0.72f, 0.78f);
	const FSlateBrush BadgeBrush = MakeTintBrush(BadgeTint);
	const FString BadgeText = bBreakFollowUpAvailableVisible
		? TEXT("FOLLOW-UP READY")
		: (bBreakFollowUpConsumedVisible ? FString::Printf(TEXT("FOLLOW +%.0f"), LastFollowUpBonusDamage) : TEXT("FOLLOW-UP"));
	FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, BadgePosition, BadgeSize), &BadgeBrush, ESlateDrawEffect::None, BadgeTint);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer + 1, BadgePosition + V2f(16.0f, 6.0f), BadgeText, 12, BadgeTextTint);
	CurrentLayer += 2;
	++MutableNativeFollowUpPaintCount;
	++MutableNativeLabelPaintCount;

	const FVector2f TurnRailPosition(LocalSize.X - 96.0f, 112.0f);
	const FVector2f TurnSlotSize(66.0f, 44.0f);
	const int32 TurnSlotCount = FMath::Max(4, LastTurnOrderPreview.Num());
	for (int32 SlotIndex = 0; SlotIndex < TurnSlotCount; ++SlotIndex)
	{
		const float SlotY = TurnRailPosition.Y + static_cast<float>(SlotIndex) * 52.0f;
		const bool bActiveSlot = SlotIndex == ActiveTurnOrderIndex;
		const FLinearColor SlotTint = bActiveSlot ? FLinearColor(0.98f, 0.78f, 0.32f, 0.92f) : FLinearColor(0.20f, 0.16f, 0.28f, 0.78f);
		const FLinearColor InnerTint = SlotIndex % 2 == 0 ? FLinearColor(0.68f, 0.48f, 0.92f, 0.92f) : FLinearColor(0.44f, 0.72f, 0.96f, 0.92f);
		const FString UnitName = LastTurnOrderPreview.IsValidIndex(SlotIndex) ? LastTurnOrderPreview[SlotIndex] : (SlotIndex % 2 == 0 ? TEXT("Melusina") : TEXT("Slime"));
		const FSlateBrush SlotBrush = MakeTintBrush(SlotTint);
		const FSlateBrush InnerBrush = MakeTintBrush(InnerTint);
		FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, V2f(TurnRailPosition.X, SlotY), TurnSlotSize), &SlotBrush, ESlateDrawEffect::None, SlotTint);
		FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer + 1, PaintBox(AllottedGeometry, V2f(TurnRailPosition.X + 12.0f, SlotY + 9.0f), V2f(22.0f, 22.0f)), &InnerBrush, ESlateDrawEffect::None, InnerTint);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer + 2, V2f(TurnRailPosition.X + 17.0f, SlotY + 9.0f), InitialForName(UnitName), 14, FLinearColor(0.06f, 0.04f, 0.08f, 0.95f));
		TArray<FVector2f> TickPoints;
		TickPoints.Add(V2f(TurnRailPosition.X - 10.0f, SlotY + 22.0f));
		TickPoints.Add(V2f(TurnRailPosition.X - (bActiveSlot ? 30.0f : 20.0f), SlotY + 22.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer + 3, AllottedGeometry.ToPaintGeometry(), TickPoints, ESlateDrawEffect::None, FiligreeTint, true, bActiveSlot ? 3.0f : 1.4f);
		CurrentLayer += 4;
	}
	++MutableNativeLabelPaintCount;
	++MutableNativePortraitPaintCount;
	++MutableNativeTurnRailPaintCount;

	const FVector2f CommandStart(PanelPosition.X + PanelSize.X + 22.0f, LocalSize.Y - 82.0f);
	const FVector2f CommandSize(112.0f, 52.0f);
	const FString CommandLabels[3] = { TEXT("BASIC"), TEXT("SKILL"), TEXT("ULT") };
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		const bool bUltimateCard = CardIndex == 2;
		const bool bHighlighted = bUltimateCard && bUltimateReadyVisible;
		const bool bSkillCard = CardIndex == 1;
		const bool bSkillAvailable = LastSkillPoints > 0;
		const FVector2f CardPosition = CommandStart + V2f(static_cast<float>(CardIndex) * 122.0f, 0.0f);
		if (CardPosition.X + CommandSize.X > LocalSize.X - 120.0f)
		{
			break;
		}
		const FLinearColor CardTint = bHighlighted
			? FLinearColor(0.98f, 0.70f, 0.24f, 0.94f)
			: ((bSkillCard && !bSkillAvailable) ? FLinearColor(0.11f, 0.09f, 0.13f, 0.72f) : FLinearColor(0.16f, 0.12f, 0.22f, 0.84f));
		const FLinearColor CommandTextTint = (bSkillCard && !bSkillAvailable)
			? FLinearColor(0.52f, 0.48f, 0.58f, 0.82f)
			: (bHighlighted ? PanelTint : FLinearColor(0.98f, 0.88f, 1.0f, 0.94f));
		const FSlateBrush CardBrush = MakeTintBrush(CardTint);
		FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, CardPosition, CommandSize), &CardBrush, ESlateDrawEffect::None, CardTint);
		TArray<FVector2f> CardGlyph;
		CardGlyph.Add(CardPosition + V2f(20.0f, 36.0f));
		CardGlyph.Add(CardPosition + V2f(38.0f, 16.0f + static_cast<float>(CardIndex) * 3.0f));
		CardGlyph.Add(CardPosition + V2f(58.0f, 32.0f));
		CardGlyph.Add(CardPosition + V2f(82.0f, 18.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer + 1, AllottedGeometry.ToPaintGeometry(), CardGlyph, ESlateDrawEffect::None, bHighlighted ? PanelTint : CommandTextTint, true, 2.0f);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer + 2, CardPosition + V2f(16.0f, 8.0f), CommandLabels[CardIndex], 12, CommandTextTint);
		CurrentLayer += 3;
	}
	++MutableNativeLabelPaintCount;
	++MutableNativeCommandCardPaintCount;

	const float DamageFlashAge = Time - LastDamageFlashTime;
	if (DamageFlashAge >= 0.0f && DamageFlashAge <= 0.45f)
	{
		const float DamageFlashAlpha = FMath::Clamp(1.0f - DamageFlashAge / 0.45f, 0.0f, 1.0f);
		const FSlateBrush FlashBrush = MakeTintBrush(FLinearColor(1.0f, 0.85f, 0.35f, 0.22f * DamageFlashAlpha));
		FSlateDrawElement::MakeBox(OutDrawElements, CurrentLayer, PaintBox(AllottedGeometry, V2f(0.0f, 0.0f), LocalSize), &FlashBrush, ESlateDrawEffect::None, FLinearColor(1.0f, 0.85f, 0.35f, 0.22f * DamageFlashAlpha));
		++CurrentLayer;
		++MutableNativeDamageFlashPaintCount;
	}

	const float BurstAge = Time - LastSparkleBurstTime;
	const float BurstAlpha = FMath::Clamp(1.0f - BurstAge / 1.2f, 0.0f, 1.0f);
	const int32 SparkleTotal = BurstAlpha > 0.0f ? 18 : 8;
	for (int32 SparkleIndex = 0; SparkleIndex < SparkleTotal; ++SparkleIndex)
	{
		const float Seed = static_cast<float>(SparkleIndex);
		const float Phase = Time * (0.9f + Seed * 0.07f) + Seed * 1.618f;
		const float Drift = FMath::Frac(FMath::Sin(Seed * 12.9898f) * 43758.5453f);
		const FVector2f Center = PanelPosition + V2f(
			PanelSize.X * (0.12f + 0.78f * Drift),
			PanelSize.Y * (0.18f + 0.48f * FMath::Frac(Drift * 2.37f)) - FMath::Sin(Phase) * 8.0f);
		const float Radius = (BurstAlpha > 0.0f ? 4.5f + BurstAlpha * 6.0f : 3.0f) + FMath::Sin(Phase * 2.0f) * 1.5f;
		const FLinearColor LocalSparkleTint = FLinearColor(SparkleTint.R, SparkleTint.G, SparkleTint.B, SparkleTint.A * (0.35f + BurstAlpha * 0.65f));
		TArray<FVector2f> SparkleH;
		SparkleH.Add(Center + V2f(-Radius, 0.0f));
		SparkleH.Add(Center + V2f(Radius, 0.0f));
		TArray<FVector2f> SparkleV;
		SparkleV.Add(Center + V2f(0.0f, -Radius));
		SparkleV.Add(Center + V2f(0.0f, Radius));
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer, AllottedGeometry.ToPaintGeometry(), SparkleH, ESlateDrawEffect::None, LocalSparkleTint, true, 1.5f);
		FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer, AllottedGeometry.ToPaintGeometry(), SparkleV, ESlateDrawEffect::None, LocalSparkleTint, true, 1.5f);
	}
	++MutableNativeSparklePaintCount;
	SyncMutablePaintStats();

	return CurrentLayer + 1;
}

void UMelodiaRhythmHUDWidget::DoPulse_Implementation()
{
	if (UWidget* PulseRing = FindWidgetByName(TEXT("PulseRing")))
	{
		PulseRing->SetRenderOpacity(1.0f);
	}
}

void UMelodiaRhythmHUDWidget::SetJudgment_Implementation(const FText& NewText)
{
	if (UTextBlock* JudgmentText = FindTextBlockByName(TEXT("JudgmentText")))
	{
		JudgmentText->SetText(NewText);
		JudgmentText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMelodiaRhythmHUDWidget::SetJudgmentString_Implementation(const FString& NewText)
{
	SetJudgment(FText::FromString(NewText));
}

void UMelodiaRhythmHUDWidget::ShowBattleStatus_Implementation(const FString& NewStatusText)
{
	LastBattleStatusText = NewStatusText;
	if (NewStatusText.Contains(TEXT("Explore")))
	{
		++ExplorationPromptCount;
	}

	if (NewStatusText.Contains(TEXT("Battle started")))
	{
		++BattleStartPromptCount;
	}

	SetJudgmentString(NewStatusText);
}

void UMelodiaRhythmHUDWidget::ShowVictoryReward_Implementation(const FString& NewRewardText)
{
	LastRewardText = NewRewardText;
	bVictoryFeedbackVisible = true;
	SetJudgmentString(NewRewardText);
}

void UMelodiaRhythmHUDWidget::ClearBattleStatus_Implementation()
{
	LastBattleStatusText.Reset();
	LastRewardText.Reset();
	bVictoryFeedbackVisible = false;
}

void UMelodiaRhythmHUDWidget::SetUltimateGauge_Implementation(const float CurrentValue, const float MaxValue, const bool bReady)
{
	LastUltimateGaugeMax = FMath::Max(1.0f, MaxValue);
	LastUltimateGaugeValue = FMath::Clamp(CurrentValue, 0.0f, LastUltimateGaugeMax);
	bUltimateReadyVisible = bReady;

	if (UTextBlock* UltimateText = FindTextBlockByName(TEXT("UltimateText")))
	{
		const int32 GaugePercent = FMath::RoundToInt((LastUltimateGaugeValue / LastUltimateGaugeMax) * 100.0f);
		UltimateText->SetText(FText::FromString(FString::Printf(TEXT("ULT %d%%"), GaugePercent)));
		UltimateText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMelodiaRhythmHUDWidget::SetSkillPoints_Implementation(const int32 CurrentValue, const int32 MaxValue)
{
	LastSkillPointMax = FMath::Clamp(MaxValue, 1, 5);
	LastSkillPoints = FMath::Clamp(CurrentValue, 0, LastSkillPointMax);
	++SkillPointUpdateCount;
}

void UMelodiaRhythmHUDWidget::ShowUltimateReady_Implementation()
{
	bUltimateReadyVisible = true;
	++UltimateReadyPromptCount;
	SetJudgmentString(TEXT("Ultimate ready"));
	ShowActionPrompt(TEXT("Ultimate ready"));
	TriggerSparkleBurst();
}

void UMelodiaRhythmHUDWidget::ShowUltimateActivated_Implementation(const float DamageValue)
{
	bUltimateReadyVisible = false;
	++UltimateActivationPromptCount;
	SetJudgmentString(FString::Printf(TEXT("Ultimate %.0f"), DamageValue));
	ShowActionPrompt(TEXT("Ultimate unleashed"));
	TriggerDamageFlash(DamageValue);
	TriggerSparkleBurst();
}

void UMelodiaRhythmHUDWidget::ApplyCuteCombatTheme_Implementation()
{
	bCuteCombatThemeApplied = true;
	if (LastTurnOrderPreview.Num() == 0)
	{
		TArray<FString> DefaultTurnOrder;
		DefaultTurnOrder.Add(TEXT("Melusina"));
		DefaultTurnOrder.Add(TEXT("Slime"));
		DefaultTurnOrder.Add(TEXT("Melusina"));
		DefaultTurnOrder.Add(TEXT("Slime"));
		SetTurnOrderPreview(DefaultTurnOrder, 0);
	}
	if (LastActionPromptText.IsEmpty())
	{
		ShowActionPrompt(TEXT("Basic | Skill | Ultimate"));
	}
	if (SkillPointUpdateCount == 0)
	{
		SetSkillPoints(LastSkillPoints, LastSkillPointMax);
	}
	if (EnemyBreakGaugeUpdateCount == 0)
	{
		SetEnemyBreakGauge(LastEnemyToughness, LastEnemyToughnessMax, bEnemyBreakVisible);
	}
	if (BreakFollowUpUpdateCount == 0)
	{
		SetBreakFollowUpWindow(false, false, 0.0f);
	}
	if (ReactiveStateUpdateCount == 0)
	{
		SetReactiveBattleState(TEXT("Ready"), TEXT("Waiting"), 0.0f, false, false);
	}
	if (DamageFlashCount == 0)
	{
		TriggerDamageFlash(1.0f);
	}
	if (UWidget* FiligreeFrame = FindWidgetByName(TEXT("FiligreeFrame")))
	{
		FiligreeFrame->SetRenderOpacity(1.0f);
		FiligreeFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UMelodiaRhythmHUDWidget::SetEnemyVitals_Implementation(const float CurrentHP, const float MaxHP)
{
	LastEnemyMaxHP = FMath::Max(1.0f, MaxHP);
	LastEnemyHP = FMath::Clamp(CurrentHP, 0.0f, LastEnemyMaxHP);
	++EnemyVitalsUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetEnemyBreakGauge_Implementation(const float CurrentValue, const float MaxValue, const bool bBroken)
{
	LastEnemyToughnessMax = FMath::Max(1.0f, MaxValue);
	LastEnemyToughness = FMath::Clamp(CurrentValue, 0.0f, LastEnemyToughnessMax);
	bEnemyBreakVisible = bBroken;
	++EnemyBreakGaugeUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetBreakFollowUpWindow_Implementation(const bool bAvailable, const bool bConsumed, const float BonusDamage)
{
	bBreakFollowUpAvailableVisible = bAvailable;
	bBreakFollowUpConsumedVisible = bConsumed;
	LastFollowUpBonusDamage = FMath::Max(0.0f, BonusDamage);
	++BreakFollowUpUpdateCount;
	if (bAvailable || bConsumed)
	{
		TriggerSparkleBurst();
	}
}

void UMelodiaRhythmHUDWidget::SetEnemyTurnDelay_Implementation(const int32 DelayStacks, const int32 LastDelayApplied)
{
	LastEnemyTurnDelayStacks = FMath::Max(0, DelayStacks);
	LastEnemyTurnDelayApplied = FMath::Max(0, LastDelayApplied);
	++EnemyTurnDelayUpdateCount;
	if (LastEnemyTurnDelayApplied > 0)
	{
		TriggerSparkleBurst();
	}
}

void UMelodiaRhythmHUDWidget::SetTurnOrderPreview_Implementation(const TArray<FString>& UnitNames, const int32 ActiveIndex)
{
	LastTurnOrderPreview = UnitNames;
	if (LastTurnOrderPreview.Num() == 0)
	{
		LastTurnOrderPreview.Add(TEXT("Melusina"));
		LastTurnOrderPreview.Add(TEXT("Enemy"));
	}
	ActiveTurnOrderIndex = FMath::Clamp(ActiveIndex, 0, LastTurnOrderPreview.Num() - 1);
	++TurnOrderUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetReactiveBattleState_Implementation(const FString& CommandName, const FString& EnemyIntentName, const float EnemyIntentPower, const bool bUltimateWindow, const bool bInterrupted)
{
	LastCommandName = CommandName;
	LastEnemyIntentName = EnemyIntentName.IsEmpty() ? TEXT("Waiting") : EnemyIntentName;
	LastEnemyIntentPower = FMath::Max(0.0f, EnemyIntentPower);
	bLastUltimateWindow = bUltimateWindow;
	bLastUltimateInterrupted = bInterrupted;
	++ReactiveStateUpdateCount;
}

void UMelodiaRhythmHUDWidget::ShowActionPrompt_Implementation(const FString& PromptText)
{
	LastActionPromptText = PromptText;
	++ActionPromptCount;
}

void UMelodiaRhythmHUDWidget::TriggerDamageFlash_Implementation(const float DamageValue)
{
	LastDamageFlashValue = FMath::Max(0.0f, DamageValue);
	LastDamageFlashTime = GetHUDTimeSeconds();
	++DamageFlashCount;
}

void UMelodiaRhythmHUDWidget::TriggerSparkleBurst_Implementation()
{
	++SparkleBurstCount;
	LastSparkleBurstTime = GetHUDTimeSeconds();
	if (UWidget* SparkleLayer = FindWidgetByName(TEXT("SparkleLayer")))
	{
		SparkleLayer->SetRenderOpacity(1.0f);
		SparkleLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UWidget* UMelodiaRhythmHUDWidget::FindWidgetByName(const FName WidgetName) const
{
	return WidgetTree ? WidgetTree->FindWidget(WidgetName) : nullptr;
}

UTextBlock* UMelodiaRhythmHUDWidget::FindTextBlockByName(const FName WidgetName) const
{
	return Cast<UTextBlock>(FindWidgetByName(WidgetName));
}

float UMelodiaRhythmHUDWidget::GetHUDTimeSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0f;
}

void UMelodiaRhythmHUDWidget::SyncMutablePaintStats() const
{
	UMelodiaRhythmHUDWidget* MutableThis = const_cast<UMelodiaRhythmHUDWidget*>(this);
	MutableThis->NativePaintFrameCount = MutableNativePaintFrameCount;
	MutableThis->NativeFiligreePaintCount = MutableNativeFiligreePaintCount;
	MutableThis->NativeSparklePaintCount = MutableNativeSparklePaintCount;
	MutableThis->NativeTurnRailPaintCount = MutableNativeTurnRailPaintCount;
	MutableThis->NativeCommandCardPaintCount = MutableNativeCommandCardPaintCount;
	MutableThis->NativeSkillPointPaintCount = MutableNativeSkillPointPaintCount;
	MutableThis->NativeEnemyVitalsPaintCount = MutableNativeEnemyVitalsPaintCount;
	MutableThis->NativeIntentPaintCount = MutableNativeIntentPaintCount;
	MutableThis->NativeBreakGaugePaintCount = MutableNativeBreakGaugePaintCount;
	MutableThis->NativeFollowUpPaintCount = MutableNativeFollowUpPaintCount;
	MutableThis->NativeDamageFlashPaintCount = MutableNativeDamageFlashPaintCount;
	MutableThis->NativeLabelPaintCount = MutableNativeLabelPaintCount;
	MutableThis->NativePortraitPaintCount = MutableNativePortraitPaintCount;
}
