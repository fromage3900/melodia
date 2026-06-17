// Builds a flat gameplay-loop test arena with gate, NPC, bed, portal, flowers, and enemy.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaGameplayLoopTestDirector.generated.h"

class AMelodiaEncounterTrigger;
class AMelodiaNPCBase;
class AMelodiaPickableFlower;
class AMelodiaPortal;
class AMelodiaRestPoint;
class AMelodiaRhythmGameModeBase;
class AMelodiaWorldEnemy;
class APlayerStart;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaGameplayLoopTestDirector : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaGameplayLoopTestDirector();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	bool bAutoBuildLayout = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	bool bApplyToGameModeOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector ArenaOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector PlayerSpawnOffset = FVector(-1500.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector EncounterGateOffset = FVector(1200.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector QuestGiverOffset = FVector(-800.0f, -900.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector RestBedOffset = FVector(-800.0f, 900.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FVector PortalOffset = FVector(800.0f, 900.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	TArray<FVector> FlowerOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath EncounterGateClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_EncounterGate.BP_TestLoop_EncounterGate_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath WorldEnemyClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_WorldEnemy.BP_TestLoop_WorldEnemy_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath QuestGiverClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_QuestGiver.BP_TestLoop_QuestGiver_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath RestBedClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_RestBed.BP_TestLoop_RestBed_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath PortalClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_Portal.BP_TestLoop_Portal_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|TestLoop")
	FSoftClassPath FlowerClassPath = FSoftClassPath(TEXT("/Game/Melodia/TestLoop/BP_TestLoop_Flower.BP_TestLoop_Flower_C"));

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	bool bLayoutBuilt = false;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<APlayerStart> PlayerStartActor;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<AMelodiaEncounterTrigger> EncounterGate;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<AMelodiaWorldEnemy> WorldEnemy;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<AMelodiaNPCBase> QuestGiver;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<AMelodiaRestPoint> RestBed;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TObjectPtr<AMelodiaPortal> Portal;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|TestLoop")
	TArray<TObjectPtr<AMelodiaPickableFlower>> Flowers;

	UFUNCTION(BlueprintCallable, Category = "Melodia|TestLoop")
	bool BuildLayout();

	UFUNCTION(BlueprintCallable, Category = "Melodia|TestLoop")
	void ApplyToGameMode(AMelodiaRhythmGameModeBase* GameMode) const;

	UFUNCTION(BlueprintPure, Category = "Melodia|TestLoop")
	FVector GetPlayerSpawnLocation() const { return ArenaOrigin + PlayerSpawnOffset; }

	UFUNCTION(BlueprintPure, Category = "Melodia|TestLoop")
	FVector GetEncounterGateLocation() const { return ArenaOrigin + EncounterGateOffset; }

protected:
	virtual void BeginPlay() override;

private:
	void EnsureDefaultFlowerOffsets();
	void EnsureArenaFloor();
	void EnsurePlayerStart();
	AActor* SpawnLoopActor(UClass* ActorClass, const FVector& Location, const FRotator& Rotation, const FName ActorTag) const;
	UClass* ResolveOptionalClass(const FSoftClassPath& ClassPath, UClass* NativeFallback) const;
};
