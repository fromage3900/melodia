// Blueprintable quest NPC base — activates quests via IMelodiaInteractable.

#include "MelodiaNPCBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"

AMelodiaNPCBase::AMelodiaNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->InitSphereRadius(160.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(InteractionSphere);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
	}
}

void AMelodiaNPCBase::BeginPlay()
{
	Super::BeginPlay();
}

FString AMelodiaNPCBase::GetDisplayName_Implementation() const
{
	return DisplayName;
}

FString AMelodiaNPCBase::GetInteractionPrompt_Implementation() const
{
	if (!IsMechanicLevelSufficient())
	{
		return FString::Printf(TEXT("Requires Mechanic Lv %d"), RequiredMechanicLevel);
	}
	return InteractionPrompt;
}

bool AMelodiaNPCBase::ActivateInteraction_Implementation(APawn* InstigatorPawn)
{
	return ActivateQuestForPawn(InstigatorPawn);
}

bool AMelodiaNPCBase::ActivateQuestForPawn(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || !IsMechanicLevelSufficient())
	{
		return false;
	}

	++InteractionCount;
	PublishDialogue(DialogueLine);

	if (!QuestId.IsNone())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
			{
				It->ActivateQuest(QuestId);
				break;
			}
		}
	}

	return true;
}

bool AMelodiaNPCBase::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return InstigatorPawn != nullptr && IsMechanicLevelSufficient();
}

USphereComponent* AMelodiaNPCBase::GetInteractionSphere_Implementation() const
{
	return InteractionSphere;
}

void AMelodiaNPCBase::PublishDialogue(const FString& Text) const
{
	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetJudgmentString(FString::Printf(TEXT("%s: %s"), *DisplayName, *Text));
			Widget->TriggerSparkleBurst();
		}
	}
}

bool AMelodiaNPCBase::IsMechanicLevelSufficient() const
{
	if (RequiredMechanicLevel <= 0)
	{
		return true;
	}

	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			return Progression->GetMechanicLevel() >= RequiredMechanicLevel;
		}
	}
	return RequiredMechanicLevel <= 1;
}
