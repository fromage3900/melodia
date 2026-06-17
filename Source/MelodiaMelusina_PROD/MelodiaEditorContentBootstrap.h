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

private:
	static bool EnsureContentFolder(const FString& FolderPath);
	static bool SaveNewDataAsset(UObject* Asset, const FString& ObjectPath);
};
#endif
