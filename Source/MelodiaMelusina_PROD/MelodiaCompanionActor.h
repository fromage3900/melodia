// Cockatoo companion: follows player in explore, grants battle party buff at Lv8+.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaInteractable.h"
#include "MelodiaSpellTypes.h"
#include "MelodiaCompanionActor.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EMelodiaCompanionBuffType : uint8
{
	SkillPointRegen UMETA(DisplayName = "Skill Point Regen"),
	DamageBoost     UMETA(DisplayName = "Damage Boost")
};

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaCompanionActor : public AActor, public IMelodiaInteractable
{
	GENERATED_BODY()

public:
	AMelodiaCompanionActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Companion")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|Companion")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion")
	FString DisplayName = TEXT("Cockatoo");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion")
	FString InteractionPrompt = TEXT("F: Pet the cockatoo");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion", meta = (ClampMin = "1"))
	int32 UnlockMechanicLevel = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion")
	EMelodiaCompanionBuffType BattleBuffType = EMelodiaCompanionBuffType::SkillPointRegen;

	/** Extra SP at battle start when companion is active (+1 per design). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion", meta = (ClampMin = "0"))
	int32 BonusSkillPointsAtBattleStart = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion", meta = (ClampMin = "50"))
	float FollowDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion", meta = (ClampMin = "1"))
	float FollowSpeed = 420.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Companion")
	bool bCompanionUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|Companion")
	bool bFollowingPlayer = false;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Companion")
	void SetCompanionUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintCallable, Category = "Melodia|Companion")
	void ApplyBattleStartBuff(class UMelodiaCombatStateComponent* CombatState) const;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Companion")
	void RefreshFollowTarget();

	/**
	 * Soft path for Blueprint child BP_CompanionController / BP_CockatooCompanion.
	 * Reparent a cosmetic mesh + VFX here; native code handles follow + buff logic.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|Companion")
	FSoftClassPath CompanionControllerClassPath = FSoftClassPath(TEXT("/Game/Melodia/Characters/Companions/BP_CockatooCompanion.BP_CockatooCompanion_C"));

	// IMelodiaInteractable
	virtual FString GetDisplayName_Implementation() const override;
	virtual FString GetInteractionPrompt_Implementation() const override;
	virtual bool ActivateInteraction_Implementation(APawn* InstigatorPawn) override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual USphereComponent* GetInteractionSphere_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TWeakObjectPtr<APawn> FollowTarget;

	void TickFollow(float DeltaSeconds);
	APawn* ResolvePlayerPawn() const;
};
