// Exploration trigger that starts the rhythm battle loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaEncounterTrigger.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaEncounterTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Encounter")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Encounter")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	bool bHasVisibleMarker = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Encounter")
	FSoftClassPath BattleControllerClassPath = FSoftClassPath(TEXT("/Game/TurnBasedJRPGTemplate/Blueprints/Battle/BP_BattleController.BP_BattleController_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Encounter")
	FSoftClassPath BattleDataClassPath = FSoftClassPath(TEXT("/Game/Blueprints/Gameplay/BP_PhoenixRhythmPlaytestBattle.BP_PhoenixRhythmPlaytestBattle_C"));

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	int32 ActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	bool bLastActivationStartedBattle = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	TObjectPtr<AActor> LastBattleController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Encounter", meta=(ClampMin="0"))
	float PostArmCooldownSeconds = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Encounter", meta=(ClampMin="0"))
	float MinPlayerTravelToActivate = 32.0f;

	UFUNCTION(BlueprintCallable, Category="Melodia|Encounter")
	bool StartEncounter(AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Melodia|Encounter")
	void ArmEncounter();

	UFUNCTION(BlueprintCallable, Category="Melodia|Encounter")
	void DisarmEncounter();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bEncounterArmed = false;
	double ArmedAtWorldSeconds = -1.0;
	FVector ArmedPlayerLocation = FVector::ZeroVector;
	bool bHasArmedPlayerLocation = false;

	UClass* ResolveClass(const FSoftClassPath& ClassPath) const;
	AActor* FindOrSpawnBattleController() const;
	AActor* FindOrSpawnBattleData() const;
	void InitializeTemplateBattleController(AActor* BattleController) const;

	/** Ray-cast probe to find a walkable surface near a desired location.
	 *  Casts multiple downward rays and picks the nearest walkable hit.
	 *  Falls back to DesiredLocation if no walkable surface is found. */
	FVector FindWalkableSpawnPosition(const FVector& DesiredLocation) const;

	/** Query PCG metadata for the nearest point with Walkable=true.
	 *  Returns the point location, or zero vector if no PCG walkable data found. */
	FVector FindPCGWalkablePosition(const FVector& DesiredLocation, float SearchRadius = 2000.0f) const;
};
