#include "MelodiaPartySubsystem.h"

#include "MelodiaCombatStateComponent.h"
#include "MelodiaMechanicProgressionSubsystem.h"

void UMelodiaPartySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetToDefaults();
}

UMelodiaPartySubsystem* UMelodiaPartySubsystem::Get(const UObject* WorldContextObject)
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

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UMelodiaPartySubsystem>() : nullptr;
}

void UMelodiaPartySubsystem::ResetToDefaults()
{
	PartySlots.Reset();

	FMelodiaPartySlot Leader;
	Leader.SlotIndex = 0;
	Leader.MemberId = TEXT("Melusina");
	Leader.DisplayName = INVTEXT("Melusina");
	Leader.bActiveInBattle = true;
	PartySlots.Add(Leader);

	for (int32 Slot = 1; Slot < MaxPartySlots; ++Slot)
	{
		FMelodiaPartySlot Empty;
		Empty.SlotIndex = Slot;
		Empty.bActiveInBattle = false;
		PartySlots.Add(Empty);
	}
}

void UMelodiaPartySubsystem::SyncFromProgression()
{
	if (!GetGameInstance())
	{
		return;
	}

	if (const UMelodiaMechanicProgressionSubsystem* Progression = GetGameInstance()->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
	{
		SetCompanionInParty(Progression->State.bCompanionUnlocked, INVTEXT("Cockatoo"));
	}
}

void UMelodiaPartySubsystem::SetCompanionInParty(const bool bInParty, const FText& DisplayName)
{
	if (PartySlots.Num() < 2)
	{
		ResetToDefaults();
	}

	PartySlots[1].MemberId = TEXT("CockatooCompanion");
	PartySlots[1].DisplayName = DisplayName;
	PartySlots[1].bActiveInBattle = bInParty;
	PartySlots[1].BonusSkillPointsAtBattleStart = bInParty ? 1 : 0;
}

int32 UMelodiaPartySubsystem::GetTotalBonusSkillPointsAtBattleStart() const
{
	int32 Total = 0;
	for (const FMelodiaPartySlot& Slot : PartySlots)
	{
		if (Slot.bActiveInBattle)
		{
			Total += Slot.BonusSkillPointsAtBattleStart;
		}
	}
	return Total;
}

TArray<FMelodiaPartySlot> UMelodiaPartySubsystem::GetActiveBattleMembers() const
{
	TArray<FMelodiaPartySlot> Active;
	for (const FMelodiaPartySlot& Slot : PartySlots)
	{
		if (Slot.bActiveInBattle && !Slot.MemberId.IsNone())
		{
			Active.Add(Slot);
		}
	}
	return Active;
}

void UMelodiaPartySubsystem::ApplyBattleStartBuffs(UMelodiaCombatStateComponent* CombatState) const
{
	if (!CombatState)
	{
		return;
	}

	const int32 BonusSP = GetTotalBonusSkillPointsAtBattleStart();
	if (BonusSP <= 0)
	{
		return;
	}

	CombatState->SkillPoints = FMath::Clamp(CombatState->SkillPoints + BonusSP, 0, CombatState->SkillPointMax);
	CombatState->bCompanionActive = GetActiveBattleMembers().Num() > 1;
}
