#include "MelodiaBattleBridgeLibrary.h"

#include "MelodiaBattleSession.h"

UMelodiaBattleSession* UMelodiaBattleBridgeLibrary::GetBattleSession(const UObject* WorldContextObject)
{
	return UMelodiaBattleSession::Get(WorldContextObject);
}

bool UMelodiaBattleBridgeLibrary::IsMelodiaBattleActive(const UObject* WorldContextObject)
{
	const UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->IsEncounterActive();
}

bool UMelodiaBattleBridgeLibrary::PhoenixSubmitAttack(const UObject* WorldContextObject)
{
	UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->SubmitBasicCommand();
}

bool UMelodiaBattleBridgeLibrary::PhoenixSubmitSkill(const UObject* WorldContextObject, const FName SkillId)
{
	UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->SubmitSkillCommand(SkillId);
}

bool UMelodiaBattleBridgeLibrary::PhoenixSubmitUltimate(const UObject* WorldContextObject)
{
	UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->SubmitUltimateCommand();
}

bool UMelodiaBattleBridgeLibrary::PhoenixSubmitFlee(const UObject* WorldContextObject)
{
	UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->SubmitFleeCommand();
}

bool UMelodiaBattleBridgeLibrary::PhoenixConfirmVictory(const UObject* WorldContextObject)
{
	UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->ConfirmVictoryReward();
}

TArray<FName> UMelodiaBattleBridgeLibrary::PhoenixGetUnlockedSkillIds(const UObject* WorldContextObject)
{
	const UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session ? Session->GetUnlockedSkillIds() : TArray<FName>();
}

TArray<FMelodiaSongSkillRecipe> UMelodiaBattleBridgeLibrary::PhoenixGetUnlockedSkills(const UObject* WorldContextObject)
{
	const UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session ? Session->GetUnlockedSkills() : TArray<FMelodiaSongSkillRecipe>();
}

bool UMelodiaBattleBridgeLibrary::PhoenixCanUseSkill(const UObject* WorldContextObject, const FName SkillId)
{
	const UMelodiaBattleSession* Session = GetBattleSession(WorldContextObject);
	return Session && Session->CanSubmitSkillCommand(SkillId);
}

FName UMelodiaBattleBridgeLibrary::PhoenixResolveSkillIdFromMenuIndex(const UObject* WorldContextObject, const int32 MenuIndex)
{
	const TArray<FName> Unlocked = PhoenixGetUnlockedSkillIds(WorldContextObject);
	return Unlocked.IsValidIndex(MenuIndex) ? Unlocked[MenuIndex] : NAME_None;
}

bool UMelodiaBattleBridgeLibrary::PhoenixSubmitSkillByMenuIndex(const UObject* WorldContextObject, const int32 MenuIndex)
{
	const FName SkillId = PhoenixResolveSkillIdFromMenuIndex(WorldContextObject, MenuIndex);
	if (SkillId.IsNone())
	{
		return false;
	}

	return PhoenixSubmitSkill(WorldContextObject, SkillId);
}
