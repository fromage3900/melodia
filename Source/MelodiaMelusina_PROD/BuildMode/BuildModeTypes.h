// Build-mode shared types (grid coords, placed piece info).

#pragma once

#include "CoreMinimal.h"
#include "BuildModeTypes.generated.h"

UENUM(BlueprintType)
enum class EBuildSurfaceRule : uint8
{
	Any UMETA(DisplayName="Any"),
	FloorOnly UMETA(DisplayName="Floor Only"),
	WallOnly UMETA(DisplayName="Wall Only"),
	CeilingOnly UMETA(DisplayName="Ceiling Only"),
};

USTRUCT(BlueprintType)
struct FPlacedPiece
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PieceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 RotationIdx = 0; // 0..3 = yaw 0/90/180/270
};

USTRUCT(BlueprintType)
struct FPlacedPieceSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector Cell = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PieceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 RotationIdx = 0;
};

