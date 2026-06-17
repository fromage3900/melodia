#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MelodiaPortfolioSubsystem.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPortfolioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Portfolio")
	void LaunchGameplayDemo();

private:
	void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
	void TryShowPortfolioPrompt(UWorld* World);

	FDelegateHandle PostWorldInitHandle;
};
