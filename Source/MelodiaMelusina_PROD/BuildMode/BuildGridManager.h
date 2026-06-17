// Grid manager that stores placed pieces and renders them via HISMs.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildModeTypes.h"
#include "BuildGridManager.generated.h"

class UBuildPieceLibrary;
class UBuildPieceDefinition;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API ABuildGridManager : public AActor
{
	GENERATED_BODY()

public:
	ABuildGridManager();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	float GridSizeCm = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	TObjectPtr<UBuildPieceLibrary> PieceLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	bool bRequireSupport = false;

	UFUNCTION(BlueprintCallable, Category="Build")
	FVector CellToWorld(const FIntVector& Cell) const;

	UFUNCTION(BlueprintCallable, Category="Build")
	FIntVector WorldToCell(const FVector& World) const;

	UFUNCTION(BlueprintCallable, Category="Build")
	bool CanPlaceAtCell(const FIntVector& Cell, FName PieceId, uint8 RotationIdx) const;

	UFUNCTION(BlueprintCallable, Category="Build")
	bool PlaceAtCell(const FIntVector& Cell, FName PieceId, uint8 RotationIdx);

	UFUNCTION(BlueprintCallable, Category="Build")
	bool RemoveAtCell(const FIntVector& Cell);

	// Simple extraction for saving.
	UFUNCTION(BlueprintCallable, Category="Build")
	void GetPlacedPieces(TArray<FPlacedPieceSaveData>& OutPieces) const;

	// Simple restore for loading.
	UFUNCTION(BlueprintCallable, Category="Build")
	void ClearAllPieces();

private:
	UPROPERTY()
	TMap<FIntVector, FPlacedPiece> Placed;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HismByPiece;

	const UBuildPieceDefinition* GetPieceDef(FName PieceId) const;
	UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(const UBuildPieceDefinition* Def);
	FTransform MakePieceTransform(const FIntVector& Cell, uint8 RotationIdx) const;
	bool HasSupportAtCell(const FIntVector& Cell) const;
};

