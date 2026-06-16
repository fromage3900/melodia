// Blueprintable save/rest point: Melusina's Bed advances the day and saves.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaRestPoint.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaRestPoint : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaRestPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Rest")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Rest")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rest")
	FString DisplayName = TEXT("Melusina's Bed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rest")
	FString InteractionPrompt = TEXT("E: Rest until tomorrow");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rest")
	FString SaveSlotName = TEXT("Melodia_AutoSave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Rest")
	int32 UserIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rest")
	int32 LastSavedDay = 1;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rest")
	int32 RestActivationCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Rest")
	bool bLastSaveSucceeded = false;

	UFUNCTION(BlueprintCallable, Category="Melodia|Rest")
	bool ActivateRest(APawn* RestingPawn);

	UFUNCTION(BlueprintPure, Category="Melodia|Rest")
	bool IsPawnInRange(const APawn* Pawn) const;

protected:
	virtual void BeginPlay() override;

private:
	void PublishRestFeedback(UWorld* World, const FString& Text) const;
};
