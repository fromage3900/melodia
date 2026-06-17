// Native input bridge from mapped battle commands into the Melodia rhythm loop.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaBattleInputComponent.generated.h"

class UMelodiaRhythmExecutionComponent;

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaBattleInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaBattleInputComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	FMelodiaRhythmWindows RhythmWindows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	float InputCommandGrade = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	int32 ComboToWin = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	bool bAutoBindPlayerInput = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	bool bInputBound = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	int32 BasicInputCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	int32 SkillInputCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	int32 UltimateInputCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	int32 SuccessfulCommandCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	FString LastInputCommandName;

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool BindBattleInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool HandleBasicInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool HandleSkillInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool HandleUltimateInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool HandleFleeInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Battle Input")
	bool HandleCycleSkillInput();

private:
	UMelodiaRhythmExecutionComponent* EnsureExecutionComponent();
	bool IsBattleInputAllowed() const;
	bool TryConfirmVictoryReward();
	bool ExecuteInputCommand(EMelodiaRhythmBattleCommand Command, float GradeOverride = -1.0f);
	FMelodiaRhythmGradeResult GradeCurrentBeatTap() const;
	void ShowTapFeedback(const FMelodiaRhythmGradeResult& GradeResult) const;
	void OnBasicInputPressed();
	void OnSkillInputPressed();
	void OnUltimateInputPressed();
	void OnFleeInputPressed();
	void OnCycleSkillInputPressed();
	void ShowActiveSkillPrompt() const;
	void PrimeRhythmBlueprintHooks(AActor* BattleController, float Grade) const;
};
