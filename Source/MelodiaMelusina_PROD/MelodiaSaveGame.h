// Save data for the Melodia exploration loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MelodiaQuestTypes.h"
#include "MelodiaSpellTypes.h"
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

	// ── Mechanic progression (demo Lv 1–30) ───────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	int32 MechanicLevel = 1;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	int32 MechanicXP = 0;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	int32 MechanicTotalLevelUps = 0;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	TArray<FName> UnlockedLocationPresetIds;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	TArray<FName> UnlockedSkillIds;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	TArray<FName> UnlockedKeyIds;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	FName ActiveSkillId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	EMelodiaSpellElement EquippedKeyElement = EMelodiaSpellElement::Forte;

	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Mechanic")
	bool bCompanionUnlocked = false;

	// ── Cosmetics ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Melodia|Save|Cosmetic")
	TMap<FName, FName> ActiveCosmetics;  // slot → cosmetic ID
};
