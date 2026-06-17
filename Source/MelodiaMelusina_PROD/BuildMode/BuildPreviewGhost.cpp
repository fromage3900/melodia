// Lightweight preview actor for build placement.

#include "BuildPreviewGhost.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ABuildPreviewGhost::ABuildPreviewGhost()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComp);

	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCastShadow(false);
	MeshComp->bReceivesDecals = false;
}

void ABuildPreviewGhost::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MID && PreviewMaterial && MeshComp)
	{
		MID = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
		MeshComp->SetMaterial(0, MID);
	}
}

void ABuildPreviewGhost::SetMesh(UStaticMesh* Mesh)
{
	if (MeshComp)
	{
		MeshComp->SetStaticMesh(Mesh);
	}
}

void ABuildPreviewGhost::SetValidity(bool bIsValid)
{
	if (!MID)
	{
		return;
	}

	// Convention: if the material supports these params, great; otherwise no-op.
	MID->SetScalarParameterValue(TEXT("Valid"), bIsValid ? 1.0f : 0.0f);
	MID->SetVectorParameterValue(TEXT("TintColor"), bIsValid ? FLinearColor(0.25f, 1.0f, 0.35f, 0.35f) : FLinearColor(1.0f, 0.25f, 0.25f, 0.35f));
}

