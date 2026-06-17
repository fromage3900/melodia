// Native base for rhythm HUD widgets so Blueprint callers have stable hooks.

#include "MelodiaRhythmHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Layout/Geometry.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaHUDDecor.h"
#include "MelodiaHUDFonts.h"
#include "MelodiaPortal.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRestPoint.h"
#include "MelodiaExplorationInputComponent.h"
#include "MelodiaGlideComponent.h"
#include "MelodiaPickableFlower.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaPCGEncounterSpawner.h"
#include "MelodiaPCGWalkableIndex.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

// ─────────────────────────────────────────────────────────────────────────────
UMelodiaRhythmHUDWidget* UMelodiaRhythmHUDWidget::FindFirst(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World)))
	{
		if (UMelodiaRhythmHUDWidget* ActiveWidget = GameMode->GetRhythmHUDWidget())
		{
			return ActiveWidget;
		}
	}

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			if (It->GetWorld() == World && It->GetOwningPlayer() == PlayerController)
			{
				return *It;
			}
		}
	}

	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		if (It->GetWorld() == World)
		{
			return *It;
		}
	}

	return nullptr;
}

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

void PaintLabel(
	FSlateWindowElementList& OutDrawElements,
	const FGeometry& Geometry,
	const int32 LayerId,
	const FVector2f Position,
	const FString& Text,
	const int32 FontSize,
	const FLinearColor& Tint,
	const MelodiaHUDFonts::ERole FontRole = MelodiaHUDFonts::ERole::Body,
	const float MaxWidth = 420.0f)
{
	const FSlateFontInfo FontInfo = MelodiaHUDFonts::Get(FontRole, FontSize);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId,
		PaintBox(Geometry, Position, V2f(MaxWidth, static_cast<float>(FontSize + 10))),
		FText::FromString(Text),
		FontInfo,
		ESlateDrawEffect::None,
		Tint);
}

void PaintSoftPanel(
	FSlateWindowElementList& OutDrawElements,
	const FGeometry& Geometry,
	int32& LayerId,
	const FVector2f Position,
	const FVector2f Size,
	const FLinearColor& FillTint,
	const FLinearColor& BorderTint,
	const float BorderThickness = 2.0f)
{
	const FSlateBrush PanelBrush = MakeTintBrush(FillTint);
	FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, PaintBox(Geometry, Position, Size), &PanelBrush, ESlateDrawEffect::None, FillTint);

	TArray<FVector2f> Border;
	Border.Add(Position + V2f(6.0f, 0.0f));
	Border.Add(Position + V2f(Size.X - 6.0f, 0.0f));
	Border.Add(Position + V2f(Size.X, 6.0f));
	Border.Add(Position + V2f(Size.X, Size.Y - 6.0f));
	Border.Add(Position + V2f(Size.X - 6.0f, Size.Y));
	Border.Add(Position + V2f(6.0f, Size.Y));
	Border.Add(Position + V2f(0.0f, Size.Y - 6.0f));
	Border.Add(Position + V2f(0.0f, 6.0f));
	Border.Add(Position + V2f(6.0f, 0.0f));
	FSlateDrawElement::MakeLines(OutDrawElements, ++LayerId, Geometry.ToPaintGeometry(), Border, ESlateDrawEffect::None, BorderTint, true, BorderThickness);
}

struct FMelodiaHUDLayout
{
	explicit FMelodiaHUDLayout(const FVector2f& InLocalSize)
		: LocalSize(InLocalSize)
	{
	}

	FVector2f LocalSize;
	float SafeSide = 24.0f;
	float SafeBottom = 22.0f;
	float ActionBarHeight = 118.0f;
	float CommandRowHeight = 54.0f;
	float EnemyPanelHeight = 84.0f;
	float EnemyPanelY = 18.0f;
	float FollowUpBadgeHeight = 32.0f;

	float ActionBarY() const { return LocalSize.Y - ActionBarHeight - SafeBottom; }
	float CommandRowY() const { return ActionBarY() - CommandRowHeight - 12.0f; }
	float EnemyBottomY() const { return EnemyPanelY + EnemyPanelHeight + FollowUpBadgeHeight + 10.0f; }
	float TurnRailX() const { return LocalSize.X - 82.0f - SafeSide; }
	float TurnRailYStart = 92.0f;
	float TurnRailYEnd() const { return CommandRowY() - 16.0f; }
	float PartyPanelWidth() const { return FMath::Min(LocalSize.X * 0.34f, 420.0f); }

	float HighwayLaneY() const
	{
		const float MinY = EnemyBottomY() + 20.0f;
		const float MaxY = CommandRowY() - 78.0f;
		return FMath::Clamp(LocalSize.Y * 0.43f, MinY, FMath::Max(MinY, MaxY));
	}
};

bool ShouldPaintExplorationHUD(const UMelodiaRhythmHUDWidget* Widget)
{
	if (!Widget || !Widget->bDrawExplorationHUD)
	{
		return false;
	}

	const UWorld* World = Widget->GetWorld();
	if (!World)
	{
		return true;
	}

	const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World));
	if (!GameMode)
	{
		return true;
	}

	return GameMode->CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady
		|| GameMode->CurrentLoopPhase == EMelodiaLoopPhase::Bootstrapping;
}

bool ShouldPaintBattleHUD(const UMelodiaRhythmHUDWidget* Widget)
{
	if (!Widget || !Widget->bDrawNativeCuteCombatHUD || !Widget->bCuteCombatThemeApplied)
	{
		return false;
	}

	const UWorld* World = Widget->GetWorld();
	if (!World)
	{
		return true;
	}

	const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World));
	if (!GameMode)
	{
		return true;
	}

	return GameMode->CurrentLoopPhase == EMelodiaLoopPhase::Battle
		|| GameMode->CurrentLoopPhase == EMelodiaLoopPhase::VictoryReward;
}

FString InitialForName(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return TEXT("?");
	}
	return Name.Left(1).ToUpper();
}

FLinearColor MelodiaGradeTint(const EMelodiaRhythmGrade RhythmGrade, const FLinearColor& DefaultTint)
{
	switch (RhythmGrade)
	{
	case EMelodiaRhythmGrade::Perfect:
		return FLinearColor(0.42f, 1.0f, 0.72f, 0.98f);
	case EMelodiaRhythmGrade::Great:
		return FLinearColor(0.62f, 0.92f, 1.0f, 0.96f);
	case EMelodiaRhythmGrade::Good:
		return FLinearColor(0.98f, 0.88f, 0.36f, 0.96f);
	case EMelodiaRhythmGrade::Miss:
		return FLinearColor(0.96f, 0.28f, 0.42f, 0.92f);
	default:
		return DefaultTint;
	}
}
}

void UMelodiaRhythmHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateSmoothedBars(InDeltaTime);
	SyncExplorationHUDFromWorld();
	SyncMutablePaintStats();
}

void UMelodiaRhythmHUDWidget::UpdateSmoothedBars(const float InDeltaTime)
{
	if (!bDisplayedValuesInitialized)
	{
		DisplayedEnemyHP = LastEnemyHP;
		DisplayedPartyHP = LastPartyHP;
		DisplayedUltimate = LastUltimateGaugeValue;
		DisplayedEnemyToughness = LastEnemyToughness;
		bDisplayedValuesInitialized = true;
		return;
	}

	// Frame-rate independent ease toward the authoritative targets for a satisfying JRPG drain.
	const float Alpha = FMath::Clamp(InDeltaTime * 7.0f, 0.0f, 1.0f);
	DisplayedEnemyHP = FMath::FInterpTo(DisplayedEnemyHP, LastEnemyHP, InDeltaTime, 7.0f);
	DisplayedPartyHP = FMath::FInterpTo(DisplayedPartyHP, LastPartyHP, InDeltaTime, 7.0f);
	DisplayedUltimate = FMath::FInterpTo(DisplayedUltimate, LastUltimateGaugeValue, InDeltaTime, 9.0f);
	DisplayedEnemyToughness = FMath::FInterpTo(DisplayedEnemyToughness, LastEnemyToughness, InDeltaTime, 9.0f);
	(void)Alpha;

	// Snap when extremely close so bars settle exactly on the target value.
	if (FMath::Abs(DisplayedEnemyHP - LastEnemyHP) < 0.5f) { DisplayedEnemyHP = LastEnemyHP; }
	if (FMath::Abs(DisplayedPartyHP - LastPartyHP) < 0.5f) { DisplayedPartyHP = LastPartyHP; }
	if (FMath::Abs(DisplayedUltimate - LastUltimateGaugeValue) < 0.5f) { DisplayedUltimate = LastUltimateGaugeValue; }
	if (FMath::Abs(DisplayedEnemyToughness - LastEnemyToughness) < 0.5f) { DisplayedEnemyToughness = LastEnemyToughness; }
}

int32 UMelodiaRhythmHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
	// WBP_RhythmHUD designer widgets (PulseRing, JudgmentText, etc.) overlap native HSR paint — skip them when native HUD is active.
	int32 CurrentLayer = LayerId;
	if (!bDrawNativeCuteCombatHUD)
	{
		CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	if (ShouldPaintExplorationHUD(this))
	{
		PaintExplorationHUD(OutDrawElements, AllottedGeometry, CurrentLayer);
	}

	if (!ShouldPaintBattleHUD(this))
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

	const FMelodiaHUDLayout Layout(LocalSize);
	const float Time = GetHUDTimeSeconds();
	const FLinearColor HSRGold = FiligreeTint;
	const FLinearColor HSRBlue = FLinearColor(0.52f, 0.78f, 1.0f, 0.94f);
	const FLinearColor HSRPanelFill = FLinearColor(0.09f, 0.07f, 0.14f, 0.88f);
	const FLinearColor HSRPanelBorder = FLinearColor(0.78f, 0.66f, 1.0f, 0.72f);

	// ── TOP: enemy status ───────────────────────────────────────────────────
	const float EnemyHPPercent = LastEnemyMaxHP > 0.0f ? FMath::Clamp(DisplayedEnemyHP / LastEnemyMaxHP, 0.0f, 1.0f) : 0.0f;
	const float BreakPercent = LastEnemyToughnessMax > 0.0f ? FMath::Clamp(DisplayedEnemyToughness / LastEnemyToughnessMax, 0.0f, 1.0f) : 0.0f;
	const FVector2f EnemyPanelSize(FMath::Clamp(LocalSize.X * 0.36f, 340.0f, 520.0f), Layout.EnemyPanelHeight);
	const FVector2f EnemyPanelPosition((LocalSize.X - EnemyPanelSize.X) * 0.5f, Layout.EnemyPanelY);
	PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition, EnemyPanelSize, HSRPanelFill, HSRPanelBorder);
	if (const FSlateBrush* FrameBrush = MelodiaHUDDecor::GetFrameBrush())
	{
		MelodiaHUDDecor::PaintDecorImage(OutDrawElements, AllottedGeometry, CurrentLayer, FrameBrush, EnemyPanelPosition - V2f(8.0f, 8.0f), EnemyPanelSize + V2f(16.0f, 16.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.35f));
	}

	const FSlateBrush EnemyHPBackBrush = MakeTintBrush(FLinearColor(0.20f, 0.10f, 0.18f, 0.92f));
	const FSlateBrush EnemyHPFillBrush = MakeTintBrush(FLinearColor(0.96f, 0.32f, 0.62f, 0.96f));
	const FSlateBrush BreakBackBrush = MakeTintBrush(FLinearColor(0.10f, 0.14f, 0.26f, 0.92f));
	const FSlateBrush BreakFillBrush = MakeTintBrush(bEnemyBreakVisible ? HSRGold : HSRBlue);
	const float BarWidth = EnemyPanelSize.X - 36.0f;
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 52.0f), V2f(BarWidth, 8.0f)), &EnemyHPBackBrush, ESlateDrawEffect::None, FLinearColor(0.20f, 0.10f, 0.18f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 52.0f), V2f(BarWidth * EnemyHPPercent, 8.0f)), &EnemyHPFillBrush, ESlateDrawEffect::None, FLinearColor(0.96f, 0.32f, 0.62f, 0.96f));
	if (!bCompactBattleHUD)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 64.0f), V2f(BarWidth, 5.0f)), &BreakBackBrush, ESlateDrawEffect::None, FLinearColor(0.10f, 0.14f, 0.26f, 0.92f));
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, EnemyPanelPosition + V2f(18.0f, 64.0f), V2f(BarWidth * BreakPercent, 5.0f)), &BreakFillBrush, ESlateDrawEffect::None, bEnemyBreakVisible ? HSRGold : HSRBlue);
	}
	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, EnemyPanelPosition + V2f(18.0f, 8.0f), TEXT("MELODY SLIME"), 15, FLinearColor(1.0f, 0.88f, 0.96f, 0.96f), MelodiaHUDFonts::ERole::Title, 260.0f);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition + V2f(EnemyPanelSize.X - 108.0f, 10.0f), FString::Printf(TEXT("%.0f / %.0f"), LastEnemyHP, LastEnemyMaxHP), 12, FLinearColor(1.0f, 0.78f, 0.86f, 0.94f), MelodiaHUDFonts::ERole::Bold, 120.0f);
	if (!bCompactBattleHUD)
	{
		const FLinearColor IntentTint = bLastUltimateInterrupted ? HSRGold : (bLastUltimateWindow ? HSRGold : HSRBlue);
		const FString IntentText = FString::Printf(TEXT("%s  ›  %s %.0f"), LastCommandName.IsEmpty() ? TEXT("Ready") : *LastCommandName, *LastEnemyIntentName, LastEnemyIntentPower);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, EnemyPanelPosition + V2f(18.0f, 30.0f), IntentText, 11, IntentTint, MelodiaHUDFonts::ERole::Body, BarWidth);
		++MutableNativeIntentPaintCount;
		++MutableNativeBreakGaugePaintCount;
	}
	++MutableNativeEnemyVitalsPaintCount;
	++MutableNativeLabelPaintCount;

	if (!bCompactBattleHUD)
	{
	const bool bFollowUpActive = bBreakFollowUpAvailableVisible || bBreakFollowUpConsumedVisible;
	const float BadgePulse = 0.5f + 0.5f * FMath::Sin(Time * 5.0f);
	const FVector2f BadgeSize(180.0f, Layout.FollowUpBadgeHeight);
	const FVector2f BadgePosition = EnemyPanelPosition + V2f((EnemyPanelSize.X - BadgeSize.X) * 0.5f, EnemyPanelSize.Y + 6.0f);
	const FLinearColor BadgeTint = bBreakFollowUpAvailableVisible
		? FLinearColor(0.98f, 0.78f + 0.12f * BadgePulse, 0.36f, 0.94f)
		: (bBreakFollowUpConsumedVisible ? FLinearColor(0.68f, 0.52f, 0.96f, 0.88f) : FLinearColor(0.14f, 0.11f, 0.20f, 0.45f));
	const FString BadgeText = bBreakFollowUpAvailableVisible
		? TEXT("FOLLOW-UP READY")
		: (bBreakFollowUpConsumedVisible ? FString::Printf(TEXT("FOLLOW +%.0f"), LastFollowUpBonusDamage) : TEXT(""));
	if (bFollowUpActive)
	{
		const FSlateBrush BadgeBrush = MakeTintBrush(BadgeTint);
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, BadgePosition, BadgeSize), &BadgeBrush, ESlateDrawEffect::None, BadgeTint);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, BadgePosition + V2f(14.0f, 7.0f), BadgeText, 12, PanelTint, MelodiaHUDFonts::ERole::Bold, 180.0f);
		++MutableNativeFollowUpPaintCount;
	}

	// ── RIGHT: turn order rail (stays above action bar, clear of center) ───────
	const FVector2f TurnSlotSize(68.0f, 42.0f);
	const int32 TurnSlotCount = FMath::Max(4, LastTurnOrderPreview.Num());
	const float TurnSlotStep = 48.0f;
	const float TurnRailMaxY = Layout.TurnRailYEnd();
	for (int32 SlotIndex = 0; SlotIndex < TurnSlotCount; ++SlotIndex)
	{
		const float SlotY = Layout.TurnRailYStart + static_cast<float>(SlotIndex) * TurnSlotStep;
		if (SlotY + TurnSlotSize.Y > TurnRailMaxY)
		{
			break;
		}
		const bool bActiveSlot = SlotIndex == ActiveTurnOrderIndex;
		const FLinearColor SlotTint = bActiveSlot ? HSRGold : FLinearColor(0.18f, 0.14f, 0.24f, 0.82f);
		const FLinearColor InnerTint = SlotIndex % 2 == 0 ? FLinearColor(0.68f, 0.48f, 0.92f, 0.92f) : HSRBlue;
		const FString UnitName = LastTurnOrderPreview.IsValidIndex(SlotIndex) ? LastTurnOrderPreview[SlotIndex] : (SlotIndex % 2 == 0 ? TEXT("Melusina") : TEXT("Slime"));
		const FSlateBrush SlotBrush = MakeTintBrush(SlotTint);
		const FSlateBrush InnerBrush = MakeTintBrush(InnerTint);
		const FVector2f SlotPos(Layout.TurnRailX(), SlotY);
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, SlotPos, TurnSlotSize), &SlotBrush, ESlateDrawEffect::None, SlotTint);
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, SlotPos + V2f(10.0f, 8.0f), V2f(24.0f, 24.0f)), &InnerBrush, ESlateDrawEffect::None, InnerTint);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, SlotPos + V2f(16.0f, 8.0f), InitialForName(UnitName), 14, FLinearColor(0.06f, 0.04f, 0.08f, 0.95f), MelodiaHUDFonts::ERole::Bold, 40.0f);
	}
	++MutableNativeTurnRailPaintCount;
	++MutableNativePortraitPaintCount;
	}

	// ── BOTTOM: party action bar ─────────────────────────────────────────────
	const FVector2f PanelSize(Layout.PartyPanelWidth(), Layout.ActionBarHeight);
	const FVector2f PanelPosition(Layout.SafeSide, Layout.ActionBarY());
	PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, PanelPosition, PanelSize, PanelTint, HSRGold, 2.2f);
	++MutableNativeFiligreePaintCount;

	const float GaugePercent = LastUltimateGaugeMax > 0.0f ? FMath::Clamp(DisplayedUltimate / LastUltimateGaugeMax, 0.0f, 1.0f) : 0.0f;
	const float PartyHPPercent = LastPartyMaxHP > 0.0f ? FMath::Clamp(DisplayedPartyHP / LastPartyMaxHP, 0.0f, 1.0f) : 0.0f;
	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, PanelPosition + V2f(18.0f, 10.0f), TEXT("MELUSINA"), 16, FLinearColor(0.98f, 0.90f, 1.0f, 0.96f), MelodiaHUDFonts::ERole::Title, 180.0f);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, PanelPosition + V2f(18.0f, 34.0f),
		LastActionPromptText.IsEmpty() ? TEXT("Space/1=Basic | 2=Skill (rhythm) | 4=Flee") : LastActionPromptText,
		12, FLinearColor(0.84f, 0.78f, 0.98f, 0.92f), MelodiaHUDFonts::ERole::Body, PanelSize.X - 36.0f);

	const FVector2f PartyGaugePosition = PanelPosition + V2f(18.0f, 58.0f);
	const FVector2f PartyGaugeSize(FMath::Min(200.0f, PanelSize.X * 0.46f), 9.0f);
	const FSlateBrush PartyGaugeBackBrush = MakeTintBrush(FLinearColor(0.16f, 0.11f, 0.20f, 0.92f));
	const FSlateBrush PartyGaugeFillBrush = MakeTintBrush(FLinearColor(0.42f, 0.92f, 0.72f, 0.96f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, PartyGaugePosition, PartyGaugeSize), &PartyGaugeBackBrush, ESlateDrawEffect::None, FLinearColor(0.16f, 0.11f, 0.20f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, PartyGaugePosition, V2f(PartyGaugeSize.X * PartyHPPercent, PartyGaugeSize.Y)), &PartyGaugeFillBrush, ESlateDrawEffect::None, FLinearColor(0.42f, 0.92f, 0.72f, 0.96f));
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, PartyGaugePosition + V2f(0.0f, -16.0f), FString::Printf(TEXT("HP %.0f/%.0f"), LastPartyHP, LastPartyMaxHP), 11, FLinearColor(0.72f, 0.98f, 0.84f, 0.92f), MelodiaHUDFonts::ERole::Bold, 160.0f);
	++MutableNativePartyVitalsPaintCount;

	if (!bCompactBattleHUD)
	{
	const FVector2f GaugePosition = PanelPosition + V2f(18.0f, PanelSize.Y - 24.0f);
	const FVector2f GaugeSize(PanelSize.X - 36.0f, 11.0f);
	const FSlateBrush GaugeBackBrush = MakeTintBrush(FLinearColor(0.20f, 0.14f, 0.28f, 0.92f));
	const FSlateBrush GaugeFillBrush = MakeTintBrush(bUltimateReadyVisible ? HSRGold : UltimateGaugeTint);
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, GaugePosition, GaugeSize), &GaugeBackBrush, ESlateDrawEffect::None, FLinearColor(0.20f, 0.14f, 0.28f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, GaugePosition, V2f(GaugeSize.X * GaugePercent, GaugeSize.Y)), &GaugeFillBrush, ESlateDrawEffect::None, bUltimateReadyVisible ? HSRGold : UltimateGaugeTint);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, GaugePosition + V2f(0.0f, -18.0f), FString::Printf(TEXT("ULT %d%%"), FMath::RoundToInt(GaugePercent * 100.0f)), 11, bUltimateReadyVisible ? HSRGold : UltimateGaugeTint, MelodiaHUDFonts::ERole::Accent, 120.0f);

	const int32 SkillPointMax = FMath::Clamp(LastSkillPointMax, 1, 5);
	const int32 SkillPoints = FMath::Clamp(LastSkillPoints, 0, SkillPointMax);
	const FVector2f SkillPipStart = PanelPosition + V2f(PanelSize.X - 28.0f - static_cast<float>(SkillPointMax) * 22.0f, 14.0f);
	for (int32 PipIndex = 0; PipIndex < SkillPointMax; ++PipIndex)
	{
		const bool bFilled = PipIndex < SkillPoints;
		const FVector2f PipPosition = SkillPipStart + V2f(static_cast<float>(PipIndex) * 22.0f, 0.0f);
		const FLinearColor PipTint = bFilled ? HSRBlue : FLinearColor(0.22f, 0.17f, 0.28f, 0.82f);
		if (bFilled && MelodiaHUDDecor::GetStarBrush())
		{
			MelodiaHUDDecor::PaintDecorImage(OutDrawElements, AllottedGeometry, ++CurrentLayer, MelodiaHUDDecor::GetStarBrush(), PipPosition, V2f(18.0f, 18.0f), PipTint);
		}
		else
		{
			const FSlateBrush PipBrush = MakeTintBrush(PipTint);
			FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, PipPosition, V2f(18.0f, 18.0f)), &PipBrush, ESlateDrawEffect::None, PipTint);
		}
	}
	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, SkillPipStart + V2f(-24.0f, 1.0f), TEXT("SP"), 12, HSRBlue, MelodiaHUDFonts::ERole::Bold, 40.0f);
	++MutableNativeSkillPointPaintCount;
	++MutableNativeLabelPaintCount;
	}

	if (!bCompactBattleHUD)
	{
	// ── CENTER-BOTTOM: command cards (HSR action row, centered above bar) ────
	const float CommandBannerAge = Time - LastBattlePhaseTime;
	const bool bEnemyTurnActive = LastBattlePhaseLabel.Equals(TEXT("Enemy Turn")) && CommandBannerAge >= 0.0f && CommandBannerAge <= 1.4f;
	const float CommandMenuAlpha = bEnemyTurnActive ? 0.34f : 1.0f;
	const FVector2f CommandSize(104.0f, Layout.CommandRowHeight);
	const float CommandGap = 12.0f;
	const float CommandRowWidth = 3.0f * CommandSize.X + 2.0f * CommandGap;
	const FVector2f CommandStart((LocalSize.X - CommandRowWidth) * 0.5f, Layout.CommandRowY());
	const FString CommandLabels[3] = { TEXT("BASIC"), TEXT("SKILL"), TEXT("ULT") };
	for (int32 CardIndex = 0; CardIndex < 3; ++CardIndex)
	{
		const bool bUltimateCard = CardIndex == 2;
		const bool bHighlighted = bUltimateCard && bUltimateReadyVisible && !bEnemyTurnActive;
		const bool bSkillCard = CardIndex == 1;
		const bool bSkillAvailable = LastSkillPoints > 0;
		const FVector2f CardPosition = CommandStart + V2f(static_cast<float>(CardIndex) * (CommandSize.X + CommandGap), 0.0f);
		FLinearColor CardTint = bHighlighted
			? FLinearColor(0.98f, 0.72f, 0.22f, 0.94f)
			: ((bSkillCard && !bSkillAvailable) ? FLinearColor(0.11f, 0.09f, 0.13f, 0.72f) : FLinearColor(0.14f, 0.11f, 0.20f, 0.86f));
		FLinearColor CommandTextTint = (bSkillCard && !bSkillAvailable)
			? FLinearColor(0.52f, 0.48f, 0.58f, 0.82f)
			: (bHighlighted ? PanelTint : FLinearColor(0.98f, 0.90f, 1.0f, 0.94f));
		CardTint.A *= CommandMenuAlpha;
		CommandTextTint.A *= CommandMenuAlpha;
		PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, CardPosition, CommandSize, CardTint, bHighlighted ? HSRGold : HSRPanelBorder, bHighlighted ? 2.4f : 1.6f);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, CardPosition + V2f(14.0f, 10.0f), CommandLabels[CardIndex], 13, CommandTextTint, MelodiaHUDFonts::ERole::Bold, 90.0f);
		if (bSkillCard && bNoteHighwayActive)
		{
			PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, CardPosition + V2f(14.0f, 30.0f), TEXT("♪ LIVE"), 10, HSRBlue, MelodiaHUDFonts::ERole::Musical, 90.0f);
		}
	}
	++MutableNativeCommandCardPaintCount;
	}

	// ── CENTER: note highway (between enemy and command row) ─────────────────
	if (bNoteHighwayActive && HighwayNotes.Num() > 0)
	{
		const float LaneWidth = FMath::Clamp(LocalSize.X * 0.56f, 420.0f, 820.0f);
		const float LaneHeight = 68.0f;
		const FVector2f LanePosition((LocalSize.X - LaneWidth) * 0.5f, Layout.HighwayLaneY());
		const FVector2f LaneSize(LaneWidth, LaneHeight);
		PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, LanePosition, LaneSize, FLinearColor(0.06f, 0.05f, 0.10f, 0.86f), HSRGold, 2.0f);

		const float HitX = LaneWidth * 0.72f;
		TArray<FVector2f> HitLine;
		HitLine.Add(LanePosition + V2f(HitX, 6.0f));
		HitLine.Add(LanePosition + V2f(HitX, LaneHeight - 6.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, ++CurrentLayer, AllottedGeometry.ToPaintGeometry(), HitLine, ESlateDrawEffect::None, HSRGold, true, 3.0f);

		const float ScrollBeats = FMath::Max(0.5f, HighwayScrollBeatsAhead);
		const float NoteWidth = 26.0f;
		const float NoteHeight = 42.0f;
		for (const FMelodiaHighwayNote& Note : HighwayNotes)
		{
			const float BeatDelta = Note.TargetBeat - HighwayBeatPosition;
			const float NormalizedX = HitX + (BeatDelta / ScrollBeats) * LaneWidth;
			if (NormalizedX < -NoteWidth || NormalizedX > LaneWidth + NoteWidth)
			{
				continue;
			}

			const FVector2f NotePosition = LanePosition + V2f(NormalizedX - NoteWidth * 0.5f, (LaneHeight - NoteHeight) * 0.5f);
			FLinearColor NoteTint(SparkleTint.R, SparkleTint.G, SparkleTint.B, 0.90f);
			if (Note.bResolved)
			{
				NoteTint = MelodiaGradeTint(Note.Grade, SparkleTint);
			}
			if (MelodiaHUDDecor::GetMagicBrush())
			{
				MelodiaHUDDecor::PaintDecorImage(OutDrawElements, AllottedGeometry, ++CurrentLayer, MelodiaHUDDecor::GetMagicBrush(), NotePosition, V2f(NoteWidth, NoteHeight), NoteTint);
			}
			else
			{
				const FSlateBrush NoteBrush = MakeTintBrush(NoteTint);
				FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, NotePosition, V2f(NoteWidth, NoteHeight)), &NoteBrush, ESlateDrawEffect::None, NoteTint);
			}
		}

		PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, LanePosition + V2f(10.0f, -20.0f), TEXT("NOTE HIGHWAY"), 12, FLinearColor(0.94f, 0.86f, 1.0f, 0.94f), MelodiaHUDFonts::ERole::Accent, 180.0f);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, LanePosition + V2f(LaneWidth - 150.0f, -20.0f), TEXT("Space / 1 = hit"), 11, HSRBlue, MelodiaHUDFonts::ERole::Body, 160.0f);
		++MutableNativeHighwayPaintCount;
	}

	// Damage flash overlay
	const float DamageFlashAge = Time - LastDamageFlashTime;
	if (DamageFlashAge >= 0.0f && DamageFlashAge <= 0.45f)
	{
		const float DamageFlashAlpha = FMath::Clamp(1.0f - DamageFlashAge / 0.45f, 0.0f, 1.0f);
		const FSlateBrush FlashBrush = MakeTintBrush(FLinearColor(1.0f, 0.85f, 0.35f, 0.18f * DamageFlashAlpha));
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, V2f(0.0f, 0.0f), LocalSize), &FlashBrush, ESlateDrawEffect::None, FLinearColor(1.0f, 0.85f, 0.35f, 0.18f * DamageFlashAlpha));
		++MutableNativeDamageFlashPaintCount;
	}

	// Sparkle decorations (Kenney sprites when available, line fallback otherwise)
	if (!bCompactBattleHUD)
	{
	const float BurstAge = Time - LastSparkleBurstTime;
	const float BurstAlpha = FMath::Clamp(1.0f - BurstAge / 1.2f, 0.0f, 1.0f);
	const int32 SparkleTotal = BurstAlpha > 0.0f ? 14 : 6;
	for (int32 SparkleIndex = 0; SparkleIndex < SparkleTotal; ++SparkleIndex)
	{
		const float Seed = static_cast<float>(SparkleIndex);
		const float Phase = Time * (0.9f + Seed * 0.07f) + Seed * 1.618f;
		const float Drift = FMath::Frac(FMath::Sin(Seed * 12.9898f) * 43758.5453f);
		const FVector2f Center = PanelPosition + V2f(
			PanelSize.X * (0.10f + 0.80f * Drift),
			PanelSize.Y * (0.20f + 0.50f * FMath::Frac(Drift * 2.37f)) - FMath::Sin(Phase) * 6.0f);
		const FLinearColor LocalSparkleTint(SparkleTint.R, SparkleTint.G, SparkleTint.B, SparkleTint.A * (0.35f + BurstAlpha * 0.65f));
		const float DecorSize = 12.0f + FMath::Sin(Phase * 2.0f) * 3.0f;
		if (const FSlateBrush* SparkBrush = (SparkleIndex % 2 == 0) ? MelodiaHUDDecor::GetSparkBrush() : MelodiaHUDDecor::GetStarBrush())
		{
			MelodiaHUDDecor::PaintDecorImage(OutDrawElements, AllottedGeometry, ++CurrentLayer, SparkBrush, Center - V2f(DecorSize * 0.5f, DecorSize * 0.5f), V2f(DecorSize, DecorSize), LocalSparkleTint);
		}
		else
		{
			const float Radius = DecorSize * 0.45f;
			TArray<FVector2f> SparkleH;
			SparkleH.Add(Center + V2f(-Radius, 0.0f));
			SparkleH.Add(Center + V2f(Radius, 0.0f));
			TArray<FVector2f> SparkleV;
			SparkleV.Add(Center + V2f(0.0f, -Radius));
			SparkleV.Add(Center + V2f(0.0f, Radius));
			FSlateDrawElement::MakeLines(OutDrawElements, ++CurrentLayer, AllottedGeometry.ToPaintGeometry(), SparkleH, ESlateDrawEffect::None, LocalSparkleTint, true, 1.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, CurrentLayer, AllottedGeometry.ToPaintGeometry(), SparkleV, ESlateDrawEffect::None, LocalSparkleTint, true, 1.5f);
		}
	}
	++MutableNativeSparklePaintCount;
	}

	// Floating combat text
	if (FloatingCombatTexts.Num() > 0)
	{
		const FVector2f EnemyAnchor = EnemyPanelPosition + V2f(EnemyPanelSize.X * 0.5f, EnemyPanelSize.Y + 4.0f);
		const FVector2f PartyAnchor = PanelPosition + V2f(100.0f, -10.0f);
		bool bPaintedAny = false;
		for (const FMelodiaFloatingCombatText& Popup : FloatingCombatTexts)
		{
			const float PopupAge = Time - Popup.SpawnTime;
			if (PopupAge < 0.0f || PopupAge > 1.15f)
			{
				continue;
			}
			const float PopupAlpha = FMath::Clamp(1.0f - PopupAge / 1.15f, 0.0f, 1.0f);
			const float Rise = PopupAge * 46.0f;
			const float Lateral = (Popup.LateralSeed - 0.5f) * 70.0f;
			const float Pop = PopupAge < 0.12f ? (PopupAge / 0.12f) : 1.0f;
			const FVector2f BaseAnchor = Popup.bAnchorEnemy ? EnemyAnchor : PartyAnchor;
			const FVector2f TextPos = BaseAnchor + V2f(Lateral - 18.0f, -Rise);
			const FLinearColor PopupTint(Popup.Tint.R, Popup.Tint.G, Popup.Tint.B, Popup.Tint.A * PopupAlpha);
			PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, TextPos, Popup.Text, FMath::RoundToInt(16.0f + 8.0f * Pop), PopupTint, MelodiaHUDFonts::ERole::Title, 160.0f);
			bPaintedAny = true;
		}
		if (bPaintedAny)
		{
			++MutableNativeFloatingTextPaintCount;
		}
	}

	// Turn-phase banner
	const float BannerAge = Time - LastBattlePhaseTime;
	if (!LastBattlePhaseLabel.IsEmpty() && BannerAge >= 0.0f && BannerAge <= 2.2f)
	{
		const float FadeIn = FMath::Clamp(BannerAge / 0.22f, 0.0f, 1.0f);
		const float FadeOut = FMath::Clamp((2.2f - BannerAge) / 0.5f, 0.0f, 1.0f);
		const float BannerAlpha = FMath::Min(FadeIn, FadeOut);
		const bool bVictory = LastBattlePhaseLabel.Contains(TEXT("Victory"));
		const bool bDefeat = LastBattlePhaseLabel.Contains(TEXT("Defeat"));
		const bool bEnemyBanner = LastBattlePhaseLabel.Contains(TEXT("Enemy"));
		FLinearColor BannerTint = bVictory
			? HSRGold
			: (bDefeat ? FLinearColor(0.92f, 0.30f, 0.40f, 0.92f)
				: (bEnemyBanner ? FLinearColor(0.86f, 0.40f, 0.58f, 0.90f) : HSRBlue));
		const FVector2f BannerSize(FMath::Clamp(LocalSize.X * 0.34f, 300.0f, 480.0f), 52.0f);
		const float BannerSlide = (1.0f - FadeIn) * 20.0f;
		const FVector2f BannerPos((LocalSize.X - BannerSize.X) * 0.5f, Layout.EnemyBottomY() + 24.0f - BannerSlide);
		PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, BannerPos, BannerSize, FLinearColor(0.06f, 0.05f, 0.10f, 0.80f * BannerAlpha), FLinearColor(BannerTint.R, BannerTint.G, BannerTint.B, BannerTint.A * BannerAlpha), 2.5f);
		PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, BannerPos + V2f(22.0f, 14.0f), LastBattlePhaseLabel.ToUpper(), 22, FLinearColor(BannerTint.R, BannerTint.G, BannerTint.B, BannerTint.A * BannerAlpha), MelodiaHUDFonts::ERole::Title, BannerSize.X - 40.0f);
		++MutableNativeBannerPaintCount;
	}

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

void UMelodiaRhythmHUDWidget::SetHUDMode(const EMelodiaHUDMode NewMode)
{
	ActiveHUDMode = NewMode;
	bDrawExplorationHUD = NewMode == EMelodiaHUDMode::Exploration;
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
	MelodiaHUDFonts::Warmup();
	MelodiaHUDDecor::Warmup();

	PanelTint = FLinearColor(0.09f, 0.07f, 0.14f, 0.90f);
	FiligreeTint = FLinearColor(0.98f, 0.82f, 0.38f, 0.94f);
	SparkleTint = FLinearColor(0.86f, 0.74f, 1.0f, 0.96f);
	UltimateGaugeTint = FLinearColor(0.62f, 0.48f, 0.98f, 0.95f);
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
		ShowActionPrompt(TEXT("Space/1=Basic | 2=Skill (rhythm) | 4=Flee"));
	}
	if (SkillPointUpdateCount == 0)
	{
		SetSkillPoints(LastSkillPoints, LastSkillPointMax);
	}
	if (PartyVitalsUpdateCount == 0)
	{
		SetPartyVitals(LastPartyHP, LastPartyMaxHP);
	}
	if (QuestLogUpdateCount == 0 && QuestLogEntries.Num() == 0)
	{
		TArray<FString> DefaultQuests;
		DefaultQuests.Add(TEXT("Whisper at the Gate (0/1)"));
		DefaultQuests.Add(TEXT("Slime Ballad (0/1)"));
		SetQuestLogEntries(DefaultQuests, TEXT("Journal ready"));
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
	if (bDrawNativeCuteCombatHUD && WidgetTree)
	{
		if (UWidget* Root = GetRootWidget())
		{
			WidgetTree->ForEachWidget([Root](UWidget* Widget)
			{
				if (Widget && Widget != Root)
				{
					Widget->SetVisibility(ESlateVisibility::Collapsed);
				}
			});
		}
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

void UMelodiaRhythmHUDWidget::SetPartyVitals_Implementation(const float CurrentHP, const float MaxHP)
{
	LastPartyMaxHP = FMath::Max(1.0f, MaxHP);
	LastPartyHP = FMath::Clamp(CurrentHP, 0.0f, LastPartyMaxHP);
	++PartyVitalsUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetBattlePhaseBanner_Implementation(const FString& PhaseLabel)
{
	LastBattlePhaseLabel = PhaseLabel;
	LastBattlePhaseTime = GetHUDTimeSeconds();
	LastTurnBannerText = PhaseLabel;
	LastTurnBannerTime = LastBattlePhaseTime;
	LastTurnBannerTint = PhaseLabel.Contains(TEXT("Enemy"))
		? FLinearColor(0.86f, 0.40f, 0.58f, 0.90f)
		: FLinearColor(0.52f, 0.78f, 1.0f, 0.90f);
	++TurnBannerCount;
	++BattlePhaseUpdateCount;
}

void UMelodiaRhythmHUDWidget::PushFloatingCombatText_Implementation(const FString& Text, const bool bAnchorEnemy, const FLinearColor Tint)
{
	const float Now = GetHUDTimeSeconds();
	FMelodiaFloatingCombatText Popup;
	Popup.Text = Text;
	Popup.SpawnTime = Now;
	Popup.bAnchorEnemy = bAnchorEnemy;
	Popup.Tint = Tint;
	Popup.LateralSeed = FMath::FRand();
	FloatingCombatTexts.Add(Popup);

	LastFloatingDamageValue = FMath::Abs(FCString::Atof(*Text));
	LastFloatingDamageTime = Now;
	bLastFloatingDamageAgainstEnemy = bAnchorEnemy;
	LastFloatingDamageTint = Tint;
	++FloatingDamageCount;
	++FloatingCombatTextCount;

	// Keep only the most recent few popups so the array does not grow unbounded.
	constexpr int32 MaxPopups = 8;
	if (FloatingCombatTexts.Num() > MaxPopups)
	{
		FloatingCombatTexts.RemoveAt(0, FloatingCombatTexts.Num() - MaxPopups, EAllowShrinking::No);
	}
}

void UMelodiaRhythmHUDWidget::SetNoteHighwayActive_Implementation(const bool bActive, const TArray<FMelodiaHighwayNote>& Notes, const float BeatPosition, const float ScrollBeatsAhead)
{
	bNoteHighwayActive = bActive;
	HighwayNotes = Notes;
	HighwayBeatPosition = BeatPosition;
	HighwayScrollBeatsAhead = FMath::Max(0.5f, ScrollBeatsAhead);
	++NoteHighwayUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetQuestLogEntries_Implementation(const TArray<FString>& Entries, const FString& ToastText)
{
	QuestLogEntries = Entries;
	LastQuestToastText = ToastText;
	++QuestLogUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetInventorySlots_Implementation(const TArray<FMelodiaInventorySlot>& Slots)
{
	InventorySlots = Slots;
	++InventoryUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetInventoryPanelOpen_Implementation(const bool bOpen)
{
	bInventoryPanelOpen = bOpen;
}

void UMelodiaRhythmHUDWidget::SetMinimapState_Implementation(const FVector MapCenterWorld, const float MapHalfExtentCm, const FVector PlayerWorldLocation, const TArray<FMelodiaMinimapMarker>& Markers)
{
	MinimapCenterWorld = MapCenterWorld;
	MinimapHalfExtentCm = FMath::Max(100.0f, MapHalfExtentCm);
	MinimapPlayerLocation = PlayerWorldLocation;
	MinimapMarkers = Markers;
	++MinimapUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetMinimapMarkers_Implementation(const TArray<FMelodiaMinimapMarker>& Markers)
{
	MinimapMarkers = Markers;
	++MinimapUpdateCount;
}

void UMelodiaRhythmHUDWidget::SetMechanicProgression_Implementation(
	const int32 MechanicLevel,
	const int32 CurrentXP,
	const int32 XPToNextLevel,
	const FString& TierDisplayName,
	const int32 UnlockedPresetCount)
{
	LastMechanicLevel = MechanicLevel;
	LastMechanicXP = CurrentXP;
	LastMechanicXPToNext = XPToNextLevel;
	LastMechanicTierName = TierDisplayName;
	UnlockedLocationPresetCount = UnlockedPresetCount;
	++MechanicProgressUpdateCount;
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
	MutableThis->NativeHighwayPaintCount = MutableNativeHighwayPaintCount;
	MutableThis->NativePartyVitalsPaintCount = MutableNativePartyVitalsPaintCount;
	MutableThis->NativeMinimapPaintCount = MutableNativeMinimapPaintCount;
	MutableThis->NativeQuestLogPaintCount = MutableNativeQuestLogPaintCount;
	MutableThis->NativeInventoryPaintCount = MutableNativeInventoryPaintCount;
	MutableThis->NativeBannerPaintCount = MutableNativeBannerPaintCount;
	MutableThis->NativeFloatingTextPaintCount = MutableNativeFloatingTextPaintCount;
}

void UMelodiaRhythmHUDWidget::SyncExplorationHUDFromWorld()
{
	UWorld* World = GetWorld();
	if (!World || !bDrawExplorationHUD)
	{
		return;
	}

	const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World));
	FVector MapCenter = GameMode ? GameMode->ExplorationReturnLocation : FVector::ZeroVector;
	float MapHalfExtent = 600.0f;

	if (GameMode && GameMode->bPCGPlacementActive && GameMode->ActiveWalkableIndex)
	{
		const TArray<FVector> WalkablePositions = GameMode->ActiveWalkableIndex->GetCachedPositions();
		if (WalkablePositions.Num() > 0)
		{
			FVector BoundsMin = WalkablePositions[0];
			FVector BoundsMax = WalkablePositions[0];
			for (const FVector& Pos : WalkablePositions)
			{
				BoundsMin = BoundsMin.ComponentMin(Pos);
				BoundsMax = BoundsMax.ComponentMax(Pos);
			}
			MapCenter = (BoundsMin + BoundsMax) * 0.5f;
			const float ExtentX = FMath::Max(400.0f, (BoundsMax.X - BoundsMin.X) * 0.55f);
			const float ExtentY = FMath::Max(400.0f, (BoundsMax.Y - BoundsMin.Y) * 0.55f);
			MapHalfExtent = FMath::Max(ExtentX, ExtentY);
		}
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn && GameMode)
	{
		PlayerPawn = GameMode->ActiveExplorationPawn.Get();
	}

	TArray<FMelodiaMinimapMarker> Markers;
	if (GameMode && GameMode->ActivePCGEncounterSpawner)
	{
		int32 GateIndex = 0;
		for (const TObjectPtr<AActor>& SpawnedActor : GameMode->ActivePCGEncounterSpawner->GetSpawnedActors())
		{
			if (!SpawnedActor)
			{
				continue;
			}

			FMelodiaMinimapMarker GateMarker;
			GateMarker.WorldLocation = SpawnedActor->GetActorLocation();
			GateMarker.Label = GateIndex == 0 ? TEXT("Song Gate") : FString::Printf(TEXT("Gate %d"), GateIndex + 1);
			GateMarker.Tint = FLinearColor(0.98f, 0.54f, 0.94f, 1.0f);
			GateMarker.bPulse = GateIndex == 0;
			Markers.Add(GateMarker);
			++GateIndex;
		}
	}

	if (Markers.Num() == 0 && GameMode && GameMode->ActiveEncounterTrigger)
	{
		FMelodiaMinimapMarker GateMarker;
		GateMarker.WorldLocation = GameMode->ActiveEncounterTrigger->GetActorLocation();
		GateMarker.Label = TEXT("Song Gate");
		GateMarker.Tint = FLinearColor(0.98f, 0.54f, 0.94f, 1.0f);
		GateMarker.bPulse = true;
		Markers.Add(GateMarker);
		MapCenter = (MapCenter + GateMarker.WorldLocation) * 0.5f;
	}

	if (GameMode && GameMode->ActiveRestPoint)
	{
		FMelodiaMinimapMarker BedMarker;
		BedMarker.WorldLocation = GameMode->ActiveRestPoint->GetActorLocation();
		BedMarker.Label = TEXT("Melusina's Bed");
		BedMarker.Tint = FLinearColor(0.72f, 0.92f, 1.0f, 1.0f);
		BedMarker.bPulse = false;
		Markers.Add(BedMarker);
	}

	if (GameMode && GameMode->ActivePortal)
	{
		FMelodiaMinimapMarker PortalMarker;
		PortalMarker.WorldLocation = GameMode->ActivePortal->GetActorLocation();
		PortalMarker.Label = TEXT("Reverie Portal");
		PortalMarker.Tint = FLinearColor(0.86f, 0.62f, 1.0f, 1.0f);
		PortalMarker.bPulse = true;
		Markers.Add(PortalMarker);
	}

	for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
	{
		Markers.Append(It->BuildQuestMarkers());
		break;
	}

	const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : MapCenter;
	SetMinimapState(MapCenter, MapHalfExtent, PlayerLocation, Markers);

	if (GameMode && PlayerPawn)
	{
		if (GameMode->CurrentLoopPhase == EMelodiaLoopPhase::VictoryReward)
		{
			ShowActionPrompt(TEXT("Victory! Space/1 to claim reward"));
		}
		else if (GameMode->CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady)
		{
			FString Prompt = TEXT("WASD explore | Space jump + hold glide | F pick flowers | E interact | I inventory");
			if (const UMelodiaExplorationInputComponent* ExplorationInput = PlayerPawn->FindComponentByClass<UMelodiaExplorationInputComponent>())
			{
				const FString PickPrompt = ExplorationInput->GetNearestPickPrompt();
				if (!PickPrompt.IsEmpty())
				{
					Prompt = PickPrompt;
				}
			}
			if (GameMode->ActiveRestPoint && GameMode->ActiveRestPoint->IsPawnInRange(PlayerPawn))
			{
				Prompt = GameMode->ActiveRestPoint->InteractionPrompt;
			}
			else if (GameMode->ActivePortal && GameMode->ActivePortal->IsPawnInRange(PlayerPawn))
			{
				Prompt = GameMode->ActivePortal->InteractionPrompt;
			}
			if (const UMelodiaGlideComponent* Glide = PlayerPawn->FindComponentByClass<UMelodiaGlideComponent>())
			{
				if (Glide->bIsGliding)
				{
					Prompt = TEXT("Gliding — release Space to fall");
				}
			}
			ShowActionPrompt(Prompt);
		}
	}
}

FVector2f UMelodiaRhythmHUDWidget::WorldToMinimap(const FVector2f& LocalSize, const FVector& WorldLocation) const
{
	const float Extent = FMath::Max(100.0f, MinimapHalfExtentCm);
	const float MapSize = 148.0f;
	const FVector2f MapTopRight(LocalSize.X - MapSize - 24.0f, 24.0f);

	const float NormalizedX = (WorldLocation.X - MinimapCenterWorld.X) / Extent;
	const float NormalizedY = (WorldLocation.Y - MinimapCenterWorld.Y) / Extent;
	return MapTopRight + V2f((NormalizedX * 0.5f + 0.5f) * MapSize, (NormalizedY * 0.5f + 0.5f) * MapSize);
}

void UMelodiaRhythmHUDWidget::PaintExplorationHUD(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32& CurrentLayer) const
{
	const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X < 320.0f || LocalSize.Y < 180.0f)
	{
		return;
	}

	const float MechanicBannerWidth = FMath::Clamp(LocalSize.X * 0.34f, 280.0f, 460.0f);
	const FVector2f MechanicBannerPos((LocalSize.X - MechanicBannerWidth) * 0.5f, 18.0f);
	PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, MechanicBannerPos, V2f(MechanicBannerWidth, 52.0f), FLinearColor(0.08f, 0.06f, 0.12f, 0.90f), FiligreeTint, 1.6f);
	const FString MechanicTitle = FString::Printf(TEXT("MECHANIC LV %d  ·  %s"), LastMechanicLevel, *LastMechanicTierName);
	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, MechanicBannerPos + V2f(14.0f, 8.0f), MechanicTitle, 13, FiligreeTint, MelodiaHUDFonts::ERole::Title, MechanicBannerWidth - 28.0f);
	const float XPPercent = LastMechanicXPToNext > 0 ? FMath::Clamp(static_cast<float>(LastMechanicXP) / static_cast<float>(LastMechanicXPToNext), 0.0f, 1.0f) : 1.0f;
	const FSlateBrush XPBackBrush = MakeTintBrush(FLinearColor(0.14f, 0.10f, 0.20f, 0.92f));
	const FSlateBrush XPFillBrush = MakeTintBrush(FLinearColor(0.62f, 0.48f, 0.98f, 0.96f));
	const FVector2f XPBarPos = MechanicBannerPos + V2f(14.0f, 30.0f);
	const float XPBarWidth = MechanicBannerWidth - 28.0f;
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, XPBarPos, V2f(XPBarWidth, 8.0f)), &XPBackBrush, ESlateDrawEffect::None, FLinearColor(0.14f, 0.10f, 0.20f, 0.92f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, XPBarPos, V2f(XPBarWidth * XPPercent, 8.0f)), &XPFillBrush, ESlateDrawEffect::None, FLinearColor(0.62f, 0.48f, 0.98f, 0.96f));
	const FString PresetLine = FString::Printf(TEXT("%d/%d location presets  ·  XP %d/%d"), UnlockedLocationPresetCount, 30, LastMechanicXP, LastMechanicXPToNext);
	PaintLabel(OutDrawElements, AllottedGeometry, CurrentLayer, MechanicBannerPos + V2f(MechanicBannerWidth - 168.0f, 8.0f), PresetLine, 10, FLinearColor(0.82f, 0.74f, 0.96f, 0.88f), MelodiaHUDFonts::ERole::Body, 160.0f);

	const float MapSize = 148.0f;
	const FVector2f MapTopRight(LocalSize.X - MapSize - 24.0f, 24.0f);
	const FSlateBrush MapBackBrush = MakeTintBrush(FLinearColor(0.08f, 0.06f, 0.12f, 0.88f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, MapTopRight, V2f(MapSize, MapSize)), &MapBackBrush, ESlateDrawEffect::None, FLinearColor(0.08f, 0.06f, 0.12f, 0.88f));

	const FVector2f PlayerDot = WorldToMinimap(LocalSize, MinimapPlayerLocation);
	const FSlateBrush PlayerBrush = MakeTintBrush(FLinearColor(0.42f, 0.92f, 0.72f, 1.0f));
	FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, PlayerDot - V2f(5.0f, 5.0f), V2f(10.0f, 10.0f)), &PlayerBrush, ESlateDrawEffect::None, FLinearColor(0.42f, 0.92f, 0.72f, 1.0f));

	const float Time = GetHUDTimeSeconds();

	for (const FMelodiaMinimapMarker& Marker : MinimapMarkers)
	{
		const FVector2f MarkerPos = WorldToMinimap(LocalSize, Marker.WorldLocation);
		FLinearColor MarkerTint = Marker.Tint;
		float MarkerSize = 8.0f;
		if (Marker.bPulse)
		{
			const float Pulse = 0.55f + 0.45f * FMath::Sin(Time * 4.2f);
			MarkerTint.A = FMath::Clamp(MarkerTint.A * Pulse, 0.35f, 1.0f);
			MarkerSize = 8.0f + Pulse * 4.0f;
		}
		if (Marker.bCompleted)
		{
			MarkerTint = FLinearColor(0.42f, 0.92f, 0.72f, 0.95f);
		}
		const FSlateBrush MarkerBrush = MakeTintBrush(MarkerTint);
		const float HalfSize = MarkerSize * 0.5f;
		FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, MarkerPos - V2f(HalfSize, HalfSize), V2f(MarkerSize, MarkerSize)), &MarkerBrush, ESlateDrawEffect::None, MarkerTint);
	}

	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, MapTopRight + V2f(0.0f, MapSize + 4.0f), TEXT("MINIMAP"), 11, FLinearColor(0.82f, 0.74f, 0.96f, 0.92f), MelodiaHUDFonts::ERole::Accent, 160.0f);
	++MutableNativeMinimapPaintCount;

	const FVector2f QuestPanelPos(24.0f, 24.0f);
	const float QuestPanelWidth = FMath::Clamp(LocalSize.X * 0.26f, 220.0f, 320.0f);
	const float QuestPanelHeight = 24.0f + FMath::Clamp(static_cast<float>(QuestLogEntries.Num()), 1.0f, 6.0f) * 22.0f + 18.0f;
	PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, QuestPanelPos, V2f(QuestPanelWidth, QuestPanelHeight), FLinearColor(0.10f, 0.08f, 0.14f, 0.88f), FiligreeTint, 1.8f);
	PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, QuestPanelPos + V2f(12.0f, 8.0f), TEXT("QUESTS"), 13, FiligreeTint, MelodiaHUDFonts::ERole::Title, 180.0f);

	int32 LineIndex = 0;
	for (const FString& Entry : QuestLogEntries)
	{
		const bool bCompletedLine = Entry.StartsWith(TEXT("✓"));
		const FLinearColor LineTint = bCompletedLine
			? FLinearColor(0.42f, 0.92f, 0.72f, 0.95f)
			: FLinearColor(0.88f, 0.84f, 0.98f, 0.92f);
		PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, QuestPanelPos + V2f(12.0f, 28.0f + static_cast<float>(LineIndex) * 22.0f), Entry, 11, LineTint, MelodiaHUDFonts::ERole::Body, QuestPanelWidth - 24.0f);
		++LineIndex;
		if (LineIndex >= 6)
		{
			break;
		}
	}
	++MutableNativeQuestLogPaintCount;

	if (bInventoryPanelOpen || InventorySlots.Num() > 0)
	{
		const float InvWidth = 210.0f;
		const float InvHeight = bInventoryPanelOpen ? 176.0f : 52.0f;
		const FVector2f InvPos(24.0f, LocalSize.Y - InvHeight - 24.0f);
		PaintSoftPanel(OutDrawElements, AllottedGeometry, CurrentLayer, InvPos, V2f(InvWidth, InvHeight), FLinearColor(0.09f, 0.07f, 0.13f, bInventoryPanelOpen ? 0.92f : 0.78f), FLinearColor(0.52f, 0.78f, 1.0f, 0.72f), 1.6f);
		PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, InvPos + V2f(12.0f, 8.0f), bInventoryPanelOpen ? TEXT("INVENTORY (I to close)") : TEXT("INVENTORY (I)"), 11, FLinearColor(0.74f, 0.88f, 1.0f, 0.92f), MelodiaHUDFonts::ERole::Bold, 200.0f);

		if (bInventoryPanelOpen)
		{
			int32 SlotIndex = 0;
			for (const FMelodiaInventorySlot& InvSlot : InventorySlots)
			{
				const int32 Row = SlotIndex / 4;
				const int32 Col = SlotIndex % 4;
				const FVector2f SlotPos = InvPos + V2f(12.0f + static_cast<float>(Col) * 48.0f, 34.0f + static_cast<float>(Row) * 48.0f);
				const FSlateBrush SlotBrush = MakeTintBrush(InvSlot.IconTint);
				FSlateDrawElement::MakeBox(OutDrawElements, ++CurrentLayer, PaintBox(AllottedGeometry, SlotPos, V2f(40.0f, 40.0f)), &SlotBrush, ESlateDrawEffect::None, InvSlot.IconTint);
				PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, SlotPos + V2f(2.0f, 24.0f), FString::Printf(TEXT("%d"), InvSlot.Quantity), 10, FLinearColor(0.06f, 0.04f, 0.08f, 0.95f));
				++SlotIndex;
				if (SlotIndex >= 8)
				{
					break;
				}
			}
		}
		else if (InventorySlots.Num() > 0)
		{
			PaintLabel(OutDrawElements, AllottedGeometry, ++CurrentLayer, InvPos + V2f(12.0f, 28.0f),
				FString::Printf(TEXT("%d items"), InventorySlots.Num()), 11, FLinearColor(0.82f, 0.74f, 0.96f, 0.92f));
		}
		++MutableNativeInventoryPaintCount;
	}
}
