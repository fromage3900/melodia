// Exploration trigger that starts the rhythm battle loop.

#include "MelodiaEncounterTrigger.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

AMelodiaEncounterTrigger::AMelodiaEncounterTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	RootComponent = TriggerSphere;
	TriggerSphere->InitSphereRadius(180.0f);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SongGateMarker"));
	VisualMesh->SetupAttachment(TriggerSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	VisualMesh->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
	VisualMesh->SetRenderCustomDepth(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
		bHasVisibleMarker = true;
	}
}

void AMelodiaEncounterTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerSphere)
	{
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AMelodiaEncounterTrigger::OnTriggerBeginOverlap);
	}
}

bool AMelodiaEncounterTrigger::StartEncounter(AActor* InstigatorActor)
{
	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (GameMode->CurrentLoopPhase != EMelodiaLoopPhase::ExplorationReady)
		{
			return false;
		}
	}

	AActor* BattleController = FindOrSpawnBattleController();
	if (!BattleController)
	{
		bLastActivationStartedBattle = false;
		return false;
	}

	++ActivationCount;
	LastBattleController = BattleController;
	UMelodiaBattleLoopLibrary::ResetRhythmBattleEncounter(BattleController);

	if (AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->SetLoopPhase(EMelodiaLoopPhase::Battle);
	}

	if (UWorld* World = GetWorld())
	{
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			UMelodiaRhythmHUDWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World)
			{
				Widget->ShowBattleStatus(TEXT("Battle started"));
			}
		}
	}

	bLastActivationStartedBattle = true;
	UE_LOG(LogTemp, Log, TEXT("Melodia encounter trigger started battle from %s."), InstigatorActor ? *InstigatorActor->GetName() : TEXT("direct activation"));
	return true;
}

void AMelodiaEncounterTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->IsA<APawn>())
	{
		StartEncounter(OtherActor);
	}
}

UClass* AMelodiaEncounterTrigger::ResolveClass(const FSoftClassPath& ClassPath) const
{
	UObject* LoadedObject = ClassPath.TryLoad();
	return Cast<UClass>(LoadedObject);
}

AActor* AMelodiaEncounterTrigger::FindOrSpawnBattleController() const
{
	UWorld* World = GetWorld();
	UClass* BattleClass = ResolveClass(BattleControllerClassPath);
	if (!World || !BattleClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, BattleClass); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(BattleClass, GetActorLocation() + FVector(120.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
}
