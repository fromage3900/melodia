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
#include "MelodiaCharacterBase.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaExplorationInputComponent.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaLoopVerifier.h"
#include "MelodiaMusicManager.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaReverieRunManager.h"
#include "MelodiaRestPoint.h"
#include "MelodiaPortal.h"
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
	EnsureBattleInputBridge();
	if (CurrentLoopPhase == EMelodiaLoopPhase::ExplorationReady)
	{
		RestoreExplorationControl();
	}
}

void AMelodiaRhythmGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	SetLoopPhase(EMelodiaLoopPhase::Bootstrapping);

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
			ActiveMusicManager->StartBattleMusic(nullptr, DefaultBattleBPM);
			UE_LOG(LogTemp, Log, TEXT("Melodia loop bootstrap started Quartz rhythm clock at %.2f BPM."), DefaultBattleBPM);
		}
	}

	if (UClass* RhythmHUDClass = ResolveClass(RhythmHUDActorClassPath))
	{
		if (!FindExistingActorOfClass(RhythmHUDClass))
		{
			SpawnLoopActor(RhythmHUDClass, FVector(-10620.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
		}
	}

	EnsureRhythmHUDWidget();

	if (UClass* RhythmTestManagerClass = ResolveClass(RhythmTestManagerClassPath))
	{
		if (!FindExistingActorOfClass(RhythmTestManagerClass))
		{
			SpawnLoopActor(RhythmTestManagerClass, FVector(-10700.0f, -5040.0f, -2150.0f), FRotator::ZeroRotator);
		}
	}

	EnsureBattleInputBridge();

	if (UClass* QuestManagerClass = ResolveClass(QuestManagerClassPath))
	{
		if (!FindExistingActorOfClass(QuestManagerClass))
		{
			SpawnLoopActor(QuestManagerClass, FVector(-10570.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
		}
	}
	else
	{
		if (!FindExistingActorOfClass(AMelodiaQuestManagerBase::StaticClass()))
		{
			GetWorld()->SpawnActor<AMelodiaQuestManagerBase>(AMelodiaQuestManagerBase::StaticClass(), FVector(-10570.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
			UE_LOG(LogTemp, Warning, TEXT("Melodia loop bootstrap fell back to native AMelodiaQuestManagerBase."));
		}
	}

	EnsureEncounterTrigger();
	EnsureReverieRunManager();
	EnsurePCGGameplayPlacement();
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

void AMelodiaRhythmGameModeBase::SetLoopPhase(const EMelodiaLoopPhase NewPhase)
{
	CurrentLoopPhase = NewPhase;
	LastLoopPhaseText = GetLoopPhaseText();

	switch (NewPhase)
	{
	case EMelodiaLoopPhase::Battle:
		++BattlePhaseEntryCount;
		EnsureBattleInputBridge();
		EnsureRhythmHUDWidget();
		break;
	case EMelodiaLoopPhase::VictoryReward:
		++VictoryRewardPhaseCount;
		if (UWorld* World = GetWorld())
		{
			if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
			{
				Widget->ShowActionPrompt(TEXT("Victory! Space/1 to claim reward"));
			}
		}
		break;
	case EMelodiaLoopPhase::ExplorationReady:
		++ExplorationReadyPhaseCount;
		RestoreExplorationControl();
		if (ActiveEncounterTrigger)
		{
			ActiveEncounterTrigger->ArmEncounter();
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

	if (ActiveEncounterTrigger && !bPCGPlacementActive)
	{
		ActiveEncounterTrigger->SetActorLocation(EncounterTriggerLocation);
	}
}

void AMelodiaRhythmGameModeBase::FinishLoopBootstrap()
{
	EnsurePCGGameplayPlacement();
	SyncExplorationLocations();
	EnsureRhythmHUDWidget();
	EnsureBattleInputBridge();
	EnsureWorldInteractions();
	SetLoopPhase(EMelodiaLoopPhase::ExplorationReady);

	if (AMelodiaQuestManagerBase* QuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(ResolveClass(QuestManagerClassPath))))
	{
		QuestManager->RegisterStarterQuests(EncounterTriggerLocation);
	}
	else if (AMelodiaQuestManagerBase* NativeQuestManager = Cast<AMelodiaQuestManagerBase>(FindExistingActorOfClass(AMelodiaQuestManagerBase::StaticClass())))
	{
		NativeQuestManager->RegisterStarterQuests(EncounterTriggerLocation);
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

void AMelodiaRhythmGameModeBase::EnsureRhythmHUDWidget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ActiveRhythmHUDWidget)
	{
		ActiveRhythmHUDWidget->ApplyCuteCombatTheme();
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		bRhythmHUDWidgetInViewport = true;
		return;
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		ActiveRhythmHUDWidget = Widget;
		ActiveRhythmHUDWidget->ApplyCuteCombatTheme();
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		bRhythmHUDWidgetInViewport = true;
		return;
	}

	UClass* WidgetClass = ResolveClass(RhythmHUDWidgetClassPath);
	if (!WidgetClass)
	{
		WidgetClass = UMelodiaRhythmHUDWidget::StaticClass();
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop fell back to native UMelodiaRhythmHUDWidget."));
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop could not load rhythm HUD widget class: %s"), *RhythmHUDWidgetClassPath.ToString());
		return;
	}

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia loop could not add rhythm HUD widget yet: no player controller."));
		return;
	}

	ActiveRhythmHUDWidget = CreateWidget<UMelodiaRhythmHUDWidget>(PlayerController, WidgetClass);
	if (ActiveRhythmHUDWidget)
	{
		ActiveRhythmHUDWidget->AddToViewport(40);
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ActiveRhythmHUDWidget->bDrawExplorationHUD = true;
		ActiveRhythmHUDWidget->ApplyCuteCombatTheme();
		ActiveRhythmHUDWidget->TriggerSparkleBurst();
		bRhythmHUDWidgetInViewport = true;
		UE_LOG(LogTemp, Log, TEXT("Melodia loop added native rhythm HUD widget to viewport."));
	}
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
		if (AMelodiaEncounterTrigger* ExistingTrigger = Cast<AMelodiaEncounterTrigger>(FindExistingActorOfClass(AMelodiaEncounterTrigger::StaticClass())))
		{
			ActiveEncounterTrigger = ExistingTrigger;
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
		if (!bPCGPlacementActive)
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

	if (ActivePortal)
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

	ActiveBattleController = BattleController;
	UMelodiaBattleInputComponent* InputBridge = BattleController->FindComponentByClass<UMelodiaBattleInputComponent>();
	if (!InputBridge)
	{
		InputBridge = NewObject<UMelodiaBattleInputComponent>(BattleController, UMelodiaBattleInputComponent::StaticClass(), TEXT("MelodiaBattleInput"));
		if (InputBridge)
		{
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
