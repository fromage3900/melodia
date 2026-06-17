// SaveGame for placed build pieces (grid coord + piece + rotation).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BuildModeTypes.h"
#include "BuildSaveGame.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UBuildSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Build")
	TArray<FPlacedPieceSaveData> Pieces;
};

