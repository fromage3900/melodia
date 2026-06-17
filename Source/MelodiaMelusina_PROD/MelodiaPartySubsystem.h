// Party roster for Phoenix playerUnits sync (Melusina + companions).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MelodiaPartySubsystem.generated.h"

USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaPartySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Party")
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Party")
	FName MemberId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Party")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Party")
	bool bActiveInBattle = true;

	/** Bonus SP at battle start (cockatoo = 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Party")
	int32 BonusSkillPointsAtBattleStart = 0;
};

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPartySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static constexpr int32 MaxPartySlots = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Party")
	TArray<FMelodiaPartySlot> PartySlots;

	UFUNCTION(BlueprintPure, Category = "Melodia|Party", meta = (WorldContext = "WorldContextObject"))
	static UMelodiaPartySubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Party")
	void ResetToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Party")
	void SyncFromProgression();

	UFUNCTION(BlueprintPure, Category = "Melodia|Party")
	int32 GetTotalBonusSkillPointsAtBattleStart() const;

	UFUNCTION(BlueprintPure, Category = "Melodia|Party")
	TArray<FMelodiaPartySlot> GetActiveBattleMembers() const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Party")
	void SetCompanionInParty(const bool bInParty, const FText& DisplayName = INVTEXT("Cockatoo"));

	UFUNCTION(BlueprintCallable, Category = "Melodia|Party")
	void ApplyBattleStartBuffs(class UMelodiaCombatStateComponent* CombatState) const;
};
