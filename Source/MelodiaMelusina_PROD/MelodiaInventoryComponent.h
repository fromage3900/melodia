// Lightweight inventory for exploration rewards and quest items.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaQuestTypes.h"
#include "MelodiaInventoryComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Inventory")
	int32 MaxSlots = 12;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Inventory")
	TArray<FMelodiaInventorySlot> Slots;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Inventory")
	int32 TotalItemsCollected = 0;

	UFUNCTION(BlueprintCallable, Category="Melodia|Inventory")
	bool AddItem(FName ItemId, const FString& DisplayName, int32 Quantity, FLinearColor IconTint);

	UFUNCTION(BlueprintPure, Category="Melodia|Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category="Melodia|Inventory")
	void SeedStarterKit();
};
