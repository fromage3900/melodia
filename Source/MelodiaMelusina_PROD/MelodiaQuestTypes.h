// Quest and inventory data types for the Melodia exploration loop.

#pragma once

#include "CoreMinimal.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaQuestTypes.generated.h"

UENUM(BlueprintType)
enum class EMelodiaQuestStatus : uint8
{
	Locked UMETA(DisplayName="Locked"),
	Available UMETA(DisplayName="Available"),
	Active UMETA(DisplayName="Active"),
	Completed UMETA(DisplayName="Completed")
};

UENUM(BlueprintType)
enum class EMelodiaQuestObjectiveType : uint8
{
	ReachLocation UMETA(DisplayName="Reach Location"),
	WinBattle UMETA(DisplayName="Win Battle"),
	CollectItem UMETA(DisplayName="Collect Item"),
	PerfectNotesInSkill UMETA(DisplayName="Perfect Notes In Skill"),
	BreakEnemy UMETA(DisplayName="Break Enemy"),
	WinWithUltimate UMETA(DisplayName="Win With Ultimate"),
	TalkToNpc UMETA(DisplayName="Talk To NPC")
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaQuestDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	EMelodiaQuestObjectiveType ObjectiveType = EMelodiaQuestObjectiveType::WinBattle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FName TargetItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	int32 TargetCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FVector WorldMarkerLocation = FVector::ZeroVector;

	/**
	 * Optional PCG architectural role for auto-generated markers.
	 * When set, WorldMarkerLocation is ignored and the quest system
	 * auto-discovers positions from PCG data matching this role.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	EPCGArchitecturalRole AssociatedPCGRole = EPCGArchitecturalRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Quest")
	FString RewardText = TEXT("+50G");
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	FName QuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	EMelodiaQuestStatus Status = EMelodiaQuestStatus::Locked;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 CurrentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Quest")
	int32 TargetCount = 1;
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaMinimapMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Minimap")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Minimap")
	FLinearColor Tint = FLinearColor(0.98f, 0.78f, 0.32f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Minimap")
	FString Label;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Minimap")
	bool bPulse = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Minimap")
	bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Inventory")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Inventory")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Inventory")
	FLinearColor IconTint = FLinearColor(0.72f, 0.88f, 1.0f, 1.0f);
};
