// Runtime cosmetic preset application for Melodia characters.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaCosmeticsComponent.generated.h"

class UMaterialInterface;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FMelodiaCosmeticPreset
{
	GENERATED_BODY()

	FMelodiaCosmeticPreset()
		: DisplayName(NSLOCTEXT("MelodiaCosmetics", "MelusinaDefaultDisplayName", "Melusina Atelier"))
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	FName PresetId = TEXT("MelusinaDefault");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	TSoftObjectPtr<UMaterialInterface> AccentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	FLinearColor PrimaryTint = FLinearColor(0.84f, 0.72f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	FLinearColor AccentTint = FLinearColor(1.0f, 0.82f, 0.38f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Cosmetics")
	bool bEnableSparkles = true;
};

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaCosmeticsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaCosmeticsComponent();

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Cosmetics")
	FMelodiaCosmeticPreset CurrentPreset;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Cosmetics")
	FName LastAppliedPresetId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Cosmetics")
	int32 AppliedCosmeticCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Cosmetics")
	bool bLastApplySucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Cosmetics")
	bool bSparkleCosmeticEnabled = false;

	UFUNCTION(BlueprintCallable, Category="Melodia|Cosmetics")
	bool ApplyCosmeticPreset(const FMelodiaCosmeticPreset& Preset);

	UFUNCTION(BlueprintCallable, Category="Melodia|Cosmetics")
	bool ApplyDefaultMelusinaPreset();

private:
	USkeletalMeshComponent* FindTargetMesh() const;
};
