// Save data for the Melodia exploration loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MelodiaSaveGame.generated.h"

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UMelodiaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
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
};
