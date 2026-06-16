// Blueprintable exploration portal for Melodia map/area flow.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaInteractable.h"
#include "MelodiaPortal.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPortal : public AActor, public IMelodiaInteractable
{
	GENERATED_BODY()

public:
	AMelodiaPortal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Portal")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Portal")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Portal")
	FString DisplayName = TEXT("Reverie Portal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Portal")
	FString InteractionPrompt = TEXT("E: Enter portal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Portal")
	FName TargetLevelName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Portal")
	FVector TargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Portal")
	bool bUseTargetWorldLocation = true;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Portal")
	int32 ActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Portal")
	bool bLastActivationSucceeded = false;

	UFUNCTION(BlueprintCallable, Category="Melodia|Portal")
	bool ActivatePortal(APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category="Melodia|Portal")
	bool IsPawnInRange(const APawn* Pawn) const;

	// ── IMelodiaInteractable interface ──────────────────────────────
	virtual FString GetDisplayName_Implementation() const override;
	virtual FString GetInteractionPrompt_Implementation() const override;
	virtual bool ActivateInteraction_Implementation(APawn* InstigatorPawn) override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual USphereComponent* GetInteractionSphere_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	void PublishPortalFeedback(UWorld* World, const FString& Text) const;
};
