// Runtime bootstrapper for the Melodia rhythm vertical slice.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MelodiaRhythmGameModeBase.generated.h"

class AMelodiaMusicManager;
class AMelodiaLoopVerifier;
class APawn;
class AMelodiaEncounterTrigger;
class UMelodiaCosmeticsComponent;
class UMelodiaBattleInputComponent;
class UMelodiaRhythmHUDWidget;

UENUM(BlueprintType)
enum class EMelodiaLoopPhase : uint8
{
	Bootstrapping,
	Battle,
	VictoryReward,
	ExplorationReady
};

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaRhythmGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	float DefaultBattleBPM = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath QuartzMusicManagerClassPath = FSoftClassPath(TEXT("/Game/Melodia/Core/BP_MelodiaQuartzMusicManager.BP_MelodiaQuartzMusicManager_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath RhythmHUDActorClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_RhythmHUD.BP_RhythmHUD_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath RhythmHUDWidgetClassPath = FSoftClassPath(TEXT("/Game/Blueprints/WBP_RhythmHUD.WBP_RhythmHUD_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath RhythmTestManagerClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_RhythmTestBattleManager.BP_RhythmTestBattleManager_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath BattleControllerClassPath = FSoftClassPath(TEXT("/Game/TurnBasedJRPGTemplate/Blueprints/Battle/BP_BattleController.BP_BattleController_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath QuestManagerClassPath = FSoftClassPath(TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath ExplorationPawnClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_Melusina.BP_Melusina_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FVector ExplorationReturnLocation = FVector(-10480.0f, -5000.0f, -2140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FVector EncounterTriggerLocation = FVector(-10240.0f, -5000.0f, -2140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	bool bRunLoopVerifier = true;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	EMelodiaLoopPhase CurrentLoopPhase = EMelodiaLoopPhase::Bootstrapping;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	int32 BattlePhaseEntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	int32 VictoryRewardPhaseCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	int32 ExplorationReadyPhaseCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	FString LastLoopPhaseText = TEXT("Bootstrapping");

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	bool bExplorationControlReady = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	int32 ExplorationControlRestoreCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<APawn> ActiveExplorationPawn;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaEncounterTrigger> ActiveEncounterTrigger;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	TObjectPtr<UMelodiaCosmeticsComponent> ActiveCosmeticsComponent;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	bool bMelusinaPawnActive = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	int32 MelusinaPawnApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	FString LastCosmeticPresetText;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	TObjectPtr<UMelodiaRhythmHUDWidget> ActiveRhythmHUDWidget;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Presentation")
	bool bRhythmHUDWidgetInViewport = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	TObjectPtr<AActor> ActiveBattleController;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	TObjectPtr<UMelodiaBattleInputComponent> ActiveBattleInputComponent;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Battle Input")
	bool bBattleInputBound = false;

	UFUNCTION(BlueprintCallable, Category="Melodia|Loop")
	void SetLoopPhase(EMelodiaLoopPhase NewPhase);

	UFUNCTION(BlueprintPure, Category="Melodia|Loop")
	FString GetLoopPhaseText() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaMusicManager> ActiveMusicManager;

	UClass* ResolveClass(const FSoftClassPath& ClassPath) const;
	AActor* FindExistingActorOfClass(UClass* ActorClass) const;
	AActor* SpawnLoopActor(UClass* ActorClass, const FVector& Location, const FRotator& Rotation) const;
	void RestoreExplorationControl();
	void EnsureEncounterTrigger();
	void EnsureBattleInputBridge();
	void EnsureRhythmHUDWidget();
	void ApplyMelusinaPresentation(APawn* ExplorationPawn);
};
