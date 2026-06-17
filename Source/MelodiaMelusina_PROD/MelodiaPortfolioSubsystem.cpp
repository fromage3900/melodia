#include "MelodiaPortfolioSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MelodiaMenuBridgeLibrary.h"
#include "TimerManager.h"

void UMelodiaPortfolioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this, &UMelodiaPortfolioSubsystem::HandlePostWorldInit);
}

void UMelodiaPortfolioSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
	Super::Deinitialize();
}

void UMelodiaPortfolioSubsystem::HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UMelodiaPortfolioSubsystem::TryShowPortfolioPrompt, World));
}

void UMelodiaPortfolioSubsystem::TryShowPortfolioPrompt(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const FString MapName = World->GetMapName();
	if (!MapName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase))
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			30.0f,
			FColor::Cyan,
			TEXT("Melodia Portfolio — PLAY = gameplay demo | console: Melodia.PlayPCGDemo for PCG Terrace Garden"));
	}
}

void UMelodiaPortfolioSubsystem::LaunchGameplayDemo()
{
	UMelodiaMenuBridgeLibrary::LaunchGameplayLoopTest(this);
}
