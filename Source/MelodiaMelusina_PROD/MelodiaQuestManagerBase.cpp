// Native quest manager with starter quests, progress tracking, and HUD hooks.

#include "MelodiaQuestManagerBase.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaKeySystemLibrary.h"
#include "MelodiaMechanicProgressionLibrary.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaRhythmHUDWidget.h"
#include "MelodiaSongSkillLibrary.h"
#include "PCG/MelodiaPCGLibrary.h"
#include "UObject/UObjectIterator.h"

AMelodiaQuestManagerBase::AMelodiaQuestManagerBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMelodiaQuestManagerBase::BeginPlay()
{
	Super::BeginPlay();
}

void AMelodiaQuestManagerBase::RegisterStarterQuests(const FVector& SongGateLocation)
{
	QuestCatalog.Reset();
	QuestProgress.Reset();

	FMelodiaQuestDefinition GateQuest;
	GateQuest.QuestId = TEXT("WhisperAtGate");
	GateQuest.Title = TEXT("Whisper at the Gate");
	GateQuest.Description = TEXT("Follow the minimap star to the glowing song gate.");
	GateQuest.ObjectiveType = EMelodiaQuestObjectiveType::ReachLocation;
	GateQuest.TargetCount = 1;
	GateQuest.WorldMarkerLocation = SongGateLocation;
	GateQuest.RewardText = TEXT("+25G | Map unlocked");
	AddQuestDefinition(GateQuest);

	FMelodiaQuestDefinition SlimeQuest;
	SlimeQuest.QuestId = TEXT("SlimeBallad");
	SlimeQuest.Title = TEXT("Slime Ballad");
	SlimeQuest.Description = TEXT("Win a rhythm battle against the Melody Slime.");
	SlimeQuest.ObjectiveType = EMelodiaQuestObjectiveType::WinBattle;
	SlimeQuest.TargetCount = 1;
	SlimeQuest.RewardText = TEXT("+Gold Stucco Shard");
	AddQuestDefinition(SlimeQuest);

	FMelodiaQuestDefinition StuccoQuest;
	StuccoQuest.QuestId = TEXT("StuccoScraps");
	StuccoQuest.Title = TEXT("Gold Stucco Scraps");
	StuccoQuest.Description = TEXT("Collect 2 stucco shards for Melusina's atelier.");
	StuccoQuest.ObjectiveType = EMelodiaQuestObjectiveType::CollectItem;
	StuccoQuest.TargetItemId = TEXT("GoldStuccoShard");
	StuccoQuest.TargetCount = 2;
	StuccoQuest.RewardText = TEXT("+Reverie Tonic");
	AddQuestDefinition(StuccoQuest);

	FMelodiaQuestDefinition PerfectQuest;
	PerfectQuest.QuestId = TEXT("PerfectRiff");
	PerfectQuest.Title = TEXT("Perfect Riff");
	PerfectQuest.Description = TEXT("Land 3 Perfect grades during one skill highway.");
	PerfectQuest.ObjectiveType = EMelodiaQuestObjectiveType::PerfectNotesInSkill;
	PerfectQuest.TargetCount = 3;
	PerfectQuest.RewardText = TEXT("+Sheet Music");
	AddQuestDefinition(PerfectQuest);

	FMelodiaQuestDefinition BreakQuest;
	BreakQuest.QuestId = TEXT("BreakTheSlime");
	BreakQuest.Title = TEXT("Break the Slime");
	BreakQuest.Description = TEXT("Shatter the Melody Slime's toughness bar in battle.");
	BreakQuest.ObjectiveType = EMelodiaQuestObjectiveType::BreakEnemy;
	BreakQuest.TargetCount = 1;
	BreakQuest.RewardText = TEXT("+Break Charm");
	AddQuestDefinition(BreakQuest);

	FMelodiaQuestDefinition UltimateQuest;
	UltimateQuest.QuestId = TEXT("UltimateCrescendo");
	UltimateQuest.Title = TEXT("Ultimate Crescendo");
	UltimateQuest.Description = TEXT("Finish a battle with your ultimate crescendo.");
	UltimateQuest.ObjectiveType = EMelodiaQuestObjectiveType::WinWithUltimate;
	UltimateQuest.TargetCount = 1;
	UltimateQuest.RewardText = TEXT("+Crescendo Crystal");
	AddQuestDefinition(UltimateQuest);

	FMelodiaQuestDefinition BlossomQuest;
	BlossomQuest.QuestId = TEXT("BlossomGatherer");
	BlossomQuest.Title = TEXT("Blossom Gatherer");
	BlossomQuest.Description = TEXT("Pick 3 Reverie Blossoms in the world (F near flowers).");
	BlossomQuest.ObjectiveType = EMelodiaQuestObjectiveType::CollectItem;
	BlossomQuest.TargetItemId = TEXT("ReverieBlossom");
	BlossomQuest.TargetCount = 3;
	BlossomQuest.RewardText = TEXT("+Garden Shears cosmetic token");
	AddQuestDefinition(BlossomQuest);

	ActivateQuest(TEXT("WhisperAtGate"));
	ActivateQuest(TEXT("SlimeBallad"));
	ActivateQuest(TEXT("StuccoScraps"));
	ActivateQuest(TEXT("PerfectRiff"));
	ActivateQuest(TEXT("BreakTheSlime"));
	ActivateQuest(TEXT("UltimateCrescendo"));
	ActivateQuest(TEXT("BlossomGatherer"));

	RegisterMechanicDemoQuests();
	RegisterProgressionQuestChain();

	LastQuestToast = TEXT("New quests added to your journal");
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::RegisterMechanicDemoQuests()
{
	auto AddLevelQuest = [this](const FName QuestId, const int32 TargetLevel, const FString& Title, const FString& Description, const int32 ReqLevel, const FString& Reward)
	{
		FMelodiaQuestDefinition Def;
		Def.QuestId = QuestId;
		Def.Title = Title;
		Def.Description = Description;
		Def.ObjectiveType = EMelodiaQuestObjectiveType::ReachMechanicLevel;
		Def.TargetCount = TargetLevel;
		Def.RequiredMechanicLevel = ReqLevel;
		Def.RewardText = Reward;
		AddQuestDefinition(Def);
	};

	AddLevelQuest(TEXT("RisingMelody"), 5, TEXT("Rising Melody"), TEXT("Reach Mechanic Level 5 to unlock Tier I presets."), 1, TEXT("+Location Preset Lv5"));
	AddLevelQuest(TEXT("MoonApprentice"), 10, TEXT("Moon Apprentice"), TEXT("Reach Mechanic Level 10 — Tier II location presets."), 6, TEXT("+Tier II Presets"));
	AddLevelQuest(TEXT("CometAdept"), 15, TEXT("Comet Adept"), TEXT("Reach Mechanic Level 15 for advanced PCG zones."), 11, TEXT("+Tier III Presets"));
	AddLevelQuest(TEXT("AuroraVirtuoso"), 20, TEXT("Aurora Virtuoso"), TEXT("Reach Mechanic Level 20."), 16, TEXT("+Tier IV Presets"));
	AddLevelQuest(TEXT("NebulaMaestro"), 25, TEXT("Nebula Maestro"), TEXT("Reach Mechanic Level 25."), 21, TEXT("+Tier V Presets"));
	AddLevelQuest(TEXT("DemoMaestro"), 30, TEXT("Demo Maestro"), TEXT("Master all demo location presets at Level 30."), 26, TEXT("+Maestro Sanctum"));

	auto AddPresetQuest = [this](const FName QuestId, const FName PresetId, const int32 Level, const FString& Title)
	{
		FMelodiaQuestDefinition Def;
		Def.QuestId = QuestId;
		Def.Title = Title;
		Def.Description = FString::Printf(TEXT("Unlock location preset at Mechanic Level %d."), Level);
		Def.ObjectiveType = EMelodiaQuestObjectiveType::UnlockLocationPreset;
		Def.TargetCount = 1;
		Def.RequiredMechanicLevel = Level;
		Def.TargetLocationPresetId = PresetId;
		Def.RewardText = TEXT("+Reverie Blueprint");
		AddQuestDefinition(Def);
	};

	AddPresetQuest(TEXT("UnlockEscherEntry"), TEXT("Lv06_EscherEntry"), 6, TEXT("Unlock: Escher Entry"));
	AddPresetQuest(TEXT("UnlockMaestroSanctum"), TEXT("Lv30_MaestroSanctum"), 30, TEXT("Unlock: Maestro Sanctum"));

	NotifyMechanicLevelChanged(1);
}

void AMelodiaQuestManagerBase::RegisterProgressionQuestChain()
{
	auto AddTierTutorQuest = [this](const FName QuestId, const int32 TierLevel, const FString& Title, const FString& Desc)
	{
		FMelodiaQuestDefinition Def;
		Def.QuestId = QuestId;
		Def.Title = Title;
		Def.Description = Desc;
		Def.ObjectiveType = EMelodiaQuestObjectiveType::ReachMechanicLevel;
		Def.TargetCount = TierLevel;
		Def.RequiredMechanicLevel = FMath::Max(1, TierLevel - 4);
		Def.RewardText = TEXT("+Tier skill unlock");
		AddQuestDefinition(Def);
	};

	AddTierTutorQuest(TEXT("Tutor_TierII"), 6, TEXT("Moon Tutor"), TEXT("Reach Lv6 to study Tier II songwriting."));
	AddTierTutorQuest(TEXT("Tutor_TierIV"), 16, TEXT("Aurora Tutor"), TEXT("Reach Lv16 for Tier IV harmonic keys."));
	AddTierTutorQuest(TEXT("Tutor_TierVI"), 26, TEXT("Celestial Tutor"), TEXT("Reach Lv26 for the demo capstone."));

	FMelodiaQuestDefinition KeyQuest;
	KeyQuest.QuestId = TEXT("KeyOfForte");
	KeyQuest.Title = TEXT("Key of Forte");
	KeyQuest.Description = TEXT("Equip the Forte harmonic key unlocked at Mechanic Lv3.");
	KeyQuest.ObjectiveType = EMelodiaQuestObjectiveType::EquipElementKey;
	KeyQuest.TargetCount = 1;
	KeyQuest.RequiredMechanicLevel = 3;
	KeyQuest.TargetElement = EMelodiaSpellElement::Forte;
	KeyQuest.RewardText = TEXT("+Forte Resonance Charm");
	AddQuestDefinition(KeyQuest);

	FMelodiaQuestDefinition WeaknessQuest;
	WeaknessQuest.QuestId = TEXT("WeaknessWaltz");
	WeaknessQuest.Title = TEXT("Weakness Waltz");
	WeaknessQuest.Description = TEXT("Land a weakness hit with a songwriting skill (tutorial at Lv3).");
	WeaknessQuest.ObjectiveType = EMelodiaQuestObjectiveType::WeaknessHit;
	WeaknessQuest.TargetCount = 1;
	WeaknessQuest.RequiredMechanicLevel = 3;
	WeaknessQuest.RewardText = TEXT("+Harmonic Primer");
	AddQuestDefinition(WeaknessQuest);

	FMelodiaQuestDefinition MasteryQuest;
	MasteryQuest.QuestId = TEXT("SongwritingMastery");
	MasteryQuest.Title = TEXT("Songwriting Mastery");
	MasteryQuest.Description = TEXT("Use 5 different unlocked songwriting skills in battle.");
	MasteryQuest.ObjectiveType = EMelodiaQuestObjectiveType::UseSongSkill;
	MasteryQuest.TargetCount = 5;
	MasteryQuest.RequiredMechanicLevel = 5;
	MasteryQuest.RewardText = TEXT("+Maestro Quill");
	AddQuestDefinition(MasteryQuest);

	FMelodiaQuestDefinition CompanionQuest;
	CompanionQuest.QuestId = TEXT("CockatooCompanion");
	CompanionQuest.Title = TEXT("Feathered Duet");
	CompanionQuest.Description = TEXT("Unlock the cockatoo companion at Mechanic Lv8.");
	CompanionQuest.ObjectiveType = EMelodiaQuestObjectiveType::UnlockCompanion;
	CompanionQuest.TargetCount = 1;
	CompanionQuest.RequiredMechanicLevel = 8;
	CompanionQuest.RewardText = TEXT("+Companion Whistle");
	AddQuestDefinition(CompanionQuest);

	for (const FMelodiaElementKeyDefinition& Key : UMelodiaKeySystemLibrary::BuildDemoElementKeys())
	{
		if (Key.KeyId == TEXT("Key_Lv03_Forte"))
		{
			continue;
		}
		FMelodiaQuestDefinition CollectKey;
		CollectKey.QuestId = FName(*FString::Printf(TEXT("Collect_%s"), *Key.KeyId.ToString()));
		CollectKey.Title = FString::Printf(TEXT("Collect %s"), *Key.DisplayName.ToString());
		CollectKey.Description = FString::Printf(TEXT("Unlock and equip %s at Lv%d."), *Key.DisplayName.ToString(), Key.MechanicLevelRequired);
		CollectKey.ObjectiveType = EMelodiaQuestObjectiveType::EquipElementKey;
		CollectKey.TargetCount = 1;
		CollectKey.RequiredMechanicLevel = Key.MechanicLevelRequired;
		CollectKey.TargetElement = Key.Element;
		CollectKey.RewardText = TEXT("+Key Shard");
		AddQuestDefinition(CollectKey);
	}
}

void AMelodiaQuestManagerBase::NotifySkillUsed(const FName SkillId)
{
	if (SkillId.IsNone())
	{
		return;
	}

	for (FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.Status != EMelodiaQuestStatus::Active)
		{
			continue;
		}
		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition || Definition->ObjectiveType != EMelodiaQuestObjectiveType::UseSongSkill)
		{
			continue;
		}
		if (Definition->TargetSkillId.IsNone() || Definition->TargetSkillId == SkillId)
		{
			++Progress.CurrentCount;
			if (Progress.CurrentCount >= Progress.TargetCount)
			{
				CompleteQuest(Progress.QuestId);
			}
		}
	}
	RefreshCounts();
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyKeyEquipped(const EMelodiaSpellElement Element)
{
	for (FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.Status != EMelodiaQuestStatus::Active)
		{
			continue;
		}
		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition || Definition->ObjectiveType != EMelodiaQuestObjectiveType::EquipElementKey)
		{
			continue;
		}
		if (Definition->TargetElement == Element)
		{
			Progress.CurrentCount = Progress.TargetCount;
			CompleteQuest(Progress.QuestId);
		}
	}
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyMechanicLevelChanged(const int32 NewMechanicLevel)
{
	for (FMelodiaQuestProgress& Progress : QuestProgress)
	{
		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition)
		{
			continue;
		}

		if (Progress.Status == EMelodiaQuestStatus::Locked
			&& Definition->RequiredMechanicLevel > 0
			&& NewMechanicLevel >= Definition->RequiredMechanicLevel)
		{
			Progress.Status = EMelodiaQuestStatus::Available;
		}

		if (Progress.Status != EMelodiaQuestStatus::Active && Progress.Status != EMelodiaQuestStatus::Available)
		{
			continue;
		}

		if (Definition->ObjectiveType == EMelodiaQuestObjectiveType::ReachMechanicLevel)
		{
			Progress.CurrentCount = FMath::Min(NewMechanicLevel, Progress.TargetCount);
			if (Progress.Status == EMelodiaQuestStatus::Available)
			{
				Progress.Status = EMelodiaQuestStatus::Active;
			}
			if (Progress.CurrentCount >= Progress.TargetCount && Progress.Status == EMelodiaQuestStatus::Active)
			{
				CompleteQuest(Progress.QuestId);
			}
		}
		else if (Definition->ObjectiveType == EMelodiaQuestObjectiveType::UnlockLocationPreset)
		{
			FMelodiaLocationLevelPreset Preset;
			if (UMelodiaMechanicProgressionLibrary::FindLocationPreset(Definition->TargetLocationPresetId, Preset)
				&& NewMechanicLevel >= Preset.MechanicLevelRequired)
			{
				if (Progress.Status == EMelodiaQuestStatus::Available)
				{
					Progress.Status = EMelodiaQuestStatus::Active;
				}
				Progress.CurrentCount = 1;
				if (Progress.Status == EMelodiaQuestStatus::Active)
				{
					CompleteQuest(Progress.QuestId);
				}
			}
		}
		else if (Definition->ObjectiveType == EMelodiaQuestObjectiveType::UnlockCompanion)
		{
			if (NewMechanicLevel >= 8)
			{
				if (Progress.Status == EMelodiaQuestStatus::Available)
				{
					Progress.Status = EMelodiaQuestStatus::Active;
				}
				Progress.CurrentCount = 1;
				if (Progress.Status == EMelodiaQuestStatus::Active)
				{
					CompleteQuest(Progress.QuestId);
				}
			}
		}
	}

	for (FMelodiaQuestProgress& Progress : QuestProgress)
	{
		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition || Definition->RequiredMechanicLevel <= 0)
		{
			continue;
		}
		if (Progress.Status == EMelodiaQuestStatus::Available && NewMechanicLevel >= Definition->RequiredMechanicLevel)
		{
			ActivateQuest(Progress.QuestId);
		}
	}

	RefreshCounts();
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyWeaknessHit()
{
	AdvanceQuest(TEXT("WeaknessWaltz"), 1);
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyBattleWon_Implementation()
{
	++CompletedBattleCount;
	bLastBattleWonNotified = true;
	AdvanceQuest(TEXT("SlimeBallad"), 1);

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				Progression->GrantMechanicXP(45, TEXT("Battle victory"));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia quest manager received NotifyBattleWon."));
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyReachedSongGate()
{
	AdvanceQuest(TEXT("WhisperAtGate"), 1);
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyPerfectNotesInSkill(const int32 PerfectCount)
{
	if (PerfectCount <= 0)
	{
		return;
	}

	if (FMelodiaQuestProgress* Progress = FindProgress(TEXT("PerfectRiff")))
	{
		if (Progress->Status == EMelodiaQuestStatus::Active)
		{
			Progress->CurrentCount = FMath::Max(Progress->CurrentCount, PerfectCount);
			if (Progress->CurrentCount >= Progress->TargetCount)
			{
				CompleteQuest(TEXT("PerfectRiff"));
			}
			RefreshCounts();
			SyncHUD(GetWorld());
		}
	}
}

void AMelodiaQuestManagerBase::NotifyEnemyBroken()
{
	AdvanceQuest(TEXT("BreakTheSlime"), 1);
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyUltimateVictory()
{
	AdvanceQuest(TEXT("UltimateCrescendo"), 1);
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyItemCollected(const FName ItemId, const int32 Quantity)
{
	if (ItemId == TEXT("GoldStuccoShard"))
	{
		AdvanceQuest(TEXT("StuccoScraps"), Quantity);
	}
	if (ItemId == TEXT("ReverieBlossom"))
	{
		AdvanceQuest(TEXT("BlossomGatherer"), Quantity);
	}
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyFlowerPicked(const FName FlowerItemId, const int32 Quantity)
{
	NotifyItemCollected(FlowerItemId, Quantity);
}

void AMelodiaQuestManagerBase::SyncHUD(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	TArray<FString> QuestLines;
	for (const FMelodiaQuestProgress& Progress : QuestProgress)
	{
		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition)
		{
			continue;
		}

		if (Progress.Status == EMelodiaQuestStatus::Completed)
		{
			QuestLines.Add(FString::Printf(TEXT("✓ %s"), *Definition->Title));
			continue;
		}

		if (Progress.Status != EMelodiaQuestStatus::Active)
		{
			continue;
		}

		QuestLines.Add(FString::Printf(TEXT("%s (%d/%d)"), *Definition->Title, Progress.CurrentCount, Progress.TargetCount));
	}

	TArray<FMelodiaInventorySlot> InventorySlots;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		if (const UMelodiaInventoryComponent* Inventory = It->FindComponentByClass<UMelodiaInventoryComponent>())
		{
			InventorySlots = Inventory->Slots;
			break;
		}
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->SetQuestLogEntries(QuestLines, LastQuestToast);
		Widget->SetInventorySlots(InventorySlots);
		Widget->SetMinimapMarkers(BuildQuestMarkers());
	}
}

TArray<FMelodiaMinimapMarker> AMelodiaQuestManagerBase::BuildQuestMarkers() const
{
	TArray<FMelodiaMinimapMarker> Markers;
	UWorld* World = GetWorld();

	for (const FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.Status != EMelodiaQuestStatus::Active)
		{
			continue;
		}

		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition)
		{
			continue;
		}

		// PCG-driven marker: auto-discover positions from PCG data.
		if (Definition->AssociatedPCGRole != EPCGArchitecturalRole::None && World)
		{
			const FVector SearchCenter = Definition->WorldMarkerLocation.IsNearlyZero(10.0f)
				? FVector::ZeroVector
				: Definition->WorldMarkerLocation;
			constexpr float SearchRadius = 50000.0f; // Large radius to cover the level.

			const TArray<FMelodiaWalkablePoint> PCGPoints =
				UMelodiaPCGLibrary::GetWalkablePointsByRoleInRadius(
					World, SearchCenter, SearchRadius, Definition->AssociatedPCGRole);

			for (const FMelodiaWalkablePoint& Pt : PCGPoints)
			{
				FMelodiaMinimapMarker Marker;
				Marker.WorldLocation = Pt.Location;
				Marker.Label = Definition->Title;
				Marker.Tint = FLinearColor(0.98f, 0.78f, 0.32f, 1.0f);
				Marker.bPulse = Definition->ObjectiveType == EMelodiaQuestObjectiveType::ReachLocation;
				Markers.Add(Marker);
			}
			continue;
		}

		// Static marker: use the manually-set WorldMarkerLocation.
		if (Definition->WorldMarkerLocation.IsNearlyZero(10.0f))
		{
			continue;
		}

		FMelodiaMinimapMarker Marker;
		Marker.WorldLocation = Definition->WorldMarkerLocation;
		Marker.Label = Definition->Title;
		Marker.Tint = FLinearColor(0.98f, 0.78f, 0.32f, 1.0f);
		Marker.bPulse = Definition->ObjectiveType == EMelodiaQuestObjectiveType::ReachLocation;
		Markers.Add(Marker);
	}
	return Markers;
}

void AMelodiaQuestManagerBase::NotifyPCGRebuilt()
{
	// Refresh HUD markers to reflect new PCG-derived quest positions.
	if (UWorld* World = GetWorld())
	{
		SyncHUD(World);
	}
}

void AMelodiaQuestManagerBase::AddQuestDefinition(const FMelodiaQuestDefinition& Definition)
{
	QuestCatalog.Add(Definition);

	FMelodiaQuestProgress Progress;
	Progress.QuestId = Definition.QuestId;
	Progress.Status = EMelodiaQuestStatus::Locked;
	Progress.CurrentCount = 0;
	Progress.TargetCount = FMath::Max(1, Definition.TargetCount);
	QuestProgress.Add(Progress);
}

FMelodiaQuestProgress* AMelodiaQuestManagerBase::FindProgress(const FName QuestId)
{
	for (FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.QuestId == QuestId)
		{
			return &Progress;
		}
	}
	return nullptr;
}

const FMelodiaQuestDefinition* AMelodiaQuestManagerBase::FindDefinition(const FName QuestId) const
{
	for (const FMelodiaQuestDefinition& Definition : QuestCatalog)
	{
		if (Definition.QuestId == QuestId)
		{
			return &Definition;
		}
	}
	return nullptr;
}

void AMelodiaQuestManagerBase::ActivateQuest(const FName QuestId)
{
	if (FMelodiaQuestProgress* Progress = FindProgress(QuestId))
	{
		const FMelodiaQuestDefinition* Definition = FindDefinition(QuestId);
		if (Definition && Definition->RequiredMechanicLevel > 0)
		{
			int32 PlayerLevel = 1;
			if (UWorld* World = GetWorld())
			{
				if (UGameInstance* GI = World->GetGameInstance())
				{
					if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
					{
						PlayerLevel = Progression->GetMechanicLevel();
					}
				}
			}
			if (PlayerLevel < Definition->RequiredMechanicLevel)
			{
				Progress->Status = EMelodiaQuestStatus::Locked;
				RefreshCounts();
				return;
			}
		}

		if (Progress->Status == EMelodiaQuestStatus::Locked || Progress->Status == EMelodiaQuestStatus::Available)
		{
			Progress->Status = EMelodiaQuestStatus::Active;
			Progress->CurrentCount = 0;
		}
	}
	RefreshCounts();
}

void AMelodiaQuestManagerBase::AdvanceQuest(const FName QuestId, const int32 Delta)
{
	FMelodiaQuestProgress* Progress = FindProgress(QuestId);
	if (!Progress || Progress->Status != EMelodiaQuestStatus::Active || Delta <= 0)
	{
		return;
	}

	Progress->CurrentCount = FMath::Clamp(Progress->CurrentCount + Delta, 0, Progress->TargetCount);
	if (Progress->CurrentCount >= Progress->TargetCount)
	{
		CompleteQuest(QuestId);
	}
	else
	{
		RefreshCounts();
	}
}

void AMelodiaQuestManagerBase::CompleteQuest(const FName QuestId)
{
	FMelodiaQuestProgress* Progress = FindProgress(QuestId);
	if (!Progress || Progress->Status == EMelodiaQuestStatus::Completed)
	{
		return;
	}

	Progress->Status = EMelodiaQuestStatus::Completed;
	Progress->CurrentCount = Progress->TargetCount;

	const FMelodiaQuestDefinition* Definition = FindDefinition(QuestId);
	LastQuestToast = Definition
		? FString::Printf(TEXT("Quest complete: %s | %s"), *Definition->Title, *Definition->RewardText)
		: FString::Printf(TEXT("Quest complete: %s"), *QuestId.ToString());

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				Progression->GrantMechanicXP(35, FString::Printf(TEXT("Quest %s"), *QuestId.ToString()));
			}
		}

		for (TActorIterator<APawn> It(World); It; ++It)
		{
			GrantQuestRewards(QuestId, *It);
			break;
		}
	}

	RefreshCounts();
	PushQuestToast(LastQuestToast);
}

void AMelodiaQuestManagerBase::GrantQuestRewards(const FName QuestId, APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	UMelodiaInventoryComponent* Inventory = PlayerPawn->FindComponentByClass<UMelodiaInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	if (QuestId == TEXT("WhisperAtGate"))
	{
		Inventory->AddItem(TEXT("ExplorerMap"), TEXT("Explorer Map"), 1, FLinearColor(0.62f, 0.92f, 1.0f, 1.0f));
		Inventory->AddItem(TEXT("MelodiaGold"), TEXT("Melodia Gold"), 25, FLinearColor(0.98f, 0.82f, 0.36f, 1.0f));
	}
	else if (QuestId == TEXT("SlimeBallad"))
	{
		if (Inventory->AddItem(TEXT("GoldStuccoShard"), TEXT("Gold Stucco Shard"), 1, FLinearColor(0.98f, 0.82f, 0.36f, 1.0f)))
		{
			NotifyItemCollected(TEXT("GoldStuccoShard"), 1);
		}
	}
	else if (QuestId == TEXT("StuccoScraps"))
	{
		Inventory->AddItem(TEXT("ReveriePotion"), TEXT("Reverie Tonic"), 1, FLinearColor(0.42f, 0.92f, 0.72f, 1.0f));
	}
	else if (QuestId == TEXT("PerfectRiff"))
	{
		Inventory->AddItem(TEXT("SheetMusic"), TEXT("Sheet Music"), 1, FLinearColor(0.98f, 0.88f, 0.62f, 1.0f));
	}
	else if (QuestId == TEXT("BreakTheSlime"))
	{
		Inventory->AddItem(TEXT("BreakCharm"), TEXT("Break Charm"), 1, FLinearColor(0.98f, 0.54f, 0.94f, 1.0f));
	}
	else if (QuestId == TEXT("UltimateCrescendo"))
	{
		Inventory->AddItem(TEXT("CrescendoCrystal"), TEXT("Crescendo Crystal"), 1, FLinearColor(0.86f, 0.72f, 1.0f, 1.0f));
	}
	else if (QuestId == TEXT("BlossomGatherer"))
	{
		Inventory->AddItem(TEXT("GardenShearsToken"), TEXT("Garden Shears Token"), 1, FLinearColor(0.62f, 0.98f, 0.52f, 1.0f));
	}
}

void AMelodiaQuestManagerBase::RefreshCounts()
{
	ActiveQuestCount = 0;
	CompletedQuestCount = 0;
	for (const FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.Status == EMelodiaQuestStatus::Active)
		{
			++ActiveQuestCount;
		}
		else if (Progress.Status == EMelodiaQuestStatus::Completed)
		{
			++CompletedQuestCount;
		}
	}
}

void AMelodiaQuestManagerBase::PushQuestToast(const FString& ToastText) const
{
	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetJudgmentString(ToastText);
			Widget->TriggerSparkleBurst();
		}
	}
}
