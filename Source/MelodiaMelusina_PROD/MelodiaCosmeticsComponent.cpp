// Runtime cosmetic preset application for Melodia characters.

#include "MelodiaCosmeticsComponent.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UMelodiaCosmeticsComponent::UMelodiaCosmeticsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMelodiaCosmeticsComponent::ApplyCosmeticPreset(const FMelodiaCosmeticPreset& Preset)
{
	USkeletalMeshComponent* Mesh = FindTargetMesh();
	if (!Mesh)
	{
		bLastApplySucceeded = false;
		return false;
	}

	CurrentPreset = Preset;

	bool bAppliedMaterial = false;
	if (UMaterialInterface* BodyMaterial = Preset.BodyMaterial.LoadSynchronous())
	{
		Mesh->SetMaterial(0, BodyMaterial);
		bAppliedMaterial = true;
	}

	if (UMaterialInterface* AccentMaterial = Preset.AccentMaterial.LoadSynchronous())
	{
		const int32 AccentSlot = Mesh->GetNumMaterials() > 1 ? 1 : 0;
		Mesh->SetMaterial(AccentSlot, AccentMaterial);
		bAppliedMaterial = true;
	}

	LastAppliedPresetId = Preset.PresetId;
	bSparkleCosmeticEnabled = Preset.bEnableSparkles;
	bLastApplySucceeded = bAppliedMaterial || Preset.BodyMaterial.IsNull() || Preset.AccentMaterial.IsNull();
	++AppliedCosmeticCount;

	UE_LOG(LogTemp, Log, TEXT("Melodia cosmetics applied preset %s to %s."),
		*LastAppliedPresetId.ToString(),
		GetOwner() ? *GetOwner()->GetName() : TEXT("unknown owner"));

	return bLastApplySucceeded;
}

bool UMelodiaCosmeticsComponent::ApplyDefaultMelusinaPreset()
{
	FMelodiaCosmeticPreset Preset;
	Preset.PresetId = TEXT("MelusinaDefault");
	Preset.DisplayName = NSLOCTEXT("MelodiaCosmetics", "DefaultMelusinaPreset", "Melusina Atelier");
	Preset.BodyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/_PROJECT/04_Materials/M_Palette_Melusina.M_Palette_Melusina")));
	Preset.AccentMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/_PROJECT/04_Materials/MooaToon/Glitter/M_Glitter_UltimateSparkling.M_Glitter_UltimateSparkling")));
	Preset.PrimaryTint = FLinearColor(0.84f, 0.72f, 1.0f, 1.0f);
	Preset.AccentTint = FLinearColor(1.0f, 0.82f, 0.38f, 1.0f);
	Preset.bEnableSparkles = true;
	return ApplyCosmeticPreset(Preset);
}

USkeletalMeshComponent* UMelodiaCosmeticsComponent::FindTargetMesh() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
		{
			return CharacterMesh;
		}
	}

	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}
