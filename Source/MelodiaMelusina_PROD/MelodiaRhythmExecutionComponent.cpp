// Drives the tight-coupled note highway: song pattern -> beat-synced hits -> damage resolution.

#include "MelodiaRhythmExecutionComponent.h"

#include "EngineUtils.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaMusicManager.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/UObjectIterator.h"

namespace
{
FLinearColor GradeTint(const EMelodiaRhythmGrade Grade, const FLinearColor& DefaultTint)
{
	switch (Grade)
	{
	case EMelodiaRhythmGrade::Perfect:
		return FLinearColor(0.42f, 1.0f, 0.72f, 1.0f);
	case EMelodiaRhythmGrade::Great:
		return FLinearColor(0.62f, 0.92f, 1.0f, 1.0f);
	case EMelodiaRhythmGrade::Good:
		return FLinearColor(0.98f, 0.88f, 0.36f, 1.0f);
	case EMelodiaRhythmGrade::Miss:
		return FLinearColor(0.96f, 0.28f, 0.42f, 1.0f);
	default:
		return DefaultTint;
	}
}
}

UMelodiaRhythmExecutionComponent::UMelodiaRhythmExecutionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMelodiaRhythmExecutionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMelodiaRhythmExecutionComponent::TickComponent(const float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ExecutionState != EMelodiaRhythmExecutionState::Active)
	{
		return;
	}

	ResolveMissedNotes();
	SyncHighwayHUD();

	if (CurrentNoteIndex >= ActiveNotes.Num())
	{
		FinishExecution();
	}
}

bool UMelodiaRhythmExecutionComponent::BeginSkillExecution()
{
	if (ExecutionState != EMelodiaRhythmExecutionState::Idle)
	{
		return false;
	}

	AActor* BattleController = GetOwner();
	if (!BattleController || !UMelodiaBattleLoopLibrary::CanUseRhythmSkill(BattleController))
	{
		return false;
	}

	TArray<int32> Pitches = { 60, 64, 67, 72 };
	TArray<float> Durations = { 1.0f, 1.0f, 1.0f, 1.0f };
	TArray<FMelodiaSongMaterialInput> Materials;
	FMelodiaSongMaterialInput Material;
	Material.MaterialId = TEXT("GoldStucco");
	Material.RarityTier = 2;
	Material.PowerModifier = 1.1f;
	Materials.Add(Material);

	bSkillExecution = true;
	BuildNotesFromSong(Pitches, Durations, EMelodiaInstrument::MusicBox, Materials, true);
	return ActiveNotes.Num() > 0;
}

bool UMelodiaRhythmExecutionComponent::BeginBasicExecution()
{
	if (ExecutionState != EMelodiaRhythmExecutionState::Idle)
	{
		return false;
	}

	bSkillExecution = false;
	BuildBasicNotes();
	return ActiveNotes.Num() > 0;
}

bool UMelodiaRhythmExecutionComponent::TryHitCurrentNote()
{
	if (ExecutionState != EMelodiaRhythmExecutionState::Active || !ActiveNotes.IsValidIndex(CurrentNoteIndex))
	{
		return false;
	}

	FMelodiaHighwayNote& Note = ActiveNotes[CurrentNoteIndex];
	if (Note.bResolved)
	{
		return false;
	}

	const float BeatPosition = GetCurrentBeatPosition();
	const float TimingErrorMs = FMath::Abs(BeatPosition - Note.TargetBeat) * GetBeatLengthSeconds() * 1000.0f;

	const FMelodiaRhythmGradeResult GradeResult = UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(TimingErrorMs, RhythmWindows);
	Note.Grade = GradeResult.Grade;
	Note.bCountsAsHit = GradeResult.bCountsAsHit;
	Note.bResolved = true;

	if (GradeResult.Grade == EMelodiaRhythmGrade::Perfect)
	{
		++PerfectCount;
	}
	if (GradeResult.bCountsAsHit)
	{
		++HitCount;
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetJudgment(GradeResult.DisplayText);
			Widget->DoPulse();
			Widget->PushFloatingCombatText(GradeResult.DisplayText.ToString(), true, GradeTint(GradeResult.Grade, FLinearColor(0.86f, 0.72f, 1.0f, 1.0f)));
			if (GradeResult.bCountsAsHit)
			{
				Widget->TriggerSparkleBurst();
			}
		}
	}

	++CurrentNoteIndex;
	++ResolvedNoteCount;
	SyncHighwayHUD();
	return true;
}

bool UMelodiaRhythmExecutionComponent::IsExecutionActive() const
{
	return ExecutionState == EMelodiaRhythmExecutionState::Active;
}

float UMelodiaRhythmExecutionComponent::GetCurrentBeatPosition() const
{
	if (const AMelodiaMusicManager* MusicManager = FindMusicManager())
	{
		return MusicManager->GetCurrentBeatPosition();
	}

	return 0.0f;
}

void UMelodiaRhythmExecutionComponent::CancelExecution()
{
	ExecutionState = EMelodiaRhythmExecutionState::Idle;
	ActiveNotes.Reset();
	CurrentNoteIndex = 0;
	ResolvedNoteCount = 0;
	bSkillExecution = false;

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetNoteHighwayActive(false, TArray<FMelodiaHighwayNote>(), 0.0f, ScrollBeatsAhead);
		}
	}
}

void UMelodiaRhythmExecutionComponent::BuildNotesFromSong(const TArray<int32>& NotePitches, const TArray<float>& NoteDurations, const EMelodiaInstrument Instrument, const TArray<FMelodiaSongMaterialInput>& Materials, const bool bSkill)
{
	ActiveSpell = UMelodiaCoreRulesLibrary::GenerateSpellFromSong(NotePitches, NoteDurations, Instrument, Materials);

	ExecutionStartBeat = GetCurrentBeatPosition();
	ActiveNotes.Reset();
	CurrentNoteIndex = 0;
	ResolvedNoteCount = 0;
	PerfectCount = 0;
	HitCount = 0;

	float BeatCursor = ExecutionStartBeat + LeadInBeats;
	const int32 NoteCount = FMath::Max(ActiveSpell.HitCount, NotePitches.Num());
	for (int32 Index = 0; Index < NoteCount; ++Index)
	{
		FMelodiaHighwayNote Note;
		Note.TargetBeat = BeatCursor;
		Note.Pitch = NotePitches.IsValidIndex(Index) ? NotePitches[Index] : 60;
		ActiveNotes.Add(Note);

		const float DurationBeats = NoteDurations.IsValidIndex(Index) ? FMath::Max(0.25f, NoteDurations[Index]) : 1.0f;
		BeatCursor += DurationBeats;
	}

	ExecutionState = EMelodiaRhythmExecutionState::Active;
	SyncHighwayHUD();

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->ShowActionPrompt(bSkill ? TEXT("Tap on beat | 2=Skill | Space/1=Hit") : TEXT("Tap on beat | Space/1=Hit"));
		}
	}
}

void UMelodiaRhythmExecutionComponent::BuildBasicNotes()
{
	TArray<int32> Pitches = { 60, 62, 64 };
	TArray<float> Durations = { 1.0f, 1.0f, 1.0f };
	TArray<FMelodiaSongMaterialInput> EmptyMaterials;
	BuildNotesFromSong(Pitches, Durations, EMelodiaInstrument::MusicBox, EmptyMaterials, false);
}

void UMelodiaRhythmExecutionComponent::SyncHighwayHUD()
{
	if (UWorld* World = GetWorld())
	{
		const float BeatPosition = GetCurrentBeatPosition();
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetNoteHighwayActive(
				ExecutionState == EMelodiaRhythmExecutionState::Active,
				ActiveNotes,
				BeatPosition,
				ScrollBeatsAhead);
		}
	}
}

void UMelodiaRhythmExecutionComponent::ResolveMissedNotes()
{
	if (!ActiveNotes.IsValidIndex(CurrentNoteIndex))
	{
		return;
	}

	const float BeatPosition = GetCurrentBeatPosition();
	const float BeatLength = GetBeatLengthSeconds();
	const float MissWindowBeats = RhythmWindows.GoodWindowMs / FMath::Max(BeatLength * 1000.0f, KINDA_SMALL_NUMBER);

	while (ActiveNotes.IsValidIndex(CurrentNoteIndex))
	{
		FMelodiaHighwayNote& Note = ActiveNotes[CurrentNoteIndex];
		if (Note.bResolved)
		{
			++CurrentNoteIndex;
			continue;
		}

		if (BeatPosition <= Note.TargetBeat + MissWindowBeats)
		{
			break;
		}

		Note.Grade = EMelodiaRhythmGrade::Miss;
		Note.bCountsAsHit = false;
		Note.bResolved = true;
		++CurrentNoteIndex;
		++ResolvedNoteCount;

		if (UWorld* World = GetWorld())
		{
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
			{
				Widget->SetJudgmentString(TEXT("Miss"));
				Widget->PushFloatingCombatText(TEXT("MISS"), true, FLinearColor(0.96f, 0.28f, 0.42f, 1.0f));
			}
		}
	}
}

void UMelodiaRhythmExecutionComponent::FinishExecution()
{
	if (ExecutionState != EMelodiaRhythmExecutionState::Active)
	{
		return;
	}

	ExecutionState = EMelodiaRhythmExecutionState::Resolving;

	float MultiplierSum = 0.0f;
	int32 Count = 0;
	for (const FMelodiaHighwayNote& Note : ActiveNotes)
	{
		MultiplierSum += UMelodiaCoreRulesLibrary::GetRhythmCombatMultiplier(Note.Grade, Count);
		++Count;
	}

	AverageGradeMultiplier = Count > 0 ? MultiplierSum / static_cast<float>(Count) : 0.4f;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
		{
			It->NotifyPerfectNotesInSkill(PerfectCount);
			break;
		}
	}

	AActor* BattleController = GetOwner();
	if (BattleController)
	{
		const float SkillScalar = bSkillExecution
			? FMath::Max(0.1f, ActiveSpell.Power / 100.0f)
			: 1.0f;
		UMelodiaBattleLoopLibrary::ApplyRhythmExecutionResult(
			this,
			BattleController,
			ActiveNotes,
			bSkillExecution,
			SkillScalar,
			ActiveSpell.SPCost);
	}

	CancelExecution();
}

AMelodiaMusicManager* UMelodiaRhythmExecutionComponent::FindMusicManager() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AMelodiaMusicManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

float UMelodiaRhythmExecutionComponent::GetBeatLengthSeconds() const
{
	if (const AMelodiaMusicManager* MusicManager = FindMusicManager())
	{
		return MusicManager->GetBeatLength();
	}

	return 60.0f / 128.0f;
}
