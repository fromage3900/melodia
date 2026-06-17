// Pickable environment flower — F to harvest into inventory (portfolio interaction + env art hook).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaPickableFlower.generated.h"

class APawn;
class UStaticMeshComponent;
class USphereComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPickableFlower : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPickableFlower();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Flower")
	TObjectPtr<USphereComponent> PickRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Flower")
	TObjectPtr<UStaticMeshComponent> StemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Flower")
	TObjectPtr<UStaticMeshComponent> BloomMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Flower")
	FName FlowerItemId = TEXT("ReverieBlossom");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Flower")
	FString DisplayName = TEXT("Reverie Blossom");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Flower")
	FLinearColor BloomTint = FLinearColor(0.98f, 0.52f, 0.86f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Flower")
	int32 PickQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Flower")
	FString PickPrompt = TEXT("F: Pick flower");

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Flower")
	bool bPicked = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Flower")
	int32 PickCount = 0;

	UFUNCTION(BlueprintCallable, Category="Melodia|Flower")
	bool TryPick(APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category="Melodia|Flower")
	bool CanPick(const APawn* Pawn) const;

	UFUNCTION(BlueprintPure, Category="Melodia|Flower")
	FString GetPickPromptText() const;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyBloomTint();
	void HideFlowerVisuals();
};
