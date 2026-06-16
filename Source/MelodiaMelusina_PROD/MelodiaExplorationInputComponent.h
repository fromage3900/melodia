// Exploration UI input: inventory toggle and quest refresh.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaExplorationInputComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaExplorationInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaExplorationInputComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Melodia|Exploration Input")
	bool BindExplorationInput();

	UFUNCTION(BlueprintCallable, Category="Melodia|Exploration Input")
	void ToggleInventoryPanel();

	UFUNCTION(BlueprintCallable, Category="Melodia|Exploration Input")
	bool ActivateNearestInteraction();

private:
	bool bInputBound = false;
	void OnInventoryPressed();
	void OnInteractPressed();
};
