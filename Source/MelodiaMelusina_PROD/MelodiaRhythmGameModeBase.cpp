// Runtime bootstrapper for the Melodia rhythm vertical slice.

#include "MelodiaRhythmGameModeBase.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleInputComponent.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaLoopVerifier.h"
#include "MelodiaMusicManager.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/UObjectIterator.h"

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

	EnsureEncounterTrigger();

	if (bRunLoopVerifier && !FindExistingActorOfClass(AMelodiaLoopVerifier::StaticClass()))
	{
		GetWorld()->SpawnActor<AMelodiaLoopVerifier>(AMelodiaLoopVerifier::StaticClass(), FVector(-10540.0f, -5000.0f, -2140.0f), FRotator::ZeroRotator);
	}

	SetLoopPhase(EMelodiaLoopPhase::Battle);
}

void AMelodiaRhythmGameModeBase::SetLoopPhase(const EMelodiaLoopPhase NewPhase)
{
	CurrentLoopPhase = NewPhase;
	LastLoopPhaseText = GetLoopPhaseText();

	switch (NewPhase)
	{
	case EMelodiaLoopPhase::Battle:
		++BattlePhaseEntryCount;
		break;
	case EMelodiaLoopPhase::VictoryReward:
		++VictoryRewardPhaseCount;
		break;
	case EMelodiaLoopPhase::ExplorationReady:
		++ExplorationReadyPhaseCount;
		RestoreExplorationControl();
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

void AMelodiaRhythmGameModeBase::RestoreExplorationControl()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* ExplorationPawn = ActiveExplorationPawn.Get();
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
	ApplyMelusinaPresentation(ExplorationPawn);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		PlayerController->Possess(ExplorationPawn);
		PlayerController->SetViewTarget(ExplorationPawn);
		bExplorationControlReady = PlayerController->GetPawn() == ExplorationPawn;
		if (bExplorationControlReady)
		{
			++ExplorationControlRestoreCount;
			for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
			{
				UMelodiaRhythmHUDWidget* Widget = *It;
				if (Widget && Widget->GetWorld() == World)
				{
					Widget->ShowBattleStatus(TEXT("Explore: touch the song gate"));
				}
			}

			UE_LOG(LogTemp, Log, TEXT("Melodia loop restored exploration control to %s."), *ExplorationPawn->GetName());
		}
	}
}

void AMelodiaRhythmGameModeBase::ApplyMelusinaPresentation(APawn* ExplorationPawn)
{
	if (!ExplorationPawn)
	{
		return;
	}

	bMelusinaPawnActive = ExplorationPawn->GetClass()->GetName().Contains(TEXT("Melusina"));
	if (bMelusinaPawnActive)
	{
		++MelusinaPawnApplyCount;
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
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			UMelodiaRhythmHUDWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World)
			{
				Widget->ApplyCuteCombatTheme();
				Widget->TriggerSparkleBurst();
			}
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

	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		UMelodiaRhythmHUDWidget* Widget = *It;
		if (Widget && Widget->GetWorld() == World)
		{
			ActiveRhythmHUDWidget = Widget;
			ActiveRhythmHUDWidget->ApplyCuteCombatTheme();
			ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			bRhythmHUDWidgetInViewport = true;
			return;
		}
	}

	UClass* WidgetClass = ResolveClass(RhythmHUDWidgetClassPath);
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!WidgetClass || !PlayerController)
	{
		return;
	}

	ActiveRhythmHUDWidget = CreateWidget<UMelodiaRhythmHUDWidget>(PlayerController, WidgetClass);
	if (ActiveRhythmHUDWidget)
	{
		ActiveRhythmHUDWidget->AddToViewport(40);
		ActiveRhythmHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ActiveRhythmHUDWidget->ApplyCuteCombatTheme();
		ActiveRhythmHUDWidget->TriggerSparkleBurst();
		bRhythmHUDWidgetInViewport = true;
		UE_LOG(LogTemp, Log, TEXT("Melodia loop added native rhythm HUD widget to viewport."));
	}
}

void AMelodiaRhythmGameModeBase::EnsureEncounterTrigger()
{
	if (ActiveEncounterTrigger)
	{
		return;
	}

	if (AMelodiaEncounterTrigger* ExistingTrigger = Cast<AMelodiaEncounterTrigger>(FindExistingActorOfClass(AMelodiaEncounterTrigger::StaticClass())))
	{
		ActiveEncounterTrigger = ExistingTrigger;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveEncounterTrigger = World->SpawnActor<AMelodiaEncounterTrigger>(AMelodiaEncounterTrigger::StaticClass(), EncounterTriggerLocation, FRotator::ZeroRotator, SpawnParameters);
}

void AMelodiaRhythmGameModeBase::EnsureBattleInputBridge()
{
	if (ActiveBattleInputComponent && bBattleInputBound)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* BattleControllerClass = ResolveClass(BattleControllerClassPath);
	if (!BattleControllerClass)
	{
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
	bBattleInputBound = ActiveBattleInputComponent && ActiveBattleInputComponent->BindBattleInput();
	if (bBattleInputBound)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia rhythm battle input bridge bound Attack, Skill, and Ultimate."));
	}
}
