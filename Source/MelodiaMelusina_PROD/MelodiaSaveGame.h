// Save data for the Melodia exploration loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MelodiaQuestTypes.h"
#include "MelodiaSaveGame.generated.h"

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UMelodiaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// ── Day/Rest tracking ────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	int32 DayIndex = 1;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	int32 RestCount = 0;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	FName LastMapName = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	FVector LastRestLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	FRotator LastRestRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	bool bHasRestedAtMelusinasBed = false;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save")
	FString LastSaveReason;

	// ── Quest state ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Quest")
	TArray<FMelodiaQuestProgress> QuestProgressArray;

	// ── Inventory ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Inventory")
	TArray<FMelodiaInventorySlot> InventorySlots;

	// ── Run state ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Run")
	int32 CurrentDayIndex = 1;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Run")
	FName CurrentAreaName = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Run")
	float RunElapsedTime = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Run")
	int32 AreasCompleted = 0;

	// ── Cosmetics ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Cosmetic")
	TMap<FName, FName> ActiveCosmetics;  // slot → cosmetic ID
};
