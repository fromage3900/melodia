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

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	int32 ActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	bool bLastActivationStartedBattle = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Encounter")
	TObjectPtr<AActor> LastBattleController;

	UFUNCTION(BlueprintCallable, Category="Melodia|Encounter")
	bool StartEncounter(AActor* InstigatorActor = nullptr);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UClass* ResolveClass(const FSoftClassPath& ClassPath) const;
	AActor* FindOrSpawnBattleController() const;
};
