// Native quest manager with starter quests, progress tracking, and HUD hooks.

#include "MelodiaQuestManagerBase.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaRhythmHUDWidget.h"
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

	ActivateQuest(TEXT("WhisperAtGate"));
	ActivateQuest(TEXT("SlimeBallad"));
	ActivateQuest(TEXT("StuccoScraps"));
	ActivateQuest(TEXT("PerfectRiff"));
	ActivateQuest(TEXT("BreakTheSlime"));
	ActivateQuest(TEXT("UltimateCrescendo"));

	LastQuestToast = TEXT("New quests added to your journal");
	SyncHUD(GetWorld());
}

void AMelodiaQuestManagerBase::NotifyBattleWon_Implementation()
{
	++CompletedBattleCount;
	bLastBattleWonNotified = true;
	AdvanceQuest(TEXT("SlimeBallad"), 1);

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
	SyncHUD(GetWorld());
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

	for (TObjectIterator<UMelodiaRhythmHUDWidget> WidgetIt; WidgetIt; ++WidgetIt)
	{
		UMelodiaRhythmHUDWidget* Widget = *WidgetIt;
		if (Widget && Widget->GetWorld() == World)
		{
			Widget->SetQuestLogEntries(QuestLines, LastQuestToast);
			Widget->SetInventorySlots(InventorySlots);
			Widget->SetMinimapMarkers(BuildQuestMarkers());
		}
	}
}

TArray<FMelodiaMinimapMarker> AMelodiaQuestManagerBase::BuildQuestMarkers() const
{
	TArray<FMelodiaMinimapMarker> Markers;
	for (const FMelodiaQuestProgress& Progress : QuestProgress)
	{
		if (Progress.Status != EMelodiaQuestStatus::Active)
		{
			continue;
		}

		const FMelodiaQuestDefinition* Definition = FindDefinition(Progress.QuestId);
		if (!Definition || Definition->WorldMarkerLocation.IsNearlyZero(10.0f))
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
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			UMelodiaRhythmHUDWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World)
			{
				Widget->SetJudgmentString(ToastText);
				Widget->TriggerSparkleBurst();
			}
		}
	}
}
