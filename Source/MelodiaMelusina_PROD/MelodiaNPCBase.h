// Blueprintable quest NPC base — activates quests via IMelodiaInteractable.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaInteractable.h"
#include "MelodiaNPCBase.generated.h"

class APawn;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaNPCBase : public AActor, public IMelodiaInteractable
{
	GENERATED_BODY()

public:
	AMelodiaNPCBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|NPC")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Melodia|NPC")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|NPC")
	FString DisplayName = TEXT("Melodia Tutor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|NPC")
	FString InteractionPrompt = TEXT("F: Talk");

	/** Quest activated when the player interacts (e.g. RisingMelody, KeyOfForte). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|NPC")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|NPC")
	FString DialogueLine = TEXT("Welcome, apprentice musician.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|NPC", meta = (ClampMin = "0"))
	int32 RequiredMechanicLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Melodia|NPC")
	int32 InteractionCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Melodia|NPC")
	bool ActivateQuestForPawn(APawn* InstigatorPawn);

	// IMelodiaInteractable
	virtual FString GetDisplayName_Implementation() const override;
	virtual FString GetInteractionPrompt_Implementation() const override;
	virtual bool ActivateInteraction_Implementation(APawn* InstigatorPawn) override;
	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual USphereComponent* GetInteractionSphere_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	void PublishDialogue(const FString& Text) const;
	bool IsMechanicLevelSufficient() const;
};
