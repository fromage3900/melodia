// Native reactive combat state for Melodia battle actors.

#include "MelodiaCombatStateComponent.h"

UMelodiaCombatStateComponent::UMelodiaCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UMelodiaCombatStateComponent::AddUltimateGauge(const float Delta)
{
	const float SafeMax = FMath::Max(1.0f, UltimateMax);
	UltimateMax = SafeMax;
	UltimateGauge = FMath::Clamp(UltimateGauge + FMath::Max(0.0f, Delta), 0.0f, SafeMax);
	bUltimateReady = UltimateGauge >= SafeMax;
	return UltimateGauge;
}

void UMelodiaCombatStateComponent::ResetUltimateGauge()
{
	UltimateGauge = 0.0f;
	LastUltimateDamage = 0.0f;
	UltimateActivationCount = 0;
	bUltimateReady = false;
}

void UMelodiaCombatStateComponent::RecordUltimateActivated(const float Damage)
{
	LastUltimateDamage = FMath::Max(0.0f, Damage);
	UltimateGauge = 0.0f;
	bUltimateReady = false;
	++UltimateActivationCount;
	++TotalUltimateActivationCount;
}

int32 UMelodiaCombatStateComponent::AddSkillPoints(const int32 Delta)
{
	const int32 SafeMax = FMath::Max(1, SkillPointMax);
	SkillPointMax = SafeMax;
	const int32 PreviousSkillPoints = FMath::Clamp(SkillPoints, 0, SafeMax);
	SkillPoints = FMath::Clamp(PreviousSkillPoints + Delta, 0, SafeMax);
	LastSkillPointDelta = SkillPoints - PreviousSkillPoints;
	if (Delta > 0)
	{
		++BasicActivationCount;
	}
	return SkillPoints;
}

bool UMelodiaCombatStateComponent::SpendSkillPoints(const int32 Cost)
{
	const int32 SafeCost = FMath::Max(0, Cost);
	const int32 SafeMax = FMath::Max(1, SkillPointMax);
	SkillPointMax = SafeMax;
	SkillPoints = FMath::Clamp(SkillPoints, 0, SafeMax);
	if (SkillPoints < SafeCost)
	{
		LastSkillPointDelta = 0;
		return false;
	}

	SkillPoints -= SafeCost;
	LastSkillPointDelta = -SafeCost;
	if (SafeCost > 0)
	{
		++SkillActivationCount;
	}
	return true;
}

bool UMelodiaCombatStateComponent::CanSpendSkillPoints(const int32 Cost) const
{
	return FMath::Clamp(SkillPoints, 0, FMath::Max(1, SkillPointMax)) >= FMath::Max(0, Cost);
}

void UMelodiaCombatStateComponent::ResetActionEconomy()
{
	SkillPointMax = FMath::Max(1, SkillPointMax);
	SkillPoints = FMath::Clamp(3, 0, SkillPointMax);
	LastSkillPointDelta = 0;
	BasicActivationCount = 0;
	SkillActivationCount = 0;
	CommandSequence = 0;
	ActiveTurnOrderIndex = 0;
	LastCommandName = TEXT("Battle Start");
	EnemyIntentName = TEXT("Waiting");
	EnemyIntentPower = 0.0f;
	bUltimateInterruptWindow = false;
	UltimateInterruptCount = 0;
	bBreakFollowUpAvailable = false;
	bBreakFollowUpConsumed = false;
	LastFollowUpBonusDamage = 0.0f;
	BreakFollowUpAvailableCount = 0;
	BreakFollowUpConsumedCount = 0;
	EnemyTurnDelayStacks = 0;
	LastEnemyTurnDelay = 0;
	EnemyTurnDelayApplyCount = 0;
	ResetEnemyToughness();
}

void UMelodiaCombatStateComponent::RecordCommandState(const FString& CommandName, const FString& IntentName, const float IntentPower, const bool bUltimateWindow, const bool bUltimateInterrupt)
{
	LastCommandName = CommandName.IsEmpty() ? TEXT("Command") : CommandName;
	EnemyIntentName = IntentName.IsEmpty() ? TEXT("Waiting") : IntentName;
	EnemyIntentPower = FMath::Max(0.0f, IntentPower);
	bUltimateInterruptWindow = bUltimateWindow;
	++CommandSequence;
	ActiveTurnOrderIndex = EnemyTurnDelayStacks > 0 ? 0 : CommandSequence % 4;
	if (bUltimateInterrupt)
	{
		++UltimateInterruptCount;
	}
}

bool UMelodiaCombatStateComponent::ApplyEnemyToughnessDamage(const float Damage)
{
	const float SafeMax = FMath::Max(1.0f, EnemyToughnessMax);
	EnemyToughnessMax = SafeMax;
	const float PreviousToughness = FMath::Clamp(EnemyToughness, 0.0f, SafeMax);
	const bool bWasBroken = PreviousToughness <= 0.0f || bEnemyBroken;
	LastToughnessDamage = FMath::Max(0.0f, Damage);
	EnemyToughness = FMath::Clamp(PreviousToughness - LastToughnessDamage, 0.0f, SafeMax);
	bEnemyBroken = EnemyToughness <= 0.0f;

	if (bEnemyBroken && !bWasBroken)
	{
		++EnemyBreakCount;
		++TotalEnemyBreakCount;
		OpenBreakFollowUpWindow();
		return true;
	}

	return false;
}

void UMelodiaCombatStateComponent::OpenBreakFollowUpWindow()
{
	bBreakFollowUpAvailable = true;
	bBreakFollowUpConsumed = false;
	LastFollowUpBonusDamage = 0.0f;
	++BreakFollowUpAvailableCount;
}

bool UMelodiaCombatStateComponent::ConsumeBreakFollowUp(const float BonusDamage)
{
	if (!bBreakFollowUpAvailable || bBreakFollowUpConsumed)
	{
		LastFollowUpBonusDamage = 0.0f;
		return false;
	}

	bBreakFollowUpAvailable = false;
	bBreakFollowUpConsumed = true;
	LastFollowUpBonusDamage = FMath::Max(0.0f, BonusDamage);
	++BreakFollowUpConsumedCount;
	++TotalBreakFollowUpConsumedCount;
	return true;
}

int32 UMelodiaCombatStateComponent::ApplyEnemyTurnDelay(const int32 DelayAmount)
{
	const int32 SafeDelay = FMath::Clamp(DelayAmount, 0, 4);
	LastEnemyTurnDelay = SafeDelay;
	if (SafeDelay <= 0)
	{
		return EnemyTurnDelayStacks;
	}

	EnemyTurnDelayStacks = FMath::Clamp(EnemyTurnDelayStacks + SafeDelay, 0, 6);
	++EnemyTurnDelayApplyCount;
	++TotalEnemyTurnDelayApplyCount;
	return EnemyTurnDelayStacks;
}

int32 UMelodiaCombatStateComponent::ConsumeEnemyTurnDelay()
{
	if (EnemyTurnDelayStacks > 0)
	{
		--EnemyTurnDelayStacks;
	}
	return EnemyTurnDelayStacks;
}

void UMelodiaCombatStateComponent::ResetEnemyToughness()
{
	EnemyToughnessMax = FMath::Max(1.0f, EnemyToughnessMax);
	EnemyToughness = EnemyToughnessMax;
	LastToughnessDamage = 0.0f;
	bEnemyBroken = false;
	EnemyBreakCount = 0;
	bBreakFollowUpAvailable = false;
	bBreakFollowUpConsumed = false;
	LastFollowUpBonusDamage = 0.0f;
	EnemyTurnDelayStacks = 0;
	LastEnemyTurnDelay = 0;
}
