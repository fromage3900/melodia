// Native input bridge from mapped battle commands into the Melodia rhythm loop.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaBattleInputComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaBattleInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaBattleInputComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	float InputCommandGrade = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	int32 ComboToWin = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle Input")
	bool bAutoBindPlayerInput = true;

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

private:
	bool ExecuteInputCommand(EMelodiaRhythmBattleCommand Command);
	void OnBasicInputPressed();
	void OnSkillInputPressed();
	void OnUltimateInputPressed();
	void PrimeRhythmBlueprintHooks(AActor* BattleController) const;
};
