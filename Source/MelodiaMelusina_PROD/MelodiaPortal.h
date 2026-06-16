// Blueprintable exploration portal for Melodia map/area flow.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaPortal.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPortal : public AActor
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

protected:
	virtual void BeginPlay() override;

private:
	void PublishPortalFeedback(UWorld* World, const FString& Text) const;
};
