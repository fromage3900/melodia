// Data asset holding a list of build pieces for cycling/picking.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildPieceLibrary.generated.h"

class UBuildPieceDefinition;

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UBuildPieceLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	TArray<TObjectPtr<UBuildPieceDefinition>> Pieces;

	UFUNCTION(BlueprintCallable, Category="Build")
	UBuildPieceDefinition* FindById(FName Id) const;
};

