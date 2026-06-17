#include "MelodiaCombatSyncLibrary.h"

#include "GameFramework/Actor.h"
#include "MelodiaCombatStateComponent.h"
#include "UObject/UnrealType.h"

namespace
{
void SetFloatProperty(AActor* Actor, const FName PropertyName, const float Value)
{
	if (!Actor)
	{
		return;
	}

	if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
	{
		FloatProperty->SetPropertyValue_InContainer(Actor, Value);
	}
}

void SetIntProperty(AActor* Actor, const FName PropertyName, const int32 Value)
{
	if (!Actor)
	{
		return;
	}

	if (FIntProperty* IntProperty = CastField<FIntProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
	{
		IntProperty->SetPropertyValue_InContainer(Actor, Value);
	}
}

void SetBoolProperty(AActor* Actor, const FName PropertyName, const bool bValue)
{
	if (!Actor)
	{
		return;
	}

	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
	{
		BoolProperty->SetPropertyValue_InContainer(Actor, bValue);
	}
}
}

void UMelodiaCombatSyncLibrary::MirrorCombatStateToLegacyProperties(
	AActor* BattleController,
	const UMelodiaCombatStateComponent* CombatState,
	FMelodiaCombatLegacyOverrides Overrides)
{
	if (!BattleController || !CombatState)
	{
		return;
	}

	const float UltimateMax = FMath::Max(1.0f, CombatState->UltimateMax);
	const float UltimateGauge = FMath::Clamp(CombatState->UltimateGauge, 0.0f, UltimateMax);
	const bool bUltimateReady = UltimateGauge >= UltimateMax;

	SetIntProperty(BattleController, TEXT("RhythmSkillPoints"), CombatState->SkillPoints);
	SetIntProperty(BattleController, TEXT("RhythmSkillPointMax"), CombatState->SkillPointMax);
	SetIntProperty(BattleController, TEXT("RhythmLastSkillPointDelta"), CombatState->LastSkillPointDelta);
	SetIntProperty(BattleController, TEXT("RhythmBasicActivationCount"), CombatState->BasicActivationCount);
	SetIntProperty(BattleController, TEXT("RhythmSkillActivationCount"), CombatState->SkillActivationCount);

	SetFloatProperty(BattleController, TEXT("RhythmUltimateGauge"), UltimateGauge);
	SetFloatProperty(BattleController, TEXT("RhythmUltimateMax"), UltimateMax);
	SetBoolProperty(BattleController, TEXT("bRhythmUltimateReady"), bUltimateReady);
	SetIntProperty(BattleController, TEXT("RhythmUltimateActivationCount"), CombatState->UltimateActivationCount);
	SetIntProperty(BattleController, TEXT("RhythmTotalEnemyBreakCount"), CombatState->TotalEnemyBreakCount);

	SetFloatProperty(BattleController, TEXT("RhythmPartyHP"), CombatState->PartyHP);
	SetFloatProperty(BattleController, TEXT("RhythmPartyMaxHP"), CombatState->PartyMaxHP);

	SetFloatProperty(BattleController, TEXT("RhythmEnemyToughness"), CombatState->EnemyToughness);
	SetFloatProperty(BattleController, TEXT("RhythmEnemyToughnessMax"), FMath::Max(1.0f, CombatState->EnemyToughnessMax));
	SetBoolProperty(BattleController, TEXT("bRhythmEnemyBroken"), CombatState->bEnemyBroken);
	SetBoolProperty(BattleController, TEXT("bRhythmBreakFollowUpAvailable"), CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed);
	SetBoolProperty(BattleController, TEXT("bRhythmBreakFollowUpConsumed"), CombatState->bBreakFollowUpConsumed);
	SetIntProperty(BattleController, TEXT("RhythmEnemyBreakCount"), CombatState->EnemyBreakCount);
	SetIntProperty(BattleController, TEXT("RhythmBreakFollowUpAvailableCount"), CombatState->BreakFollowUpAvailableCount);
	SetIntProperty(BattleController, TEXT("RhythmBreakFollowUpConsumedCount"), CombatState->BreakFollowUpConsumedCount);
	SetIntProperty(BattleController, TEXT("RhythmEnemyTurnDelayStacks"), CombatState->EnemyTurnDelayStacks);
	SetIntProperty(BattleController, TEXT("RhythmLastEnemyTurnDelay"), CombatState->LastEnemyTurnDelay);
	SetIntProperty(BattleController, TEXT("RhythmEnemyTurnDelayApplyCount"), CombatState->EnemyTurnDelayApplyCount);

	SetIntProperty(BattleController, TEXT("RhythmCommandSequence"), CombatState->CommandSequence);
	SetIntProperty(BattleController, TEXT("RhythmActiveTurnOrderIndex"), CombatState->ActiveTurnOrderIndex);
	SetIntProperty(BattleController, TEXT("RhythmUltimateInterruptCount"), CombatState->UltimateInterruptCount);
	SetFloatProperty(BattleController, TEXT("RhythmEnemyIntentPower"), CombatState->EnemyIntentPower);
	SetBoolProperty(BattleController, TEXT("bRhythmUltimateInterruptWindow"), CombatState->bUltimateInterruptWindow);

	if (Overrides.bOverrideLastDamage)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastDamage"), Overrides.LastDamage);
	}
	else if (CombatState->LastUltimateDamage > 0.0f)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastDamage"), CombatState->LastUltimateDamage);
	}

	if (Overrides.bOverrideLastUltimateDamage)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastUltimateDamage"), Overrides.LastUltimateDamage);
	}

	if (Overrides.bOverrideLastToughnessDamage)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastToughnessDamage"), Overrides.LastToughnessDamage);
	}
	else
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastToughnessDamage"), CombatState->LastToughnessDamage);
	}

	if (Overrides.bOverrideLastFollowUpBonusDamage)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), Overrides.LastFollowUpBonusDamage);
	}
	else
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), CombatState->LastFollowUpBonusDamage);
	}

	if (Overrides.bOverrideLastPartyDamage)
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastPartyDamage"), Overrides.LastPartyDamage);
	}
	else
	{
		SetFloatProperty(BattleController, TEXT("RhythmLastPartyDamage"), CombatState->LastPartyDamageTaken);
	}

	if (Overrides.bOverrideEnemyMaxHP)
	{
		SetFloatProperty(BattleController, TEXT("RhythmEnemyMaxHP"), Overrides.EnemyMaxHP);
	}

	if (Overrides.bOverrideEnemyHP)
	{
		SetFloatProperty(BattleController, TEXT("RhythmEnemyHP"), Overrides.EnemyHP);
	}
}
