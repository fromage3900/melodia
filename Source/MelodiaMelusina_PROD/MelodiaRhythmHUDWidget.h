// Native base for rhythm HUD widgets so Blueprint callers have stable hooks.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MelodiaRhythmHUDWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API UMelodiaRhythmHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	bool bDrawNativeCuteCombatHUD = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor PanelTint = FLinearColor(0.11f, 0.08f, 0.16f, 0.84f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor FiligreeTint = FLinearColor(1.0f, 0.82f, 0.38f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor SparkleTint = FLinearColor(0.86f, 0.72f, 1.0f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm HUD|Style")
	FLinearColor UltimateGaugeTint = FLinearColor(0.98f, 0.54f, 0.94f, 0.95f);

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
	float LastSparkleBurstTime = -1000.0f;

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

	float GetHUDTimeSeconds() const;
	void SyncMutablePaintStats() const;
};
