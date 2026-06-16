// Exploration trigger that starts the rhythm battle loop.

#include "MelodiaEncounterTrigger.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaPCGLibrary.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "CollisionQueryParams.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGMelodiaAttributes.h"
#include "PCG/MelodiaPCGWalkableIndex.h"

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
}

void AMelodiaEncounterTrigger::ArmEncounter()
{
	if (bEncounterArmed || !TriggerSphere)
	{
		return;
	}

	bEncounterArmed = true;
	ArmedAtWorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
	bHasArmedPlayerLocation = false;
	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (const APawn* ExplorationPawn = GameMode->ActiveExplorationPawn.Get())
		{
			ArmedPlayerLocation = ExplorationPawn->GetActorLocation();
			bHasArmedPlayerLocation = true;
		}
	}
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AMelodiaEncounterTrigger::OnTriggerBeginOverlap);
	UE_LOG(LogTemp, Log, TEXT("Melodia encounter trigger armed at %s."), *GetActorLocation().ToString());
}

void AMelodiaEncounterTrigger::DisarmEncounter()
{
	if (!TriggerSphere || !bEncounterArmed)
	{
		return;
	}

	TriggerSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AMelodiaEncounterTrigger::OnTriggerBeginOverlap);
	bEncounterArmed = false;
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
	InitializeTemplateBattleController(BattleController);
	UMelodiaBattleLoopLibrary::ResetRhythmBattleEncounter(BattleController);

	if (AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->SetLoopPhase(EMelodiaLoopPhase::Battle);
	}

	for (TActorIterator<AMelodiaQuestManagerBase> QuestIt(GetWorld()); QuestIt; ++QuestIt)
	{
		QuestIt->NotifyReachedSongGate();
		break;
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->ShowBattleStatus(TEXT("Battle started"));
		}
	}

	bLastActivationStartedBattle = true;
	DisarmEncounter();
	UE_LOG(LogTemp, Log, TEXT("Melodia encounter trigger started battle from %s."), InstigatorActor ? *InstigatorActor->GetName() : TEXT("direct activation"));
	return true;
}

void AMelodiaEncounterTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bEncounterArmed)
	{
		return;
	}

	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (GameMode->CurrentLoopPhase != EMelodiaLoopPhase::ExplorationReady)
		{
			return;
		}
	}

	if (!OtherActor || OtherActor == this || !OtherActor->IsA<APawn>())
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (ArmedAtWorldSeconds >= 0.0 && (World->GetTimeSeconds() - ArmedAtWorldSeconds) < PostArmCooldownSeconds)
		{
			return;
		}
	}

	if (bHasArmedPlayerLocation && MinPlayerTravelToActivate > 0.0f)
	{
		if (FVector::Dist2D(OtherActor->GetActorLocation(), ArmedPlayerLocation) < MinPlayerTravelToActivate)
		{
			return;
		}
	}

	StartEncounter(OtherActor);
}

UClass* AMelodiaEncounterTrigger::ResolveClass(const FSoftClassPath& ClassPath) const
{
	UObject* LoadedObject = ClassPath.TryLoad();
	return Cast<UClass>(LoadedObject);
}

AActor* AMelodiaEncounterTrigger::FindOrSpawnBattleController() const
{
	UWorld* World = GetWorld();
	FSoftClassPath ResolvedBattleClassPath = BattleControllerClassPath;
	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (GameMode->BattleControllerClassPath.IsValid())
		{
			ResolvedBattleClassPath = GameMode->BattleControllerClassPath;
		}
	}
	UClass* BattleClass = ResolveClass(ResolvedBattleClassPath);
	if (!World || !BattleClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, BattleClass); It; ++It)
	{
		return *It;
	}

	// Use PCG walkable attributes or raycast probe to place the battle controller
	// on a valid surface rather than at a hardcoded offset.
	const FVector DesiredLocation = GetActorLocation() + FVector(120.0f, 0.0f, 0.0f);

	// Try PCG walkable attributes first (preferred — uses authored PCG data).
	const FVector PCGHint = FindPCGWalkablePosition(DesiredLocation);
	const FVector RefineOrigin = PCGHint.IsZero() ? DesiredLocation : PCGHint;

	// Refine with raycast walkability probe on PCG-generated geometry.
	const FVector SpawnLocation = FindWalkableSpawnPosition(RefineOrigin);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(BattleClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
}

AActor* AMelodiaEncounterTrigger::FindOrSpawnBattleData() const
{
	UWorld* World = GetWorld();
	FSoftClassPath ResolvedBattleDataClassPath = BattleDataClassPath;
	if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (GameMode->BattleDataClassPath.IsValid())
		{
			ResolvedBattleDataClassPath = GameMode->BattleDataClassPath;
		}
	}

	UClass* BattleDataClass = ResolveClass(ResolvedBattleDataClassPath);
	if (!World || !BattleDataClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, BattleDataClass); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(BattleDataClass, GetActorLocation() + FVector(80.0f, 0.0f, 20.0f), FRotator::ZeroRotator, SpawnParameters);
}

void AMelodiaEncounterTrigger::InitializeTemplateBattleController(AActor* BattleController) const
{
	if (!BattleController)
	{
		return;
	}

	AActor* BattleData = FindOrSpawnBattleData();
	if (!BattleData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia encounter could not spawn Phoenix battle data."));
		return;
	}

	if (FObjectPropertyBase* CurrentBattleProperty = CastField<FObjectPropertyBase>(BattleController->GetClass()->FindPropertyByName(TEXT("currentBattle"))))
	{
		CurrentBattleProperty->SetObjectPropertyValue_InContainer(BattleController, BattleData);
	}

	// Phoenix expects a single StartBattle call; firing InitBattle + StartBattle together double-inits units.
	const FName InitCandidates[] = { TEXT("StartBattle"), TEXT("InitBattle"), TEXT("SwitchToBattleMode") };
	for (const FName FunctionName : InitCandidates)
	{
		UFunction* Function = BattleController->FindFunction(FunctionName);
		if (!Function)
		{
			continue;
		}

		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Function->ParmsSize, 1)));
		FMemory::Memzero(Params, Function->ParmsSize);
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		int32 ObjectInputIndex = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				It->InitializeValue_InContainer(Params);
				if (!It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
				{
					if (FObjectPropertyBase* ObjectParam = CastField<FObjectPropertyBase>(*It))
					{
						const FString ParamName = It->GetAuthoredName();
						UObject* Value = nullptr;
						if (ParamName.Contains(TEXT("Controller")) || ParamName.Contains(TEXT("Player")))
						{
							Value = PlayerController;
						}
						else if (ParamName.Contains(TEXT("Battle")))
						{
							Value = BattleData;
						}
						else
						{
							Value = ObjectInputIndex == 0 ? BattleData : Cast<UObject>(PlayerController);
						}

						ObjectParam->SetObjectPropertyValue_InContainer(Params, Value);
						++ObjectInputIndex;
					}
				}
			}
		}

		BattleController->ProcessEvent(Function, Params);

		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				It->DestroyValue_InContainer(Params);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Melodia encounter initialized battle via %s."), *FunctionName.ToString());
		break;
	}
}

FVector AMelodiaEncounterTrigger::FindWalkableSpawnPosition(const FVector& DesiredLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredLocation;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindWalkableSpawn), false, this);

	// Cast a downward ray from well above the desired location.
	constexpr float ProbeHeight    = 2000.0f;   // start 20 m above
	constexpr float ProbeDepth     = 4000.0f;   // ray length 40 m down
	constexpr float MaxWalkableAngle = 50.0f;   // ≤ 50° surface normal

	const FVector TraceStart = DesiredLocation + FVector(0.0f, 0.0f, ProbeHeight);
	const FVector TraceEnd   = DesiredLocation - FVector(0.0f, 0.0f, ProbeDepth);

	FHitResult BestHit;
	bool bFoundWalkable = false;
	float  BestDistSq   = FLT_MAX;

	// Primary downward trace
	if (World->LineTraceSingleByChannel(BestHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(BestHit.ImpactNormal, FVector::UpVector)));
		if (SurfaceAngle <= MaxWalkableAngle)
		{
			bFoundWalkable = true;
			BestDistSq = FVector::DistSquared(BestHit.ImpactPoint, DesiredLocation);
		}
	}

	// Fan out: try 4 offset positions (N/E/S/W) to catch nearby walkable floors
	// that the primary ray might miss (e.g. when the trigger is at a corridor edge).
	constexpr float ProbeRadius = 200.0f;   // 2 m search radius
	const FVector Offsets[4] = {
		FVector( ProbeRadius, 0.0f, 0.0f),
		FVector(-ProbeRadius, 0.0f, 0.0f),
		FVector(0.0f,  ProbeRadius, 0.0f),
		FVector(0.0f, -ProbeRadius, 0.0f),
	};

	for (const FVector& Offset : Offsets)
	{
		const FVector OffsetStart = TraceStart + Offset;
		const FVector OffsetEnd   = TraceEnd   + Offset;

		FHitResult OffsetHit;
		if (World->LineTraceSingleByChannel(OffsetHit, OffsetStart, OffsetEnd, ECC_WorldStatic, QueryParams))
		{
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OffsetHit.ImpactNormal, FVector::UpVector)));
			if (Angle <= MaxWalkableAngle)
			{
				const float DistSq = FVector::DistSquared(OffsetHit.ImpactPoint, DesiredLocation);
				if (!bFoundWalkable || DistSq < BestDistSq)
				{
					bFoundWalkable = true;
					BestDistSq    = DistSq;
					BestHit       = OffsetHit;
				}
			}
		}
	}

	if (bFoundWalkable)
	{
		// Spawn slightly above the surface to avoid floor clipping.
		return BestHit.ImpactPoint + FVector(0.0f, 0.0f, 5.0f);
	}

	return DesiredLocation;
}

FVector AMelodiaEncounterTrigger::FindPCGWalkablePosition(const FVector& DesiredLocation, float SearchRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	// Preferred path: use the WalkableIndex for actual walkable point data.
	AMelodiaPCGWalkableIndex* Index = UMelodiaPCGLibrary::FindWalkableIndex(World);

	if (Index && Index->GetCachedPointCount() > 0)
	{
		FVector WalkPos = FVector::ZeroVector;
		EPCGArchitecturalRole WalkRole = EPCGArchitecturalRole::None;
		FVector WalkNormal = FVector::UpVector;
		if (Index->FindNearestWalkable(DesiredLocation, SearchRadius, WalkPos, WalkRole, WalkNormal))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("EncounterTrigger: WalkableIndex found walkable point near %s (role=%d)."),
				*DesiredLocation.ToString(), static_cast<int32>(WalkRole));
			return WalkPos;
		}
	}

	// Fallback: use PCG component proximity as a hint for raycast refinement.
	float BestDistSq = FLT_MAX;
	FVector BestPosition = FVector::ZeroVector;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		const UPCGGraph* Graph = PCGComp->GetGraph();
		if (!Graph)
		{
			continue;
		}

		const FVector CompLoc = It->GetActorLocation();
		const float DistSq = FVector::DistSquared(CompLoc, DesiredLocation);

		if (DistSq < BestDistSq && DistSq < (SearchRadius * SearchRadius))
		{
			BestDistSq = DistSq;
			BestPosition = CompLoc;
		}
	}

	if (!BestPosition.IsZero())
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("EncounterTrigger: Fallback — PCG component proximity near %s (dist=%.0f cm)."),
			*DesiredLocation.ToString(), FMath::Sqrt(BestDistSq));
	}

	return BestPosition;
}
