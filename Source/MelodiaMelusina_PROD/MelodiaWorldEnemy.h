// Visible exploration enemy that starts a Melodia encounter on overlap.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaWorldEnemy.generated.h"

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaWorldEnemy : public AMelodiaEncounterTrigger
{
	GENERATED_BODY()

public:
	AMelodiaWorldEnemy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Enemy")
	FString EnemyDisplayName = TEXT("Garden Slime");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Enemy")
	FLinearColor SlimeTint = FLinearColor(0.35f, 0.92f, 0.48f, 1.0f);

protected:
	virtual void BeginPlay() override;

private:
	void ApplySlimePresentation();
};
