// Copyright Melodia Project. All Rights Reserved.
// Common interface for actors the player can interact with during exploration.
// Implemented by AMelodiaRestPoint and AMelodiaPortal.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MelodiaInteractable.generated.h"

class APawn;
class USphereComponent;

/**
 * Interface for interactable actors in the exploration phase.
 * 
 * Provides a unified API for:
 *   - Display name and interaction prompt (for HUD)
 *   - Activation (when player presses interact key)
 *   - Range checking (is pawn close enough?)
 *   - Interaction sphere (for visualization/debugging)
 *
 * Implementors: AMelodiaRestPoint, AMelodiaPortal
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UMelodiaInteractable : public UInterface
{
	GENERATED_BODY()
};

class MELODIAMELUSINA_PROD_API IMelodiaInteractable
{
	GENERATED_BODY()

public:
	/** Display name shown in the HUD (e.g. "Melusina's Bed", "Reverie Portal"). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melodia|Interactable")
	FString GetDisplayName() const;

	/** Interaction prompt shown to the player (e.g. "E: Rest until tomorrow"). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melodia|Interactable")
	FString GetInteractionPrompt() const;

	/** Activate this interactable with the given pawn. Returns true on success. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melodia|Interactable")
	bool ActivateInteraction(APawn* InstigatorPawn);

	/** Check if this interactable can be activated by the given pawn. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melodia|Interactable")
	bool CanInteract(APawn* InstigatorPawn) const;

	/** Get the interaction sphere component (for range checks / visualization). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melodia|Interactable")
	USphereComponent* GetInteractionSphere() const;
};
