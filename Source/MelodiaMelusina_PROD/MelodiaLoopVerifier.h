// PIE-only runtime verifier for the Melodia rhythm battle loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaLoopVerifier.generated.h"

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaLoopVerifier : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaLoopVerifier();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	bool bRunOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	float VerificationDelaySeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	FSoftClassPath BattleControllerClassPath = FSoftClassPath(TEXT("/Game/TurnBasedJRPGTemplate/Blueprints/Battle/BP_BattleController.BP_BattleController_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	FSoftClassPath RhythmWidgetClassPath = FSoftClassPath(TEXT("/Game/Blueprints/WBP_RhythmHUD.WBP_RhythmHUD_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	FSoftClassPath QuestManagerClassPath = FSoftClassPath(TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop Verification")
	FSoftClassPath RhythmManagerClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_RhythmTestBattleManager.BP_RhythmTestBattleManager_C"));

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop Verification")
	bool bLastVerificationPassed = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop Verification")
	FString LastVerificationSummary;

	UFUNCTION(BlueprintCallable, Category="Melodia|Loop Verification")
	bool RunVerificationNow();

private:
	float ElapsedSeconds = 0.0f;
	float FirstBeatPosition = 0.0f;
	bool bCapturedFirstBeat = false;
	bool bHasRun = false;

	UClass* ResolveClass(const FSoftClassPath& ClassPath) const;
	AActor* FindOrSpawnActor(UClass* ActorClass, const FVector& Location) const;
	bool VerifyMusicClock(FString& Detail);
	bool VerifyBattleHooks(FString& Detail);
	bool VerifyEncounterSessionPhases(FString& Detail);
	bool VerifyHUDHooks(FString& Detail);
	bool VerifyQuestHook(FString& Detail);
	bool VerifyRhythmManagerWiring(FString& Detail);
	bool VerifyPCGGraphs(FString& Detail);
};
