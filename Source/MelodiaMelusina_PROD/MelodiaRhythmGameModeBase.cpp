// Runtime bootstrapper for the Melodia rhythm vertical slice.

#include "MelodiaRhythmGameModeBase.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleInputComponent.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaBattleSession.h"
#include "MelodiaCharacterBase.h"
#include "MelodiaCompanionActor.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaExplorationInputComponent.h"
#include "MelodiaGameplayLoopTestDirector.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaJRPGBridgeLibrary.h"
#include "MelodiaJRPGPresenter.h"
#include "MelodiaLoopVerifier.h"
#include "MelodiaMusicManager.h"
#include "MelodiaNPCBase.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaReverieRunManager.h"
#include "MelodiaRestPoint.h"
#include "MelodiaPickableFlower.h"
#include "MelodiaPortal.h"
#include "MelodiaSaveGame.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaRhythmHUDWidget.h"
#include "MelodiaPCGEncounterSpawner.h"
#include "MelodiaPCGWalkableIndex.h"
#include "MelodiaPCGLibrary.h"
#include "PCGComponent.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"

AMelodiaRhythmGameModeBase::AMelodiaRhythmGameModeBase()
{
	DefaultPawnClass = nullptr;
	bMinimalDemoMode = false;
	bUsePCGPlacement = false;
}

void AMelodiaRhythmGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (UClass* ExplorationPawnClass = ResolveClass(ExplorationPawnClassPath))
	{
		DefaultPawnClass = ExplorationPawnClass;
	}
	else if (DefaultPawnClass == nullptr)
	{
		DefaultPawnClass = AMelodiaCharacterBase::StaticClass();
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap falling back to native AMelodiaCharacterBase (could not resolve %s)."),
			*ExplorationPawnClassPath.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap could not resolve exploration pawn class: %s"), *ExplorationPawnClassPath.ToString());
	}

	if (FParse::Param(*Options, TEXT("MinimalDemo")))
	{
		bMinimalDemoMode = true;
	}
	else if (FParse::Param(*Options, TEXT("FullGame")))
	{
		bMinimalDemoMode = false;
	}

	if (FParse::Param(*Options, TEXT("GameplayLoopTest"))
		|| MapName.Contains(TEXT("GameplayLoopTest"), ESearchCase::IgnoreCase))
	{
		bGameplayLoopTestMap = true;
		bPreferLevelPlacedLoopActors = true;
		bUsePCGPlacement = false;
	}
	else if (FParse::Param(*Options, TEXT("PCGDemo"))
		|| MapName.Contains(TEXT("MelodiaPCGDemo"), ESearchCase::IgnoreCase))
	{
		ConfigurePCGDemo();
	}
	else if (FParse::Param(*Options, TEXT("PortfolioBezier"))
		|| MapName.Contains(TEXT("MelodiaPortfolioTerrace"), ESearchCase::IgnoreCase))
	{
		ConfigurePortfolioBezierDemo();
	}
}

void AMelodiaRhythmGameModeBase::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (APawn* SpawnedPawn = NewPlayer ? NewPlayer->GetPawn() : nullptr)
	{
		ActiveExplorationPawn = SpawnedPawn;
		SyncExplorationLocations();
		SpawnedPawn->SetActorLocation(ExplorationReturnLocation, false, nullptr, ETeleportType::TeleportPhysics);

		if (APlayerController* PlayerController = Cast<APlayerController>(NewPlayer))
		{
			PlayerController->Possess(SpawnedPawn);
			PlayerController->SetViewTarget(SpawnedPawn);
		}
	}
}

void AMelodiaRhythmGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	EnsureRhythmHUDWidget();
	if (CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady)
	{
		RestoreExplorationControl();
	}
}

void AMelodiaRhythmGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	SetLoopPhase(EMelodiaLoopPhase::Bootstrapping);

	if (bGameplayLoopTestMap)
	{
		EnsureGameplayLoopTestDirector();
	}

	if (bMinimalDemoMode)
	{
		SanitizeWorldForMinimalDemo();
		if (UClass* QuartzMusicClass = ResolveClass(QuartzMusicManagerClassPath))
		{
			ActiveMusicManager = Cast<AMelodiaMusicManager>(FindExistingActorOfClass(QuartzMusicClass));
		}
		EnsureRhythmHUDWidget();
		EnsureQuestManager();
		EnsureCompanionActor();
		EnsureProgressionNPCs();
		EnsureEncounterTrigger();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AMelodiaRhythmGameModeBase::FinishLoopBootstrap));
		}
		UE_LOG(LogTemp, Warning, TEXT("MELODIA_MIN_DEMO: explore with WASD, walk to the glowing sphere, Space/1 to attack in battle."));
		return;
	}

	UClass* QuartzMusicClass = ResolveClass(QuartzMusicManagerClassPath);
	if (QuartzMusicClass)
	{
		AActor* ExistingMusicActor = FindExistingActorOfClass(QuartzMusicClass);
		if (!ExistingMusicActor)
		{
			ExistingMusicActor = SpawnLoopActor(QuartzMusicClass, FVector(-10650.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
		}

		ActiveMusicManager = Cast<AMelodiaMusicManager>(ExistingMusicActor);
		if (ActiveMusicManager)
		{
			UE_LOG(LogTemp, Log, TEXT("Melodia loop bootstrap found Quartz music manager (battle music deferred until encounter)."));
		}
	}

	if (UClass* RhythmHUDClass = ResolveClass(RhythmHUDActorClassPath))
	{
		// Strategy B: single HUD owner is WBP_RhythmHUD via EnsureRhythmHUDWidget — do not spawn BP_RhythmHUD (duplicate widgets).
		(void)RhythmHUDClass;
	}

	EnsureRhythmHUDWidget();

	if (UClass* RhythmTestManagerClass = ResolveClass(RhythmTestManagerClassPath))
	{
		if (bEnableLegacyRhythmTestManager && !FindExistingActorOfClass(RhythmTestManagerClass))
		{
			SpawnLoopActor(RhythmTestManagerClass, FVector(-10700.0f, -5040.0f, -2150.0f), FRotator::ZeroRotator);
		}
	}

	EnsureQuestManager();
	EnsureCompanionActor();
	EnsureProgressionNPCs();

	EnsureEncounterTrigger();
	if (!bGameplayLoopTestMap)
	{
		EnsureReverieRunManager();
		if (bPCGDemoMap)
		{
			if (bPortfolioBezierMap)
			{
				ConfigurePortfolioBezierReverieManager();
			}
			else
			{
				ConfigurePCGDemoReverieManager();
			}
			StartPCGDemoRun();
		}
		EnsurePCGGameplayPlacement();
	}
	EnsureWorldInteractions();

	if (ShouldRunLoopVerifier() && !FindExistingActorOfClass(AMelodiaLoopVerifier::StaticClass()))
	{
		AMelodiaLoopVerifier* LoopVerifier = GetWorld()->SpawnActor<AMelodiaLoopVerifier>(AMelodiaLoopVerifier::StaticClass(), FVector(-10540.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
		if (LoopVerifier)
		{
			LoopVerifier->bRunOnBeginPlay = true;
		}
	}
	else if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AMelodiaLoopVerifier> It(World); It; ++It)
		{
			It->Destroy();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AMelodiaRhythmGameModeBase::FinishLoopBootstrap));
	}
}

void AMelodiaRhythmGameModeBase::NotifyBattleSessionBegan(AActor* BattleController)
{
	ActiveBattleController = BattleController;
	EnsureBattleInputBridge();
	PrepareMelodiaBattleView();
}

void AMelodiaRhythmGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bJRPGOnlyMode)
	{
		return;
	}

	AActor* BattleController = ActiveBattleController.Get();
	if (!BattleController)
	{
		return;
	}

	// When Phoenix marks battle over, force-return to exploration loop.
	if (const FBoolProperty* BattleOverProp = CastField<FBoolProperty>(BattleController->GetClass()->FindPropertyByName(TEXT("isBattleOver"))))
	{
		const bool bBattleOver = BattleOverProp->GetPropertyValue_InContainer(BattleController);
		if (bBattleOver)
		{
			PrepareExplorationPresentation();
			SetLoopPhase(EMelodiaLoopPhase::ExplorationReady);
			ActiveBattleController = nullptr;
		}
	}
}

void AMelodiaRhythmGameModeBase::SetLoopPhase(const EMelodiaLoopPhase NewPhase)
{
	CurrentLoopPhase = NewPhase;
	LastLoopPhaseText = GetLoopPhaseText();

	switch (NewPhase)
	{
	case EMelodiaLoopPhase::Battle:
		++BattlePhaseEntryCount;
		if (!bJRPGOnlyMode)
		{
			EnsureBattleMusicClock();
			EnsureBattleInputBridge();
			PrepareMelodiaBattleView();
			EnsureRhythmHUDWidget();
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(GetWorld()))
			{
				Widget->bDrawExplorationHUD = false;
				if (bHSRStyleBattle && !bUseRhythmHighway)
				{
					Widget->SetNoteHighwayActive(false, TArray<FMelodiaHighwayNote>(), 0.0f, 2.5f);
					Widget->ShowActionPrompt(TEXT("1=Attack | 2=Skill | R=Ultimate | Tab=cycle | 4/Esc=Flee"));
				}
			}
		}
		else
		{
			PrepareMelodiaBattleView();
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(GetWorld()))
			{
				Widget->bDrawExplorationHUD = false;
				Widget->ShowActionPrompt(TEXT("Phoenix JRPG battle (legacy mode)"));
			}
		}
		break;
	case EMelodiaLoopPhase::VictoryReward:
		++VictoryRewardPhaseCount;
		if (UWorld* World = GetWorld())
		{
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
			{
				Widget->bDrawExplorationHUD = false;
				Widget->ShowActionPrompt(TEXT("Victory — Space to continue now, or wait"));
			}
			World->GetTimerManager().SetTimer(
				VictoryAutoExitHandle,
				this,
				&AMelodiaRhythmGameModeBase::AutoConfirmVictoryIfPending,
				1.25f,
				false);
		}
		break;
	case EMelodiaLoopPhase::ExplorationReady:
		++ExplorationReadyPhaseCount;
		ActiveBattleController = nullptr;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(VictoryAutoExitHandle);
		}
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(GetWorld()))
		{
			Widget->bDrawExplorationHUD = true;
		}
		PrepareExplorationPresentation();
		RestoreExplorationControl();
		if (ActiveEncounterTrigger)
		{
			ActiveEncounterTrigger->ArmEncounter();
		}
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (APawn* ExplorationPawn = ActiveExplorationPawn.Get())
			{
				PlayerController->SetViewTargetWithBlend(ExplorationPawn, 0.35f);
			}
		}
		break;
	case EMelodiaLoopPhase::Bootstrapping:
	default:
		break;
	}
}

FString AMelodiaRhythmGameModeBase::GetLoopPhaseText() const
{
	switch (CurrentLoopPhase)
	{
	case EMelodiaLoopPhase::Battle:
		return TEXT("Battle");
	case EMelodiaLoopPhase::VictoryReward:
		return TEXT("VictoryReward");
	case EMelodiaLoopPhase::ExplorationReady:
		return TEXT("ExplorationReady");
	case EMelodiaLoopPhase::Bootstrapping:
	default:
		return TEXT("Bootstrapping");
	}
}

UClass* AMelodiaRhythmGameModeBase::ResolveClass(const FSoftClassPath& ClassPath) const
{
	if (!ClassPath.IsValid())
	{
		return nullptr;
	}

	UObject* LoadedObject = ClassPath.TryLoad();
	UClass* LoadedClass = Cast<UClass>(LoadedObject);
	if (!LoadedClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap could not load class: %s"), *ClassPath.ToString());
	}

	return LoadedClass;
}

AActor* AMelodiaRhythmGameModeBase::FindExistingActorOfClass(UClass* ActorClass) const
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

AActor* AMelodiaRhythmGameModeBase::SpawnLoopActor(UClass* ActorClass, const FVector& Location, const FRotator& Rotation) const
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return World->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParameters);
}

bool AMelodiaRhythmGameModeBase::ShouldRunLoopVerifier() const
{
	bool bCommandLineVerifier = false;
	FParse::Bool(FCommandLine::Get(), TEXT("MelodiaRunLoopVerifier="), bCommandLineVerifier);
	return bCommandLineVerifier && bRunLoopVerifier;
}

FVector AMelodiaRhythmGameModeBase::ResolveExplorationSpawnLocation() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return ExplorationReturnLocation;
	}

	if (bMinimalDemoMode)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			const FVector StartLocation = It->GetActorLocation();
			if (!StartLocation.IsNearlyZero(100.0f))
			{
				return StartLocation;
			}
		}
		return ExplorationReturnLocation;
	}

	const FVector PCGCenter = FindPCGWorldCenter();
	if (bPCGPlacementActive && ActiveWalkableIndex && ActiveWalkableIndex->GetCachedPointCount() > 0)
	{
		FVector WalkPos = FVector::ZeroVector;
		EPCGArchitecturalRole WalkRole = EPCGArchitecturalRole::None;
		FVector WalkNormal = FVector::UpVector;

		if (UClass* RhythmAnchorClass = ResolveClass(RhythmTestManagerClassPath))
		{
			if (const AActor* RhythmAnchor = FindExistingActorOfClass(RhythmAnchorClass))
			{
				if (ActiveWalkableIndex->FindNearestWalkable(
					RhythmAnchor->GetActorLocation(), PCGPlacementSearchRadius, WalkPos, WalkRole, WalkNormal))
				{
					return WalkPos + FVector(0.0f, 0.0f, 80.0f);
				}
			}
		}

		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			const FVector StartLocation = It->GetActorLocation();
			if (!StartLocation.IsNearlyZero(100.0f)
				&& ActiveWalkableIndex->FindNearestWalkable(
					StartLocation, PCGPlacementSearchRadius, WalkPos, WalkRole, WalkNormal))
			{
				return WalkPos + FVector(0.0f, 0.0f, 80.0f);
			}
		}

		if (ActiveWalkableIndex->FindRandomWalkable(
			PCGCenter, PCGPlacementSearchRadius * 0.5f, 101, WalkPos))
		{
			return WalkPos + FVector(0.0f, 0.0f, 80.0f);
		}
	}

	if (UClass* RhythmAnchorClass = ResolveClass(RhythmTestManagerClassPath))
	{
		if (const AActor* RhythmAnchor = FindExistingActorOfClass(RhythmAnchorClass))
		{
			return RhythmAnchor->GetActorLocation() + FVector(20.0f, 40.0f, 120.0f);
		}
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const FVector StartLocation = It->GetActorLocation();
		if (!StartLocation.IsNearlyZero(100.0f))
		{
			return StartLocation;
		}
	}

	return ExplorationReturnLocation;
}

FVector AMelodiaRhythmGameModeBase::ResolveEncounterTriggerLocation() const
{
	if (ActiveEncounterTrigger)
	{
		return ActiveEncounterTrigger->GetActorLocation();
	}

	if (bMinimalDemoMode || !bUsePCGPlacement)
	{
		return ExplorationReturnLocation + FVector(EncounterTriggerForwardOffset, 0.0f, 0.0f);
	}

	if (bPCGPlacementActive && ActiveWalkableIndex)
	{
		FVector WalkPos = FVector::ZeroVector;
		if (TryResolveWalkableLocation(
			FindPCGWorldCenter(), EPCGArchitecturalRole::Stair, PCGPlacementSearchRadius, WalkPos))
		{
			return WalkPos + FVector(0.0f, 0.0f, 5.0f);
		}
	}

	return ExplorationReturnLocation + FVector(EncounterTriggerForwardOffset, 0.0f, 0.0f);
}

void AMelodiaRhythmGameModeBase::SyncExplorationLocations()
{
	ExplorationReturnLocation = ResolveExplorationSpawnLocation();
	EncounterTriggerLocation = ResolveEncounterTriggerLocation();

	constexpr float MinSpawnGateSeparation = 350.0f;
	const float SpawnGateDistance2D = FVector::Dist2D(ExplorationReturnLocation, EncounterTriggerLocation);
	if (SpawnGateDistance2D < MinSpawnGateSeparation)
	{
		FVector AwayFromGate = ExplorationReturnLocation - EncounterTriggerLocation;
		AwayFromGate.Z = 0.0f;
		if (AwayFromGate.IsNearlyZero(10.0f))
		{
			AwayFromGate = FVector(-1.0f, 0.0f, 0.0f);
		}
		AwayFromGate.Normalize();
		ExplorationReturnLocation = EncounterTriggerLocation + AwayFromGate * MinSpawnGateSeparation;
		ExplorationReturnLocation.Z = ResolveExplorationSpawnLocation().Z;
	}

	if (ActiveEncounterTrigger && !bPCGPlacementActive && !bPreferLevelPlacedLoopActors)
	{
		ActiveEncounterTrigger->SetActorLocation(EncounterTriggerLocation);
	}
}

void AMelodiaRhythmGameModeBase::FinishLoopBootstrap()
{
	EnsurePCGGameplayPlacement();
	SyncExplorationLocations();
	EnsureRhythmHUDWidget();
	EnsureCompanionActor();
	EnsureProgressionNPCs();
	EnsureWorldInteractions();
	EnsurePortfolioFlowers();
	SetLoopPhase(EMelodiaLoopPhase::ExplorationReady);

	if (AMelodiaQuestManagerBase* QuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(ResolveClass(QuestManagerClassPath))))
	{
		QuestManager->RegisterStarterQuests(EncounterTriggerLocation);
	}
	else if (AMelodiaQuestManagerBase* NativeQuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(AMelodiaQuestManagerBase::StaticClass())))
	{
		NativeQuestManager->RegisterStarterQuests(EncounterTriggerLocation);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			if (UMelodiaSaveGame* SaveData = Cast<UMelodiaSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("MelodiaSlot"), 0)))
			{
				Progression->LoadFromSave(SaveData);
			}
			Progression->NotifyQuestSystemOfProgress(GetWorld());
			Progression->ApplyUnlockedPresetsToReverieRunManager(ActiveReverieRunManager);
			Progression->SyncHUD(GetWorld());
		}
	}

	if (!bExplorationControlReady)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				BootstrapRetryHandle,
				this,
				&AMelodiaRhythmGameModeBase::RetryExplorationBootstrap,
				0.15f,
				false);
		}
	}
}

void AMelodiaRhythmGameModeBase::RetryExplorationBootstrap()
{
	EnsureRhythmHUDWidget();
	RestoreExplorationControl();

	if (!bExplorationControlReady)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap retry failed: player still has no exploration pawn."));
	}
}

void AMelodiaRhythmGameModeBase::RestoreExplorationControl()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SyncExplorationLocations();

	APawn* ExplorationPawn = ActiveExplorationPawn.Get();
	if (!ExplorationPawn)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			ExplorationPawn = PlayerController->GetPawn();
		}
	}

	if (!ExplorationPawn)
	{
		UClass* ExplorationPawnClass = ResolveClass(ExplorationPawnClassPath);
		if (ExplorationPawnClass)
		{
			ExplorationPawn = Cast<APawn>(FindExistingActorOfClass(ExplorationPawnClass));
			if (!ExplorationPawn)
			{
				ExplorationPawn = World->SpawnActor<APawn>(ExplorationPawnClass, ExplorationReturnLocation, FRotator::ZeroRotator);
			}
		}
	}

	if (!ExplorationPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop could not restore exploration control: no exploration pawn."));
		return;
	}

	ActiveExplorationPawn = ExplorationPawn;
	ExplorationPawn->SetActorLocation(ExplorationReturnLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (AMelodiaCharacterBase* MelusinaCharacter = Cast<AMelodiaCharacterBase>(ExplorationPawn))
	{
		MelusinaCharacter->InitializeExplorationSystems();
	}
	else
	{
		if (!ExplorationPawn->FindComponentByClass<UMelodiaInventoryComponent>())
		{
			if (UMelodiaInventoryComponent* Inventory = NewObject<UMelodiaInventoryComponent>(ExplorationPawn, UMelodiaInventoryComponent::StaticClass(), TEXT("MelodiaInventory")))
			{
				Inventory->RegisterComponent();
				ExplorationPawn->AddInstanceComponent(Inventory);
				Inventory->SeedStarterKit();
			}
		}

		if (!ExplorationPawn->FindComponentByClass<UMelodiaExplorationInputComponent>())
		{
			if (UMelodiaExplorationInputComponent* ExplorationInput = NewObject<UMelodiaExplorationInputComponent>(ExplorationPawn, UMelodiaExplorationInputComponent::StaticClass(), TEXT("MelodiaExplorationInput")))
			{
				ExplorationInput->RegisterComponent();
				ExplorationPawn->AddInstanceComponent(ExplorationInput);
				ExplorationInput->BindExplorationInput();
			}
		}
	}

	ApplyMelusinaPresentation(ExplorationPawn);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		PlayerController->Possess(ExplorationPawn);
		PlayerController->SetViewTarget(ExplorationPawn);
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetIgnoreMoveInput(false);
		bExplorationControlReady = PlayerController->GetPawn() == ExplorationPawn;
		if (bExplorationControlReady)
		{
			++ExplorationControlRestoreCount;
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
			{
				Widget->ShowBattleStatus(TEXT("Explore: touch the song gate"));
			}

			UE_LOG(LogTemp, Log, TEXT("Melodia loop restored exploration control to %s."), *ExplorationPawn->GetName());
		}
	}

	for (TActorIterator<AMelodiaQuestManagerBase> QuestIt(World); QuestIt; ++QuestIt)
	{
		QuestIt->SyncHUD(World);
		break;
	}
}

void AMelodiaRhythmGameModeBase::ApplyMelusinaPresentation(APawn* ExplorationPawn)
{
	if (!ExplorationPawn)
	{
		return;
	}

	bMelusinaPawnActive = ExplorationPawn->GetClass()->GetName().Contains(TEXT("Melusina"))
		|| ExplorationPawn->IsA<AMelodiaCharacterBase>();
	if (bMelusinaPawnActive)
	{
		++MelusinaPawnApplyCount;
	}

	if (AMelodiaCharacterBase* MelusinaCharacter = Cast<AMelodiaCharacterBase>(ExplorationPawn))
	{
		MelusinaCharacter->ApplyMelusinaPresentation();
		ActiveCosmeticsComponent = MelusinaCharacter->GetCosmeticsComponent();
		if (ActiveCosmeticsComponent)
		{
			LastCosmeticPresetText = ActiveCosmeticsComponent->LastAppliedPresetId.ToString();
		}
		EnsureRhythmHUDWidget();
		return;
	}

	UMelodiaCosmeticsComponent* CosmeticsComponent = ExplorationPawn->FindComponentByClass<UMelodiaCosmeticsComponent>();
	if (!CosmeticsComponent)
	{
		CosmeticsComponent = NewObject<UMelodiaCosmeticsComponent>(ExplorationPawn, UMelodiaCosmeticsComponent::StaticClass(), TEXT("MelodiaCosmetics"));
		if (CosmeticsComponent)
		{
			CosmeticsComponent->RegisterComponent();
			ExplorationPawn->AddInstanceComponent(CosmeticsComponent);
		}
	}

	ActiveCosmeticsComponent = CosmeticsComponent;
	if (ActiveCosmeticsComponent && ActiveCosmeticsComponent->ApplyDefaultMelusinaPreset())
	{
		LastCosmeticPresetText = ActiveCosmeticsComponent->LastAppliedPresetId.ToString();
	}

	EnsureRhythmHUDWidget();
	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->ApplyCuteCombatTheme();
			Widget->TriggerSparkleBurst();
		}
	}
}

void AMelodiaRhythmGameModeBase::RemoveStaleRhythmHUDWidgets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UClass* RhythmHUDActorClass = ResolveClass(RhythmHUDActorClassPath))
	{
		TArray<AActor*> LegacyActors;
		for (TActorIterator<AActor> It(World, RhythmHUDActorClass); It; ++It)
		{
			LegacyActors.Add(*It);
		}
		for (AActor* Actor : LegacyActors)
		{
			if (Actor)
			{
				Actor->Destroy();
			}
		}
	}

	TArray<UMelodiaRhythmHUDWidget*> ExistingWidgets;
	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		if (It->GetWorld() == World)
		{
			ExistingWidgets.Add(*It);
		}
	}

	for (UMelodiaRhythmHUDWidget* Widget : ExistingWidgets)
	{
		if (Widget)
		{
			Widget->RemoveFromParent();
		}
	}

	ActiveRhythmHUDWidget = nullptr;
	bRhythmHUDWidgetInViewport = false;
}

void AMelodiaRhythmGameModeBase::EnsureRhythmHUDWidget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ActiveRhythmHUDWidget)
	{
		ActiveRhythmHUDWidget->EnforceNativeHSRPresentation();
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		bRhythmHUDWidgetInViewport = true;
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop could not add rhythm HUD widget yet: no player controller."));
		return;
	}

	if (bHSRStyleBattle)
	{
		RemoveStaleRhythmHUDWidgets();
	}

	UClass* WidgetClass = nullptr;
	if (bHSRStyleBattle)
	{
		WidgetClass = UMelodiaRhythmHUDWidget::StaticClass();
	}
	else
	{
		WidgetClass = ResolveClass(RhythmHUDWidgetClassPath);
		if (!WidgetClass)
		{
			WidgetClass = UMelodiaRhythmHUDWidget::StaticClass();
			UE_LOG(LogTemp, Warning, TEXT("Melodia loop fell back to native UMelodiaRhythmHUDWidget."));
		}
	}

	ActiveRhythmHUDWidget = CreateWidget<UMelodiaRhythmHUDWidget>(PlayerController, WidgetClass);
	if (ActiveRhythmHUDWidget)
	{
		ActiveRhythmHUDWidget->AddToViewport(40);
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ActiveRhythmHUDWidget->bDrawExplorationHUD = CurrentLoopPhase != EMelodiaLoopPhase::Battle;
		ActiveRhythmHUDWidget->EnforceNativeHSRPresentation();
		ActiveRhythmHUDWidget->TriggerSparkleBurst();
		bRhythmHUDWidgetInViewport = true;
		UE_LOG(LogTemp, Log, TEXT("Melodia loop added native rhythm HUD widget to viewport (class=%s)."),
			*WidgetClass->GetName());
	}
}

UMelodiaRhythmHUDWidget* AMelodiaRhythmGameModeBase::EnsureActiveRhythmHUD()
{
	EnsureRhythmHUDWidget();
	return ActiveRhythmHUDWidget;
}

void AMelodiaRhythmGameModeBase::EnsureQuestManager()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (FindExistingActorOfClass(ResolveClass(QuestManagerClassPath))
		|| FindExistingActorOfClass(AMelodiaQuestManagerBase::StaticClass()))
	{
		return;
	}

	if (UClass* QuestManagerClass = ResolveClass(QuestManagerClassPath))
	{
		SpawnLoopActor(QuestManagerClass, FVector(-10570.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
		return;
	}

	World->SpawnActor<AMelodiaQuestManagerBase>(
		AMelodiaQuestManagerBase::StaticClass(),
		FVector(-10570.0f, -5000.0f, -2140.0f),
		FRotator::ZeroRotator);
	UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap fell back to native AMelodiaQuestManagerBase."));
}

void AMelodiaRhythmGameModeBase::EnsureCompanionActor()
{
	if (ActiveCompanion)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaCompanionActor> It(World); It; ++It)
	{
		ActiveCompanion = *It;
		return;
	}

	UClass* CompanionClass = ResolveClass(CompanionClassPath);
	if (!CompanionClass)
	{
		CompanionClass = AMelodiaCompanionActor::StaticClass();
	}

	SyncExplorationLocations();
	const FVector SpawnLocation = ExplorationReturnLocation + FVector(-120.0f, 140.0f, 80.0f);
	ActiveCompanion = Cast<AMelodiaCompanionActor>(SpawnLoopActor(CompanionClass, SpawnLocation, FRotator::ZeroRotator));
	if (!ActiveCompanion)
	{
		ActiveCompanion = World->SpawnActor<AMelodiaCompanionActor>(
			AMelodiaCompanionActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator);
	}

	if (ActiveCompanion)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				ActiveCompanion->SetCompanionUnlocked(Progression->State.bCompanionUnlocked);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned companion at %s."), *SpawnLocation.ToString());
	}
}

void AMelodiaRhythmGameModeBase::EnsureProgressionNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaNPCBase> It(World); It; ++It)
	{
		ActiveProgressionNPCs.AddUnique(*It);
	}

	if (bGameplayLoopTestMap)
	{
		return;
	}

	if (ActiveProgressionNPCs.Num() >= 3)
	{
		return;
	}

	UClass* NPCClass = ResolveClass(ProgressionNPCClassPath);
	if (!NPCClass)
	{
		NPCClass = AMelodiaNPCBase::StaticClass();
	}

	SyncExplorationLocations();
	struct FNPROw
	{
		FVector Offset;
		FName QuestId;
		FString Name;
		FString Dialogue;
		int32 ReqLevel;
	};

	const FNPROw Rows[3] = {
		{ FVector(200.0f, -220.0f, 0.0f), TEXT("Tutor_TierII"), TEXT("Moon Tutor"), TEXT("Tier II skills bloom at Lv6. Keep composing!"), 1 },
		{ FVector(-240.0f, 200.0f, 0.0f), TEXT("Tutor_TierIV"), TEXT("Aurora Tutor"), TEXT("Harmonic keys shine brightest at Lv16."), 11 },
		{ FVector(300.0f, 260.0f, 0.0f), TEXT("Tutor_TierVI"), TEXT("Celestial Tutor"), TEXT("The Maestro Sanctum awaits at Lv26."), 21 },
	};

	for (const FNPROw& Row : Rows)
	{
		if (ActiveProgressionNPCs.Num() >= 3)
		{
			break;
		}

		AMelodiaNPCBase* NPC = Cast<AMelodiaNPCBase>(SpawnLoopActor(
			NPCClass,
			ExplorationReturnLocation + Row.Offset + FVector(0.0f, 0.0f, 40.0f),
			FRotator(0.0f, 180.0f, 0.0f)));
		if (!NPC)
		{
			NPC = World->SpawnActor<AMelodiaNPCBase>(
				AMelodiaNPCBase::StaticClass(),
				ExplorationReturnLocation + Row.Offset + FVector(0.0f, 0.0f, 40.0f),
				FRotator(0.0f, 180.0f, 0.0f));
		}
		if (NPC)
		{
			NPC->QuestId = Row.QuestId;
			NPC->DisplayName = Row.Name;
			NPC->DialogueLine = Row.Dialogue;
			NPC->RequiredMechanicLevel = Row.ReqLevel;
			NPC->InteractionPrompt = TEXT("F: Talk to tutor");
			ActiveProgressionNPCs.Add(NPC);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned %d progression tutor NPCs."), ActiveProgressionNPCs.Num());
}

void AMelodiaRhythmGameModeBase::EnsureEncounterTrigger()
{
	SyncExplorationLocations();

	if (bPCGPlacementActive && ActivePCGEncounterSpawner && ActivePCGEncounterSpawner->GetSpawnedActors().Num() > 0)
	{
		if (AMelodiaEncounterTrigger* PCGTrigger = Cast<AMelodiaEncounterTrigger>(ActivePCGEncounterSpawner->GetSpawnedActors()[0]))
		{
			ActiveEncounterTrigger = PCGTrigger;
			EncounterTriggerLocation = PCGTrigger->GetActorLocation();
			if (CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady)
			{
				ActiveEncounterTrigger->ArmEncounter();
			}
			return;
		}
	}

	if (!ActiveEncounterTrigger)
	{
		if (ActiveTestLoopDirector && ActiveTestLoopDirector->EncounterGate)
		{
			ActiveEncounterTrigger = ActiveTestLoopDirector->EncounterGate;
		}
	}

	if (!ActiveEncounterTrigger)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			for (TActorIterator<AMelodiaEncounterTrigger> It(World); It; ++It)
			{
				if (It->ActorHasTag(TEXT("Melodia.TestLoop.Enemy")))
				{
					continue;
				}
				ActiveEncounterTrigger = *It;
				break;
			}
		}
	}

	if (!ActiveEncounterTrigger)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveEncounterTrigger = World->SpawnActor<AMelodiaEncounterTrigger>(AMelodiaEncounterTrigger::StaticClass(), EncounterTriggerLocation, FRotator::ZeroRotator, SpawnParameters);
		if (ActiveEncounterTrigger)
		{
			UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned encounter trigger at %s."), *EncounterTriggerLocation.ToString());
		}
	}

	if (ActiveEncounterTrigger)
	{
		if (!bPCGPlacementActive && !bPreferLevelPlacedLoopActors)
		{
			ActiveEncounterTrigger->SetActorLocation(EncounterTriggerLocation);
		}
		if (CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady)
		{
			ActiveEncounterTrigger->ArmEncounter();
		}
	}
}

void AMelodiaRhythmGameModeBase::EnsureReverieRunManager()
{
	if (ActiveReverieRunManager)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find an existing ReverieRunManager in the world.
	for (TActorIterator<AMelodiaReverieRunManager> It(World); It; ++It)
	{
		ActiveReverieRunManager = *It;
		return;
	}

	// Spawn a new one near the exploration area.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveReverieRunManager = World->SpawnActor<AMelodiaReverieRunManager>(
		AMelodiaReverieRunManager::StaticClass(),
		ExplorationReturnLocation + FVector(0.0f, 200.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParams);

	if (ActiveReverieRunManager)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned ReverieRunManager at %s."),
			*ActiveReverieRunManager->GetActorLocation().ToString());
	}
}

void AMelodiaRhythmGameModeBase::EnsureWorldInteractions()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SyncExplorationLocations();

	FVector RestLocation = ExplorationReturnLocation + FVector(-180.0f, -160.0f, 0.0f);
	FVector PortalLocation = ExplorationReturnLocation + FVector(220.0f, 180.0f, 0.0f);

	if (bPCGPlacementActive && ActiveWalkableIndex)
	{
		FVector PCGRest = FVector::ZeroVector;
		if (TryResolveWalkableLocation(
			ExplorationReturnLocation, EPCGArchitecturalRole::Floor, PCGPlacementSearchRadius * 0.75f, PCGRest))
		{
			RestLocation = PCGRest + FVector(0.0f, 0.0f, 5.0f);
		}

		FVector PCGPortal = FVector::ZeroVector;
		if (TryResolveWalkableLocation(
			EncounterTriggerLocation, EPCGArchitecturalRole::Door, PCGPlacementSearchRadius * 0.75f, PCGPortal))
		{
			PortalLocation = PCGPortal + FVector(0.0f, 0.0f, 5.0f);
		}
		else if (TryResolveWalkableLocation(
			EncounterTriggerLocation, EPCGArchitecturalRole::Tile, PCGPlacementSearchRadius * 0.75f, PCGPortal))
		{
			PortalLocation = PCGPortal + FVector(0.0f, 0.0f, 5.0f);
		}
	}

	if (!ActiveRestPoint)
	{
		for (TActorIterator<AMelodiaRestPoint> It(World); It; ++It)
		{
			ActiveRestPoint = *It;
			break;
		}
	}

	if (!ActiveRestPoint)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveRestPoint = World->SpawnActor<AMelodiaRestPoint>(AMelodiaRestPoint::StaticClass(), RestLocation, FRotator::ZeroRotator, SpawnParams);
		if (ActiveRestPoint)
		{
			UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned rest point at %s."), *RestLocation.ToString());
		}
	}
	else if (bPCGPlacementActive)
	{
		ActiveRestPoint->SetActorLocation(RestLocation);
	}

	if (!ActivePortal)
	{
		for (TActorIterator<AMelodiaPortal> It(World); It; ++It)
		{
			ActivePortal = *It;
			break;
		}
	}

	if (!ActivePortal)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActivePortal = World->SpawnActor<AMelodiaPortal>(AMelodiaPortal::StaticClass(), PortalLocation, FRotator::ZeroRotator, SpawnParams);
		if (ActivePortal)
		{
			UE_LOG(LogTemp, Log, TEXT("Melodia loop spawned portal at %s."), *PortalLocation.ToString());
		}
	}
	else if (bPCGPlacementActive)
	{
		ActivePortal->SetActorLocation(PortalLocation);
	}

	if (ActivePortal && !bPreferLevelPlacedLoopActors)
	{
		if (bPCGPlacementActive)
		{
			ActivePortal->TargetWorldLocation = EncounterTriggerLocation;
		}
		else
		{
			ActivePortal->TargetWorldLocation = EncounterTriggerLocation + FVector(-180.0f, 0.0f, 0.0f);
		}
		ActivePortal->bUseTargetWorldLocation = true;
	}
}

void AMelodiaRhythmGameModeBase::EnsurePortfolioFlowers()
{
	if (bGameplayLoopTestMap)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 ExistingCount = 0;
	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		++ExistingCount;
	}
	if (ExistingCount >= 3)
	{
		return;
	}

	SyncExplorationLocations();
	const FVector Origin = ExplorationReturnLocation;
	const FLinearColor Tints[] = {
		FLinearColor(0.98f, 0.52f, 0.86f, 1.0f),
		FLinearColor(0.72f, 0.62f, 0.98f, 1.0f),
		FLinearColor(0.98f, 0.82f, 0.42f, 1.0f),
		FLinearColor(0.52f, 0.92f, 0.78f, 1.0f),
		FLinearColor(0.98f, 0.68f, 0.52f, 1.0f),
	};

	const int32 SpawnCount = 6;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const float Angle = static_cast<float>(Index) * (360.0f / static_cast<float>(SpawnCount));
		const float Radius = 280.0f + static_cast<float>(Index) * 40.0f;
		const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius, FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius, 0.0f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AMelodiaPickableFlower* Flower = World->SpawnActor<AMelodiaPickableFlower>(
			AMelodiaPickableFlower::StaticClass(),
			Origin + Offset,
			FRotator(0.0f, Angle, 0.0f),
			Params);
		if (Flower)
		{
			Flower->BloomTint = Tints[Index % UE_ARRAY_COUNT(Tints)];
			Flower->DisplayName = FString::Printf(TEXT("Reverie Blossom %d"), Index + 1);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia portfolio: spawned pickable flowers around %s."), *Origin.ToString());
}

UPCGComponent* AMelodiaRhythmGameModeBase::FindPrimaryPCGComponent() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UPCGComponent* BestComp = nullptr;
	float BestVolume = 0.0f;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp || !PCGComp->GetGraph())
		{
			continue;
		}

		const FVector Extent = It->GetComponentsBoundingBox().GetExtent();
		const float Volume = Extent.X * Extent.Y * Extent.Z;
		if (!BestComp || Volume > BestVolume)
		{
			BestComp = PCGComp;
			BestVolume = Volume;
		}
	}

	return BestComp;
}

FVector AMelodiaRhythmGameModeBase::FindPCGWorldCenter() const
{
	if (const UPCGComponent* PCGComp = FindPrimaryPCGComponent())
	{
		if (const AActor* PCGOwner = PCGComp->GetOwner())
		{
			return PCGOwner->GetActorLocation();
		}
	}

	return ExplorationReturnLocation;
}

bool AMelodiaRhythmGameModeBase::TryResolveWalkableLocation(
	const FVector& Hint,
	const EPCGArchitecturalRole PreferredRole,
	const float SearchRadius,
	FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const TArray<FMelodiaWalkablePoint> RolePoints =
		UMelodiaPCGLibrary::GetWalkablePointsByRoleInRadius(World, Hint, SearchRadius, PreferredRole);
	if (RolePoints.Num() > 0)
	{
		float BestDistSq = FLT_MAX;
		for (const FMelodiaWalkablePoint& Point : RolePoints)
		{
			const float DistSq = FVector::DistSquared(Point.Location, Hint);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				OutLocation = Point.Location;
			}
		}
		return true;
	}

	if (ActiveWalkableIndex && ActiveWalkableIndex->GetCachedPointCount() > 0)
	{
		EPCGArchitecturalRole OutRole = EPCGArchitecturalRole::None;
		FVector OutNormal = FVector::UpVector;
		if (ActiveWalkableIndex->FindNearestWalkable(Hint, SearchRadius, OutLocation, OutRole, OutNormal))
		{
			return true;
		}
	}

	return false;
}

void AMelodiaRhythmGameModeBase::EnsurePCGGameplayPlacement()
{
	if (!bUsePCGPlacement)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UPCGComponent* PCGComp = FindPrimaryPCGComponent();
	if (!PCGComp)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Melodia PCG bootstrap: no PCG component in level; using fallback placement."));
		return;
	}

	const FVector PCGCenter = FindPCGWorldCenter();

	if (!ActiveWalkableIndex)
	{
		ActiveWalkableIndex = UMelodiaPCGLibrary::FindWalkableIndex(World);
	}

	if (!ActiveWalkableIndex)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveWalkableIndex = World->SpawnActor<AMelodiaPCGWalkableIndex>(
			AMelodiaPCGWalkableIndex::StaticClass(), PCGCenter, FRotator::ZeroRotator, SpawnParams);
		if (ActiveWalkableIndex)
		{
			ActiveWalkableIndex->DiscoveryRadius = PCGPlacementSearchRadius;
			UE_LOG(LogTemp, Log, TEXT("Melodia PCG bootstrap spawned WalkableIndex at %s."), *PCGCenter.ToString());
		}
	}
	else
	{
		ActiveWalkableIndex->SetActorLocation(PCGCenter);
	}

	if (ActiveWalkableIndex)
	{
		ActiveWalkableIndex->RebuildCache();
		PCGWalkablePointCount = ActiveWalkableIndex->GetCachedPointCount();
	}

	if (PCGWalkablePointCount <= 0)
	{
		if (PCGPlacementRetryCount < MaxPCGPlacementRetries)
		{
			++PCGPlacementRetryCount;
			World->GetTimerManager().SetTimer(
				PCGPlacementRetryHandle,
				this,
				&AMelodiaRhythmGameModeBase::RetryPCGGameplayPlacement,
				0.35f,
				false);
			UE_LOG(LogTemp, Verbose,
				TEXT("Melodia PCG bootstrap: waiting for walkable cache (retry %d/%d)."),
				PCGPlacementRetryCount, MaxPCGPlacementRetries);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Melodia PCG bootstrap: no walkable points after %d retries; using fallback placement."),
				MaxPCGPlacementRetries);
		}
		return;
	}

	ApplyPCGPlacedInteractables();
}

void AMelodiaRhythmGameModeBase::RetryPCGGameplayPlacement()
{
	EnsurePCGGameplayPlacement();
	if (bPCGPlacementActive)
	{
		SyncExplorationLocations();
		EnsureEncounterTrigger();
		EnsureWorldInteractions();
	}
}

void AMelodiaRhythmGameModeBase::ApplyPCGPlacedInteractables()
{
	UWorld* World = GetWorld();
	if (!World || !ActiveWalkableIndex || ActiveWalkableIndex->GetCachedPointCount() <= 0)
	{
		return;
	}

	const FVector PCGCenter = FindPCGWorldCenter();
	PCGWalkablePointCount = ActiveWalkableIndex->GetCachedPointCount();

	if (!ActivePCGEncounterSpawner)
	{
		ActivePCGEncounterSpawner = UMelodiaPCGLibrary::FindEncounterSpawner(World);
	}

	if (!ActivePCGEncounterSpawner)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActivePCGEncounterSpawner = World->SpawnActor<AMelodiaPCGEncounterSpawner>(
			AMelodiaPCGEncounterSpawner::StaticClass(), PCGCenter, FRotator::ZeroRotator, SpawnParams);
	}

	if (ActivePCGEncounterSpawner)
	{
		ActivePCGEncounterSpawner->WalkableIndex = ActiveWalkableIndex;
		ActivePCGEncounterSpawner->SearchRadiusCm = PCGPlacementSearchRadius;
		ActivePCGEncounterSpawner->SetActorLocation(PCGCenter);
		ActivePCGEncounterSpawner->ClearSpawnedActors();
		PCGSpawnedEncounterCount = ActivePCGEncounterSpawner->ScanAndSpawn();
	}

	bPCGPlacementActive = PCGSpawnedEncounterCount > 0 || PCGWalkablePointCount > 0;
	ExplorationReturnLocation = ResolveExplorationSpawnLocation();
	EncounterTriggerLocation = ResolveEncounterTriggerLocation();

	EnsureEncounterTrigger();
	EnsureWorldInteractions();

	UE_LOG(LogTemp, Log,
		TEXT("Melodia PCG bootstrap: %d walkable points, %d PCG encounters, spawn=%s, gate=%s."),
		PCGWalkablePointCount,
		PCGSpawnedEncounterCount,
		*ExplorationReturnLocation.ToString(),
		*EncounterTriggerLocation.ToString());
}

void AMelodiaRhythmGameModeBase::EnsureBattleMusicClock()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!ActiveMusicManager)
	{
		UClass* MusicClass = ResolveClass(QuartzMusicManagerClassPath);
		if (!MusicClass)
		{
			MusicClass = AMelodiaMusicManager::StaticClass();
		}

		ActiveMusicManager = Cast<AMelodiaMusicManager>(FindExistingActorOfClass(MusicClass));
		if (!ActiveMusicManager)
		{
			ActiveMusicManager = Cast<AMelodiaMusicManager>(SpawnLoopActor(MusicClass, FVector::ZeroVector, FRotator::ZeroRotator));
		}
	}

	if (ActiveMusicManager)
	{
		ActiveMusicManager->StartBattleMusic(nullptr, DefaultBattleBPM);
	}
}

void AMelodiaRhythmGameModeBase::ConfigureGameplayLoopTest(const FVector& PlayerSpawnLocation, const FVector& GateLocation)
{
	bGameplayLoopTestMap = true;
	bPCGDemoMap = false;
	bPreferLevelPlacedLoopActors = true;
	bUsePCGPlacement = false;
	ExplorationReturnLocation = PlayerSpawnLocation;
	EncounterTriggerLocation = GateLocation;
}

void AMelodiaRhythmGameModeBase::ConfigurePCGDemo()
{
	bPCGDemoMap = true;
	bPortfolioBezierMap = false;
	bGameplayLoopTestMap = false;
	bPreferLevelPlacedLoopActors = false;
	bUsePCGPlacement = true;
	bMinimalDemoMode = false;
	ExplorationReturnLocation = FVector(0.0f, 0.0f, 120.0f);
	EncounterTriggerLocation = FVector(800.0f, 0.0f, 120.0f);
	PCGPlacementSearchRadius = 20000.0f;
}

void AMelodiaRhythmGameModeBase::ConfigurePortfolioBezierDemo()
{
	ConfigurePCGDemo();
	bPortfolioBezierMap = true;
	ExplorationReturnLocation = FVector(-2400.0f, -1800.0f, 140.0f);
	EncounterTriggerLocation = FVector(2800.0f, 2000.0f, 320.0f);
}

void AMelodiaRhythmGameModeBase::NotifyReverieAreaGenerationComplete()
{
	if (!bUsePCGPlacement)
	{
		return;
	}

	RetryPCGGameplayPlacement();
	SyncExplorationLocations();
	EnsureWorldInteractions();
	EnsurePortfolioFlowers();

	if (AMelodiaQuestManagerBase* QuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(ResolveClass(QuestManagerClassPath))))
	{
		QuestManager->RegisterStarterQuests(EncounterTriggerLocation);
	}
	else if (AMelodiaQuestManagerBase* NativeQuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(AMelodiaQuestManagerBase::StaticClass())))
	{
		NativeQuestManager->RegisterStarterQuests(EncounterTriggerLocation);
	}
}

void AMelodiaRhythmGameModeBase::ConfigurePCGDemoReverieManager()
{
	if (!ActiveReverieRunManager)
	{
		return;
	}

	ActiveReverieRunManager->bAutoGenerateOnBeginPlay = false;
	ActiveReverieRunManager->AreasPerRun = 1;
	ActiveReverieRunManager->RunSeed = 0;

	if (ActiveReverieRunManager->AreaTemplates.IsEmpty())
	{
		FReverieAreaConfig TerraceGarden;
		TerraceGarden.AreaDisplayName = TEXT("星灯 Terrace Garden");
		TerraceGarden.PCGGraphAsset = FSoftObjectPath(
			TEXT("/Game/_PROJECT/PCG/Graphs/PCG_TerraceGarden.PCG_TerraceGarden"));
		TerraceGarden.MinEncounters = 1;
		TerraceGarden.MaxEncounters = 2;
		TerraceGarden.DifficultyMultiplier = 1.0f;
		ActiveReverieRunManager->AreaTemplates.Add(TerraceGarden);
	}
}

void AMelodiaRhythmGameModeBase::ConfigurePortfolioBezierReverieManager()
{
	if (!ActiveReverieRunManager)
	{
		return;
	}

	ActiveReverieRunManager->bAutoGenerateOnBeginPlay = false;
	ActiveReverieRunManager->AreasPerRun = 1;
	ActiveReverieRunManager->RunSeed = 42;

	FReverieAreaConfig PortfolioTerrace;
	PortfolioTerrace.AreaDisplayName = TEXT("Portfolio Bezier Terrace");
	PortfolioTerrace.PCGGraphAsset = FSoftObjectPath(
		TEXT("/Game/_PROJECT/PCG/Graphs/PCG_PortfolioTerraceBezier.PCG_PortfolioTerraceBezier"));
	PortfolioTerrace.MinEncounters = 1;
	PortfolioTerrace.MaxEncounters = 1;
	PortfolioTerrace.DifficultyMultiplier = 1.0f;

	ActiveReverieRunManager->AreaTemplates.Reset();
	ActiveReverieRunManager->AreaTemplates.Add(PortfolioTerrace);
}

void AMelodiaRhythmGameModeBase::StartPCGDemoRun()
{
	if (!bPCGDemoMap || !ActiveReverieRunManager)
	{
		return;
	}

	if (ActiveReverieRunManager->RunState == EReverieRunState::Idle)
	{
		ActiveReverieRunManager->StartRun(ActiveReverieRunManager->RunSeed);
	}
}

void AMelodiaRhythmGameModeBase::EnsureGameplayLoopTestDirector()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaGameplayLoopTestDirector> It(World); It; ++It)
	{
		ActiveTestLoopDirector = *It;
		break;
	}

	if (!ActiveTestLoopDirector)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveTestLoopDirector = World->SpawnActor<AMelodiaGameplayLoopTestDirector>(
			AMelodiaGameplayLoopTestDirector::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
	}

	if (!ActiveTestLoopDirector)
	{
		return;
	}

	if (!ActiveTestLoopDirector->bLayoutBuilt)
	{
		ActiveTestLoopDirector->BuildLayout();
	}

	ActiveTestLoopDirector->ApplyToGameMode(this);
	UE_LOG(LogTemp, Log, TEXT("Melodia gameplay loop test director active (spawn=%s gate=%s)."),
		*ExplorationReturnLocation.ToString(),
		*EncounterTriggerLocation.ToString());
}

void AMelodiaRhythmGameModeBase::SanitizeWorldForMinimalDemo()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaLoopVerifier> It(World); It; ++It)
	{
		It->Destroy();
	}

	TArray<AMelodiaEncounterTrigger*> ExistingTriggers;
	for (TActorIterator<AMelodiaEncounterTrigger> It(World); It; ++It)
	{
		ExistingTriggers.Add(*It);
	}
	for (AMelodiaEncounterTrigger* Trigger : ExistingTriggers)
	{
		if (Trigger)
		{
			Trigger->Destroy();
		}
	}
	ActiveEncounterTrigger = nullptr;

	if (UClass* RhythmTestManagerClass = ResolveClass(RhythmTestManagerClassPath))
	{
		TArray<AActor*> RhythmManagers;
		for (TActorIterator<AActor> It(World, RhythmTestManagerClass); It; ++It)
		{
			RhythmManagers.Add(*It);
		}
		for (AActor* Manager : RhythmManagers)
		{
			if (Manager)
			{
				Manager->Destroy();
			}
		}
	}

	if (UClass* BattleControllerClass = ResolveClass(BattleControllerClassPath))
	{
		for (TActorIterator<AActor> It(World, BattleControllerClass); It; ++It)
		{
			UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(*It, EMelodiaPhoenixTeardownScope::Full);
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}
}

void AMelodiaRhythmGameModeBase::PrepareMelodiaBattleView()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	AActor* BattleController = ActiveBattleController.Get();
	if (!BattleController)
	{
		BattleController = FindExistingActorOfClass(ResolveClass(BattleControllerClassPath));
	}

	if (bKeepPhoenixBattlePresentation && BattleController)
	{
		UMelodiaJRPGPresenter::PrepareBattlePresentation(
			this,
			BattleController,
			bSuppressPhoenixBattleUI);
	}
	else
	{
		APawn* ExplorationPawn = Cast<APawn>(PlayerController->GetPawn());
		if (!ExplorationPawn)
		{
			ExplorationPawn = ActiveExplorationPawn.Get();
		}
		if (!ExplorationPawn)
		{
			ExplorationPawn = Cast<APawn>(FindExistingActorOfClass(ResolveClass(ExplorationPawnClassPath)));
		}
		if (ExplorationPawn)
		{
			ActiveExplorationPawn = ExplorationPawn;
			PlayerController->Possess(ExplorationPawn);
			PlayerController->SetViewTarget(ExplorationPawn);
		}

		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->bShowMouseCursor = false;

		if (BattleController && bSuppressPhoenixBattleUI)
		{
			UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(BattleController, EMelodiaPhoenixTeardownScope::WidgetsOnly);
		}
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		if (bHSRStyleBattle && !bUseRhythmHighway)
		{
			Widget->ShowActionPrompt(TEXT("1=Attack | 2=Skill | R=Ultimate | Tab=cycle | 4/Esc=Flee"));
		}
		else
		{
			Widget->ShowActionPrompt(TEXT("1=Attack | 2=Skill (rhythm) | 4 or Esc=Flee"));
		}
	}
}

void AMelodiaRhythmGameModeBase::AutoConfirmVictoryIfPending()
{
	UWorld* World = GetWorld();
	if (!World || CurrentLoopPhase != EMelodiaLoopPhase::VictoryReward)
	{
		return;
	}

	AActor* BattleController = ActiveBattleController.Get();
	if (!BattleController)
	{
		BattleController = FindExistingActorOfClass(ResolveClass(BattleControllerClassPath));
	}
	if (BattleController && UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(BattleController))
	{
		if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
		{
			Session->ConfirmVictoryReward();
			return;
		}
		UMelodiaBattleLoopLibrary::ConfirmRhythmVictoryReward(BattleController);
	}
}

void AMelodiaRhythmGameModeBase::PrepareExplorationPresentation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UClass* BattleControllerClass = ResolveClass(BattleControllerClassPath))
	{
		for (TActorIterator<AActor> It(World, BattleControllerClass); It; ++It)
		{
			UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(*It, EMelodiaPhoenixTeardownScope::Full);
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->bDrawExplorationHUD = true;
		Widget->ClearBattleStatus();
		Widget->SetNoteHighwayActive(false, TArray<FMelodiaHighwayNote>(), 0.0f, 2.5f);
		Widget->SetBattlePhaseBanner(TEXT(""));
		Widget->ShowActionPrompt(TEXT("Explore: walk to the song gate | WASD move | I=Inventory"));
	}
}

void AMelodiaRhythmGameModeBase::EnsureBattleInputBridge()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* BattleControllerClass = ResolveClass(BattleControllerClassPath);
	if (!BattleControllerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle input: could not load battle controller class."));
		return;
	}

	AActor* BattleController = FindExistingActorOfClass(BattleControllerClass);
	if (!BattleController)
	{
		BattleController = SpawnLoopActor(BattleControllerClass, FVector(-10600.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
	}

	if (!BattleController)
	{
		return;
	}

	BattleController->SetActorHiddenInGame(false);
	BattleController->SetActorEnableCollision(true);

	ActiveBattleController = BattleController;
	UMelodiaBattleInputComponent* InputBridge = BattleController->FindComponentByClass<UMelodiaBattleInputComponent>();
	if (!InputBridge)
	{
		InputBridge = NewObject<UMelodiaBattleInputComponent>(BattleController, UMelodiaBattleInputComponent::StaticClass(), TEXT("MelodiaBattleInput"));
		if (InputBridge)
		{
			InputBridge->bAutoBindPlayerInput = false;
			InputBridge->RegisterComponent();
			BattleController->AddInstanceComponent(InputBridge);
		}
	}

	ActiveBattleInputComponent = InputBridge;
	if (ActiveBattleInputComponent)
	{
		bBattleInputBound = ActiveBattleInputComponent->BindBattleInput();
	}
}
