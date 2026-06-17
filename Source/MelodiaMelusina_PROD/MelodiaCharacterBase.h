// Native exploration hero base: locomotion pawn with Melodia gameplay components.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MelodiaCharacterBase.generated.h"

class UCameraComponent;
class UMelodiaCosmeticsComponent;
class UMelodiaExplorationInputComponent;
class UMelodiaGlideComponent;
class UMelodiaInventoryComponent;
class USkeletalMesh;
class USpringArmComponent;
class UInputComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character")
	TObjectPtr<UMelodiaGlideComponent> GlideComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Character|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

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

	/** Optional fallback mesh when none is assigned on the BP (defaults to SK_Melusina_Prototype). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character|Placeholder")
	TSoftObjectPtr<USkeletalMesh> PlaceholderSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Melodia/Characters/Melusina/SK_Melusina_Prototype.SK_Melusina_Prototype")));

	/** Load PlaceholderSkeletalMesh if the character mesh slot is empty at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Character|Placeholder")
	bool bUsePlaceholderMeshWhenEmpty = true;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Character")
	void InitializeExplorationSystems();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Character")
	void EnsureDisplayMesh();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Character")
	void ApplyMelusinaPresentation();

	UFUNCTION(BlueprintPure, Category = "Melodia|Character")
	UMelodiaCosmeticsComponent* GetCosmeticsComponent() const { return CosmeticsComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;
	virtual void StopJumping() override;

	void ConfigureExplorationMovement();
	void ConfigureExplorationCamera();

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
};
