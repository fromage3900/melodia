// Runtime bootstrapper for the Melodia rhythm vertical slice.
//
// OWNERSHIP MODEL:
// ─────────────────────────────────────────────────────────────────────────────
// Platform-level (owned by GameModeBase, persist across runs):
//   - HUD widget, MusicManager, ExplorationPawn, BattleInput bridge
//
// Run-level (owned by AMelodiaReverieRunManager when present):
//   - WalkableIndex, EncounterSpawner, DecorationSpawner
//   - PCG graph generation, area transitions
//
// The ActiveWalkableIndex / ActivePCGEncounterSpawner references below are
// CACHED OBSERVER references. GameModeBase discovers them for convenience
// but does NOT own their lifecycle when a ReverieRunManager is active.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaRhythmGameModeBase.generated.h"

class AMelodiaCharacterBase;
class AController;
class APlayerController;
class AMelodiaMusicManager;
class AMelodiaLoopVerifier;
class APawn;
class AMelodiaEncounterTrigger;
class AMelodiaCompanionActor;
class AMelodiaNPCBase;
class AMelodiaReverieRunManager;
class AMelodiaRestPoint;
class AMelodiaPortal;
class AMelodiaPCGEncounterSpawner;
class AMelodiaPCGWalkableIndex;
class UPCGComponent;
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
	AMelodiaRhythmGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
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
	FSoftClassPath BattleDataClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_PhoenixRhythmPlaytestBattle.BP_PhoenixRhythmPlaytestBattle_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath QuestManagerClassPath = FSoftClassPath(TEXT("/Game/Melodia/Core/BP_QuestManager.BP_QuestManager_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath CompanionClassPath = FSoftClassPath(TEXT("/Game/Melodia/Characters/Companions/BP_CockatooCompanion.BP_CockatooCompanion_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath ProgressionNPCClassPath = FSoftClassPath(TEXT("/Game/Melodia/NPC/BP_NPC_TierTutor.BP_NPC_TierTutor_C"));

	/** Reparent BP_Melusina to AMelodiaCharacterBase for baked-in components. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FSoftClassPath ExplorationPawnClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_Melusina.BP_Melusina_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FVector ExplorationReturnLocation = FVector(-10680.0f, -5000.0f, -2140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	FVector EncounterTriggerLocation = FVector(-10240.0f, -5000.0f, -2140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	float EncounterTriggerForwardOffset = 440.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	bool bRunLoopVerifier = false;

	/** Bare-minimum slice: PlayerStart explore, one song gate, one battle. Skips PCG/quest/verifier extras. Off by default — enable only for smoke tests (?MinimalDemo on URL). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	bool bMinimalDemoMode = false;

	/** Legacy rhythm test manager spawns beat-print debug actor — off by default (duplicates Melodia battle clock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Loop")
	bool bEnableLegacyRhythmTestManager = false;

	/** When true, strips Phoenix BattleUI after encounter init (Melodia-only HUD). False = Option B: keep Phoenix menus for you to replace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Battle")
	bool bSuppressPhoenixBattleUI = false;

	/** When true, place encounters/rest/portal from PCG walkable data instead of hardcoded offsets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|PCG")
	bool bUsePCGPlacement = false;

	/** Radius (cm) around the PCG volume center used for walkable-point queries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|PCG", meta=(ClampMin="500"))
	float PCGPlacementSearchRadius = 15000.0f;

	/**
	 * Cached reference to the active walkable index.
	 * 
	 * OWNERSHIP: When AMelodiaReverieRunManager is present, it owns the
	 * WalkableIndex lifecycle. This reference is cached for convenient
	 * queries; do not spawn/destroy directly when a run manager exists.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Melodia|PCG")
	TObjectPtr<AMelodiaPCGWalkableIndex> ActiveWalkableIndex;

	/**
	 * Cached reference to the active encounter spawner.
	 * 
	 * OWNERSHIP: When AMelodiaReverieRunManager is present, it owns the
	 * EncounterSpawner lifecycle. This reference is cached for convenient
	 * queries; do not spawn/destroy directly when a run manager exists.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Melodia|PCG")
	TObjectPtr<AMelodiaPCGEncounterSpawner> ActivePCGEncounterSpawner;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|PCG")
	int32 PCGWalkablePointCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|PCG")
	int32 PCGSpawnedEncounterCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|PCG")
	bool bPCGPlacementActive = false;

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

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaReverieRunManager> ActiveReverieRunManager;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaRestPoint> ActiveRestPoint;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaPortal> ActivePortal;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaCompanionActor> ActiveCompanion;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TArray<TObjectPtr<AMelodiaNPCBase>> ActiveProgressionNPCs;

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

	/** Called by UMelodiaBattleSession when an encounter begins. */
	UFUNCTION(BlueprintCallable, Category="Melodia|Battle")
	void NotifyBattleSessionBegan(AActor* BattleController);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Melodia|Loop")
	TObjectPtr<AMelodiaMusicManager> ActiveMusicManager;

	FTimerHandle BootstrapRetryHandle;
	FTimerHandle PCGPlacementRetryHandle;
	FTimerHandle VictoryAutoExitHandle;
	int32 PCGPlacementRetryCount = 0;
	static constexpr int32 MaxPCGPlacementRetries = 20;

	UClass* ResolveClass(const FSoftClassPath& ClassPath) const;
	AActor* FindExistingActorOfClass(UClass* ActorClass) const;
	AActor* SpawnLoopActor(UClass* ActorClass, const FVector& Location, const FRotator& Rotation) const;
	FVector ResolveExplorationSpawnLocation() const;
	FVector ResolveEncounterTriggerLocation() const;
	void SyncExplorationLocations();
	void FinishLoopBootstrap();
	void RetryExplorationBootstrap();
	bool ShouldRunLoopVerifier() const;
	void RestoreExplorationControl();
	void EnsureEncounterTrigger();
	void EnsureQuestManager();
	void EnsureCompanionActor();
	void EnsureProgressionNPCs();
	void EnsureReverieRunManager();
	void EnsureWorldInteractions();
	void EnsurePortfolioFlowers();
	void EnsurePCGGameplayPlacement();
	void RetryPCGGameplayPlacement();
	UPCGComponent* FindPrimaryPCGComponent() const;
	FVector FindPCGWorldCenter() const;
	bool TryResolveWalkableLocation(const FVector& Hint, EPCGArchitecturalRole PreferredRole, float SearchRadius, FVector& OutLocation) const;
	void ApplyPCGPlacedInteractables();
	void EnsureBattleInputBridge();
	void EnsureRhythmHUDWidget();
	void PrepareMelodiaBattleView();
	void AutoConfirmVictoryIfPending();
	void PrepareExplorationPresentation();
	void SanitizeWorldForMinimalDemo();
	void EnsureBattleMusicClock();
	void ApplyMelusinaPresentation(APawn* ExplorationPawn);
};
