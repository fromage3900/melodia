// Drives the tight-coupled note highway: song pattern -> beat-synced hits -> damage resolution.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaRhythmExecutionComponent.generated.h"

class AMelodiaMusicManager;

UENUM(BlueprintType)
enum class EMelodiaRhythmExecutionState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Active UMETA(DisplayName="Active"),
	Resolving UMETA(DisplayName="Resolving")
};

USTRUCT(BlueprintType)
struct FMelodiaHighwayNote
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm")
	float TargetBeat = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm")
	int32 Pitch = 60;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm")
	EMelodiaRhythmGrade Grade = EMelodiaRhythmGrade::Miss;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm")
	bool bResolved = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm")
	bool bCountsAsHit = false;
};

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaRhythmExecutionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaRhythmExecutionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm Execution")
	FMelodiaRhythmWindows RhythmWindows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm Execution")
	float LeadInBeats = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rhythm Execution")
	float ScrollBeatsAhead = 2.5f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	EMelodiaRhythmExecutionState ExecutionState = EMelodiaRhythmExecutionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	bool bSkillExecution = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	FMelodiaGeneratedSpell ActiveSpell;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	TArray<FMelodiaHighwayNote> ActiveNotes;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	int32 CurrentNoteIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	int32 ResolvedNoteCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	float ExecutionStartBeat = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	float AverageGradeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	int32 PerfectCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rhythm Execution")
	int32 HitCount = 0;

	UFUNCTION(BlueprintCallable, Category="Melodia|Rhythm Execution")
	bool BeginSkillExecution();

	UFUNCTION(BlueprintCallable, Category="Melodia|Rhythm Execution")
	bool BeginBasicExecution();

	UFUNCTION(BlueprintCallable, Category="Melodia|Rhythm Execution")
	bool TryHitCurrentNote();

	UFUNCTION(BlueprintPure, Category="Melodia|Rhythm Execution")
	bool IsExecutionActive() const;

	UFUNCTION(BlueprintPure, Category="Melodia|Rhythm Execution")
	float GetCurrentBeatPosition() const;

	UFUNCTION(BlueprintCallable, Category="Melodia|Rhythm Execution")
	void CancelExecution();

private:
	void BuildNotesFromSong(const TArray<int32>& NotePitches, const TArray<float>& NoteDurations, EMelodiaInstrument Instrument, const TArray<FMelodiaSongMaterialInput>& Materials, const bool bSkill);
	void BuildBasicNotes();
	void SyncHighwayHUD();
	void ResolveMissedNotes();
	void FinishExecution();
	AMelodiaMusicManager* FindMusicManager() const;
	float GetBeatLengthSeconds() const;
};
