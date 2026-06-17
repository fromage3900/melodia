// Editor-only: create stub Melodia DataAssets when content folders are empty.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "EditorSubsystem.h"
#include "MelodiaEditorContentBootstrap.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaEditorContentBootstrap : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool EnsureDefaultContentAssets();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool EnsureTestLoopBlueprintAssets();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool EnsureTestLoopLevelAsset();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool RepopulateGameplayLoopTestLevel();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool EnsurePCGDemoLevelAsset();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool RepopulatePCGDemoLevel();

	UFUNCTION(BlueprintCallable, Category = "Melodia|Content")
	static bool EnsureMelodiaPortfolioMenuBridge();

private:
	static bool EnsureContentFolder(const FString& FolderPath);
	static bool SaveNewDataAsset(UObject* Asset, const FString& ObjectPath);
	static bool EnsureChildBlueprint(const FString& AssetPath, UClass* ParentClass);
	static bool BakeGameplayLoopTestLayout(UWorld* World);
};
#endif
