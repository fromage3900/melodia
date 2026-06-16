// Native exploration hero base: locomotion pawn with Melodia gameplay components.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MelodiaCharacterBase.generated.h"

class UMelodiaCosmeticsComponent;
class UMelodiaExplorationInputComponent;
class UMelodiaInventoryComponent;

/**
 * Authoritative Melusina exploration character.
 * BP_Melusina should reparent to this class (or a Blueprint child) so cosmetics,
 * inventory, and exploration input are always present without game-mode patching.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMelodiaCharacterBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character")
	TObjectPtr<UMelodiaCosmeticsComponent> CosmeticsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character")
	TObjectPtr<UMelodiaExplorationInputComponent> ExplorationInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character")
	TObjectPtr<UMelodiaInventoryComponent> InventoryComponent;

	/** Apply palette/sparkle cosmetics on spawn and after battle return. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character")
	bool bAutoApplyCosmetics = true;

	/** Seed starter inventory items on first spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character")
	bool bSeedStarterInventory = true;

	/** Bind E/I exploration actions when possessed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character")
	bool bAutoBindExplorationInput = true;

	/** Cosmetic preset applied when bAutoApplyCosmetics is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character")
	FName DefaultCosmeticPresetId = TEXT("MelusinaDefault");

	UFUNCTION(BlueprintCallable, Category = "Melodia|Character")
	void InitializeExplorationSystems();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Character")
	void ApplyMelusinaPresentation();

	UFUNCTION(BlueprintPure, Category = "Melodia|Character")
	UMelodiaCosmeticsComponent* GetCosmeticsComponent() const { return CosmeticsComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	void ConfigureExplorationMovement();
};
