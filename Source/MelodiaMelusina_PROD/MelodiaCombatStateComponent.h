// Native reactive combat state for Melodia battle actors.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaCombatStateComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaCombatStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaCombatStateComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float UltimateGauge = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float UltimateMax = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bUltimateReady = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	float LastUltimateDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 UltimateActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 TotalUltimateActivationCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	int32 SkillPoints = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	int32 SkillPointMax = 5;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 LastSkillPointDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 BasicActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 SkillActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 CommandSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 ActiveTurnOrderIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	FString LastCommandName;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	FString EnemyIntentName = TEXT("Waiting");

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	float EnemyIntentPower = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bUltimateInterruptWindow = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 UltimateInterruptCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float EnemyToughness = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float EnemyToughnessMax = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	float LastToughnessDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bEnemyBroken = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 EnemyBreakCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 TotalEnemyBreakCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bBreakFollowUpAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bBreakFollowUpConsumed = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	float LastFollowUpBonusDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 BreakFollowUpAvailableCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 BreakFollowUpConsumedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 TotalBreakFollowUpConsumedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 EnemyTurnDelayStacks = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 LastEnemyTurnDelay = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 EnemyTurnDelayApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 TotalEnemyTurnDelayApplyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float PartyHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Combat")
	float PartyMaxHP = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	float LastPartyDamageTaken = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	int32 EnemyTurnCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Combat")
	bool bUltimateUsedThisBattle = false;

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	float ApplyPartyDamage(float Damage);
	float AddUltimateGauge(float Delta);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void ResetUltimateGauge();

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void RecordUltimateActivated(float Damage);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	int32 AddSkillPoints(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	bool SpendSkillPoints(int32 Cost);

	UFUNCTION(BlueprintPure, Category="Melodia|Combat")
	bool CanSpendSkillPoints(int32 Cost) const;

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void ResetActionEconomy();

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void RecordCommandState(const FString& CommandName, const FString& IntentName, float IntentPower, bool bUltimateWindow, bool bUltimateInterrupt);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	bool ApplyEnemyToughnessDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void OpenBreakFollowUpWindow();

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	bool ConsumeBreakFollowUp(float BonusDamage);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	int32 ApplyEnemyTurnDelay(int32 DelayAmount);

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	int32 ConsumeEnemyTurnDelay();

	UFUNCTION(BlueprintCallable, Category="Melodia|Combat")
	void ResetEnemyToughness();
};
