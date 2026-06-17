// Build-mode controller component: raycast -> grid cell, preview ghost, place/remove.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildModeComponent.generated.h"

class ABuildGridManager;
class ABuildPreviewGhost;
class UBuildPieceLibrary;
class UBuildPieceDefinition;
class UInputComponent;

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UBuildModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildModeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	TObjectPtr<UBuildPieceLibrary> PieceLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	TSubclassOf<ABuildPreviewGhost> PreviewGhostClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	float TraceDistanceCm = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	bool bAutoBindPlayerInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	FString SaveSlotName = TEXT("BuildMode");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build")
	int32 SaveUserIndex = 0;

	UFUNCTION(BlueprintCallable, Category="Build")
	void SetBuildModeEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Build")
	bool IsBuildModeEnabled() const { return bBuildMode; }

	UFUNCTION(BlueprintCallable, Category="Build")
	void CyclePiece(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="Build")
	void RotateYaw90(int32 DeltaSteps);

	UFUNCTION(BlueprintCallable, Category="Build")
	void Place();

	UFUNCTION(BlueprintCallable, Category="Build")
	void Remove();

	UFUNCTION(BlueprintCallable, Category="Build")
	bool BindBuildInput();

	UFUNCTION(BlueprintCallable, Category="Build")
	bool SaveBuildSlot();

	UFUNCTION(BlueprintCallable, Category="Build")
	bool LoadBuildSlot();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bBuildMode = false;
	int32 PieceIndex = 0;
	uint8 RotationIdx = 0;

	UPROPERTY()
	TObjectPtr<ABuildGridManager> Grid;

	UPROPERTY()
	TObjectPtr<ABuildPreviewGhost> Ghost;

	FIntVector CurrentCell = FIntVector::ZeroValue;
	bool bHasTarget = false;
	bool bCanPlace = false;
	bool bInputBound = false;

	const UBuildPieceDefinition* GetSelectedPieceDef() const;
	bool UpdateTargetFromTrace();
	void EnsureGhost();
	void UpdateGhost();

	UInputComponent* ResolveInputComponent() const;
	void DrawBuildHUD() const;

	// Keybind wrappers (BindKey needs parameterless handlers).
	void ToggleBuildMode();
	void RotateLeft();
	void RotateRight();
	void CyclePrev();
	void CycleNext();
	void SaveBuildSlotKey();
	void LoadBuildSlotKey();
};

