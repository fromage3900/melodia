// Data asset describing a placeable build piece.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildModeTypes.h"
#include "BuildPieceDefinition.generated.h"

UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UBuildPieceDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	FIntVector ExtentsCells = FIntVector(1, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	EBuildSurfaceRule SurfaceRule = EBuildSurfaceRule::Any;
};

