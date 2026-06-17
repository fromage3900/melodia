// Native base for rhythm HUD widgets so Blueprint callers have stable hooks.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MelodiaQuestTypes.h"
#include "MelodiaBattleTypes.h"
#include "MelodiaRhythmExecutionComponent.h"
#include "MelodiaRhythmHUDWidget.generated.h"

class UTextBlock;
class UWidget;

// Single floating combat-text entry (damage popup) tracked for native painting.
USTRUCT(BlueprintType)
struct FMelodiaFloatingCombatText
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float SpawnTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bAnchorEnemy = true;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LateralSeed = 0.0f;
};

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API UMelodiaRhythmHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Find the first UMelodiaRhythmHUDWidget instance in the world.
	 * Replaces the repeated TObjectIterator pattern scattered across 9+ files.
	 */
	UFUNCTION(BlueprintCallable, Category = "Melodia|HUD", meta = (WorldContext = "WorldContextObject"))
	static UMelodiaRhythmHUDWidget* FindFirst(const UObject* WorldContextObject);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void DoPulse();
	virtual void DoPulse_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetJudgment(const FText& NewText);
	virtual void SetJudgment_Implementation(const FText& NewText);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetJudgmentString(const FString& NewText);
	virtual void SetJudgmentString_Implementation(const FString& NewText);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ShowBattleStatus(const FString& NewStatusText);
	virtual void ShowBattleStatus_Implementation(const FString& NewStatusText);

	UFUNCTION(BlueprintCallable, Category="Melodia|Rhythm HUD")
	void SetHUDMode(EMelodiaHUDMode NewMode);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ShowVictoryReward(const FString& NewRewardText);
	virtual void ShowVictoryReward_Implementation(const FString& NewRewardText);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ClearBattleStatus();
	virtual void ClearBattleStatus_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetUltimateGauge(float CurrentValue, float MaxValue, bool bReady);
	virtual void SetUltimateGauge_Implementation(float CurrentValue, float MaxValue, bool bReady);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetSkillPoints(int32 CurrentValue, int32 MaxValue);
	virtual void SetSkillPoints_Implementation(int32 CurrentValue, int32 MaxValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ShowUltimateReady();
	virtual void ShowUltimateReady_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ShowUltimateActivated(float DamageValue);
	virtual void ShowUltimateActivated_Implementation(float DamageValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ApplyCuteCombatTheme();
	virtual void ApplyCuteCombatTheme_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void TriggerSparkleBurst();
	virtual void TriggerSparkleBurst_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetEnemyVitals(float CurrentHP, float MaxHP);
	virtual void SetEnemyVitals_Implementation(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetEnemyBreakGauge(float CurrentValue, float MaxValue, bool bBroken);
	virtual void SetEnemyBreakGauge_Implementation(float CurrentValue, float MaxValue, bool bBroken);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetBreakFollowUpWindow(bool bAvailable, bool bConsumed, float BonusDamage);
	virtual void SetBreakFollowUpWindow_Implementation(bool bAvailable, bool bConsumed, float BonusDamage);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetEnemyTurnDelay(int32 DelayStacks, int32 LastDelayApplied);
	virtual void SetEnemyTurnDelay_Implementation(int32 DelayStacks, int32 LastDelayApplied);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetTurnOrderPreview(const TArray<FString>& UnitNames, int32 ActiveIndex);
	virtual void SetTurnOrderPreview_Implementation(const TArray<FString>& UnitNames, int32 ActiveIndex);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetReactiveBattleState(const FString& CommandName, const FString& EnemyIntentName, float EnemyIntentPower, bool bUltimateWindow, bool bInterrupted);
	virtual void SetReactiveBattleState_Implementation(const FString& CommandName, const FString& EnemyIntentName, float EnemyIntentPower, bool bUltimateWindow, bool bInterrupted);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void ShowActionPrompt(const FString& PromptText);
	virtual void ShowActionPrompt_Implementation(const FString& PromptText);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void TriggerDamageFlash(float DamageValue);
	virtual void TriggerDamageFlash_Implementation(float DamageValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetPartyVitals(float CurrentHP, float MaxHP);
	virtual void SetPartyVitals_Implementation(float CurrentHP, float MaxHP);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetNoteHighwayActive(bool bActive, const TArray<FMelodiaHighwayNote>& Notes, float BeatPosition, float ScrollBeatsAhead);
	virtual void SetNoteHighwayActive_Implementation(bool bActive, const TArray<FMelodiaHighwayNote>& Notes, float BeatPosition, float ScrollBeatsAhead);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void SetBattlePhaseBanner(const FString& PhaseLabel);
	virtual void SetBattlePhaseBanner_Implementation(const FString& PhaseLabel);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Rhythm HUD")
	void PushFloatingCombatText(const FString& Text, bool bAnchorEnemy, FLinearColor Tint);
	virtual void PushFloatingCombatText_Implementation(const FString& Text, bool bAnchorEnemy, FLinearColor Tint);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetQuestLogEntries(const TArray<FString>& Entries, const FString& ToastText);
	virtual void SetQuestLogEntries_Implementation(const TArray<FString>& Entries, const FString& ToastText);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetInventorySlots(const TArray<FMelodiaInventorySlot>& Slots);
	virtual void SetInventorySlots_Implementation(const TArray<FMelodiaInventorySlot>& Slots);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetInventoryPanelOpen(bool bOpen);
	virtual void SetInventoryPanelOpen_Implementation(bool bOpen);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetMinimapState(FVector MapCenterWorld, float MapHalfExtentCm, FVector PlayerWorldLocation, const TArray<FMelodiaMinimapMarker>& Markers);
	virtual void SetMinimapState_Implementation(FVector MapCenterWorld, float MapHalfExtentCm, FVector PlayerWorldLocation, const TArray<FMelodiaMinimapMarker>& Markers);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetMinimapMarkers(const TArray<FMelodiaMinimapMarker>& Markers);
	virtual void SetMinimapMarkers_Implementation(const TArray<FMelodiaMinimapMarker>& Markers);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Melodia|Exploration HUD")
	void SetMechanicProgression(int32 MechanicLevel, int32 CurrentXP, int32 XPToNextLevel, const FString& TierDisplayName, int32 UnlockedPresetCount);
	virtual void SetMechanicProgression_Implementation(int32 MechanicLevel, int32 CurrentXP, int32 XPToNextLevel, const FString& TierDisplayName, int32 UnlockedPresetCount);

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 LastMechanicLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 LastMechanicXP = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 LastMechanicXPToNext = 85;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	FString LastMechanicTierName = TEXT("Novice Star");

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 UnlockedLocationPresetCount = 1;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 MechanicProgressUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastBattleStatusText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastRewardText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bVictoryFeedbackVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 ExplorationPromptCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 BattleStartPromptCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastUltimateGaugeValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastUltimateGaugeMax = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bUltimateReadyVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 UltimateReadyPromptCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 UltimateActivationPromptCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 LastSkillPoints = 3;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 LastSkillPointMax = 5;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 SkillPointUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 SparkleBurstCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bCuteCombatThemeApplied = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastEnemyHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastEnemyMaxHP = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 EnemyVitalsUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastEnemyToughness = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastEnemyToughnessMax = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bEnemyBreakVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 EnemyBreakGaugeUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bBreakFollowUpAvailableVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bBreakFollowUpConsumedVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastFollowUpBonusDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 BreakFollowUpUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 LastEnemyTurnDelayStacks = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 LastEnemyTurnDelayApplied = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 EnemyTurnDelayUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	TArray<FString> LastTurnOrderPreview;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 ActiveTurnOrderIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 TurnOrderUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastCommandName;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastEnemyIntentName = TEXT("Waiting");

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastEnemyIntentPower = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bLastUltimateWindow = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bLastUltimateInterrupted = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 ReactiveStateUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastActionPromptText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 ActionPromptCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 DamageFlashCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastDamageFlashValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastDamageFlashTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastTurnBannerText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastTurnBannerTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FLinearColor LastTurnBannerTint = FLinearColor(1.0f, 0.86f, 0.42f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 TurnBannerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastFloatingDamageValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastFloatingDamageTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bLastFloatingDamageAgainstEnemy = true;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FLinearColor LastFloatingDamageTint = FLinearColor(1.0f, 0.95f, 0.6f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 FloatingDamageCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	FString LastBattlePhaseLabel;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastBattlePhaseTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 BattlePhaseUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	TArray<FMelodiaFloatingCombatText> FloatingCombatTexts;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 FloatingCombatTextCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastPartyHP = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float LastPartyMaxHP = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 PartyVitalsUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	bool bNoteHighwayActive = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	TArray<FMelodiaHighwayNote> HighwayNotes;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float HighwayBeatPosition = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	float HighwayScrollBeatsAhead = 2.5f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	int32 NoteHighwayUpdateCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Exploration HUD")
	bool bDrawExplorationHUD = true;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD")
	EMelodiaHUDMode ActiveHUDMode = EMelodiaHUDMode::Exploration;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	TArray<FString> QuestLogEntries;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	FString LastQuestToastText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	TArray<FMelodiaInventorySlot> InventorySlots;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	bool bInventoryPanelOpen = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	FVector MinimapCenterWorld = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	float MinimapHalfExtentCm = 800.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	FVector MinimapPlayerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	TArray<FMelodiaMinimapMarker> MinimapMarkers;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 QuestLogUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 InventoryUpdateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Exploration HUD")
	int32 MinimapUpdateCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	bool bDrawNativeCuteCombatHUD = true;

	/** When true, battle HUD shows only enemy HP, party HP, action prompt, and note highway (no turn rail / command cards / sparkles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	bool bCompactBattleHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor PanelTint = FLinearColor(0.09f, 0.07f, 0.14f, 0.90f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor FiligreeTint = FLinearColor(0.98f, 0.82f, 0.38f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor SparkleTint = FLinearColor(0.86f, 0.74f, 1.0f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor UltimateGaugeTint = FLinearColor(0.62f, 0.48f, 0.98f, 0.95f);

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativePaintFrameCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeFiligreePaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeSparklePaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeTurnRailPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeCommandCardPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeSkillPointPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeEnemyVitalsPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeIntentPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeBreakGaugePaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeFollowUpPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeDamageFlashPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeLabelPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativePortraitPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeHighwayPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativePartyVitalsPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeMinimapPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeQuestLogPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeInventoryPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	float LastSparkleBurstTime = -1000.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeBannerPaintCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm HUD|Style")
	int32 NativeFloatingTextPaintCount = 0;

	// Smoothed (lerped) bar values driven in NativeTick so gauges drain instead of snapping.
	UPROPERTY(BlueprintReadOnly, Transient, Category="Melodia|Rhythm HUD|Style")
	float DisplayedEnemyHP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Melodia|Rhythm HUD|Style")
	float DisplayedPartyHP = 100.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Melodia|Rhythm HUD|Style")
	float DisplayedUltimate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Melodia|Rhythm HUD|Style")
	float DisplayedEnemyToughness = 100.0f;

	UPROPERTY(Transient)
	bool bDisplayedValuesInitialized = false;

protected:
	UWidget* FindWidgetByName(FName WidgetName) const;
	UTextBlock* FindTextBlockByName(FName WidgetName) const;

private:
	mutable int32 MutableNativePaintFrameCount = 0;
	mutable int32 MutableNativeFiligreePaintCount = 0;
	mutable int32 MutableNativeSparklePaintCount = 0;
	mutable int32 MutableNativeTurnRailPaintCount = 0;
	mutable int32 MutableNativeCommandCardPaintCount = 0;
	mutable int32 MutableNativeSkillPointPaintCount = 0;
	mutable int32 MutableNativeEnemyVitalsPaintCount = 0;
	mutable int32 MutableNativeIntentPaintCount = 0;
	mutable int32 MutableNativeBreakGaugePaintCount = 0;
	mutable int32 MutableNativeFollowUpPaintCount = 0;
	mutable int32 MutableNativeDamageFlashPaintCount = 0;
	mutable int32 MutableNativeLabelPaintCount = 0;
	mutable int32 MutableNativePortraitPaintCount = 0;
	mutable int32 MutableNativeHighwayPaintCount = 0;
	mutable int32 MutableNativePartyVitalsPaintCount = 0;
	mutable int32 MutableNativeMinimapPaintCount = 0;
	mutable int32 MutableNativeQuestLogPaintCount = 0;
	mutable int32 MutableNativeInventoryPaintCount = 0;
	mutable int32 MutableNativeBannerPaintCount = 0;
	mutable int32 MutableNativeFloatingTextPaintCount = 0;

	float GetHUDTimeSeconds() const;
	void UpdateSmoothedBars(float InDeltaTime);
	void SyncExplorationHUDFromWorld();
	FVector2f WorldToMinimap(const FVector2f& LocalSize, const FVector& WorldLocation) const;
	void PaintExplorationHUD(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32& CurrentLayer) const;
	void SyncMutablePaintStats() const;
};
