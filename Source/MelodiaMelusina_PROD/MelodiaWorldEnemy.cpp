#include "MelodiaWorldEnemy.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMelodiaWorldEnemy::AMelodiaWorldEnemy()
{
	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		VisualMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.85f));
	}

	if (TriggerSphere)
	{
		TriggerSphere->InitSphereRadius(220.0f);
	}
}

void AMelodiaWorldEnemy::BeginPlay()
{
	ApplySlimePresentation();
	Super::BeginPlay();
}

void AMelodiaWorldEnemy::ApplySlimePresentation()
{
	if (!VisualMesh)
	{
		return;
	}

	UMaterialInstanceDynamic* SlimeMaterial = VisualMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (SlimeMaterial)
	{
		SlimeMaterial->SetVectorParameterValue(TEXT("BaseColor"), SlimeTint);
		SlimeMaterial->SetVectorParameterValue(TEXT("Color"), SlimeTint);
	}
}
