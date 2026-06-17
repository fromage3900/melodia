// Lightweight preview actor for build placement.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildPreviewGhost.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class MELODIAMELUSINA_PROD_API ABuildPreviewGhost : public AActor
{
	GENERATED_BODY()

public:
	ABuildPreviewGhost();

	UFUNCTION(BlueprintCallable, Category="Build")
	void SetMesh(UStaticMesh* Mesh);

	UFUNCTION(BlueprintCallable, Category="Build")
	void SetValidity(bool bIsValid);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(EditDefaultsOnly, Category="Build")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID;
};

