// Grid manager that stores placed pieces and renders them via HISMs.

#include "BuildGridManager.h"

#include "BuildPieceLibrary.h"
#include "BuildPieceDefinition.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

ABuildGridManager::ABuildGridManager()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

FVector ABuildGridManager::CellToWorld(const FIntVector& Cell) const
{
	return FVector(
		static_cast<float>(Cell.X) * GridSizeCm,
		static_cast<float>(Cell.Y) * GridSizeCm,
		static_cast<float>(Cell.Z) * GridSizeCm
	);
}

FIntVector ABuildGridManager::WorldToCell(const FVector& World) const
{
	return FIntVector(
		FMath::RoundToInt(World.X / GridSizeCm),
		FMath::RoundToInt(World.Y / GridSizeCm),
		FMath::RoundToInt(World.Z / GridSizeCm)
	);
}

bool ABuildGridManager::HasSupportAtCell(const FIntVector& Cell) const
{
	// Ground layer is always supported.
	if (Cell.Z <= 0)
	{
		return true;
	}
	const FIntVector Below = Cell + FIntVector(0, 0, -1);
	return Placed.Contains(Below);
}

FTransform ABuildGridManager::MakePieceTransform(const FIntVector& Cell, uint8 RotationIdx) const
{
	const float Yaw = static_cast<float>(RotationIdx % 4) * 90.0f;
	return FTransform(FRotator(0.0f, Yaw, 0.0f), CellToWorld(Cell));
}

const UBuildPieceDefinition* ABuildGridManager::GetPieceDef(FName PieceId) const
{
	if (!PieceLibrary)
	{
		return nullptr;
	}
	return PieceLibrary->FindById(PieceId);
}

UHierarchicalInstancedStaticMeshComponent* ABuildGridManager::GetOrCreateHISM(const UBuildPieceDefinition* Def)
{
	if (!Def || !Def->Mesh)
	{
		return nullptr;
	}

	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing = HismByPiece.Find(Def->Id))
	{
		return Existing->Get();
	}

	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	if (!HISM)
	{
		return nullptr;
	}

	HISM->SetupAttachment(GetRootComponent());
	HISM->RegisterComponent();
	HISM->SetStaticMesh(Def->Mesh);
	HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISM->SetMobility(EComponentMobility::Static);
	HISM->bCastDynamicShadow = false;

	HismByPiece.Add(Def->Id, HISM);
	return HISM;
}

bool ABuildGridManager::CanPlaceAtCell(const FIntVector& Cell, FName PieceId, uint8 RotationIdx) const
{
	if (PieceId.IsNone())
	{
		return false;
	}

	const UBuildPieceDefinition* Def = GetPieceDef(PieceId);
	if (!Def || !Def->Mesh)
	{
		return false;
	}

	// v0: single-cell occupancy only (ExtentsCells reserved for later).
	if (Placed.Contains(Cell))
	{
		return false;
	}

	if (bRequireSupport && !HasSupportAtCell(Cell))
	{
		return false;
	}

	(void)RotationIdx; // reserved for multi-cell bounds checks later
	return true;
}

bool ABuildGridManager::PlaceAtCell(const FIntVector& Cell, FName PieceId, uint8 RotationIdx)
{
	if (!CanPlaceAtCell(Cell, PieceId, RotationIdx))
	{
		return false;
	}

	const UBuildPieceDefinition* Def = GetPieceDef(PieceId);
	UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Def);
	if (!HISM)
	{
		return false;
	}

	const FTransform Xform = MakePieceTransform(Cell, RotationIdx);
	HISM->AddInstance(Xform);

	FPlacedPiece PP;
	PP.PieceId = PieceId;
	PP.RotationIdx = RotationIdx;
	Placed.Add(Cell, PP);

	return true;
}

bool ABuildGridManager::RemoveAtCell(const FIntVector& Cell)
{
	FPlacedPiece* Existing = Placed.Find(Cell);
	if (!Existing)
	{
		return false;
	}

	// v0: simple and safe. Rebuild the HISM for this piece ID rather than tracking instance indices.
	const FName PieceId = Existing->PieceId;
	Placed.Remove(Cell);

	const UBuildPieceDefinition* Def = GetPieceDef(PieceId);
	UHierarchicalInstancedStaticMeshComponent* HISM = Def ? HismByPiece.FindRef(Def->Id).Get() : nullptr;
	if (!HISM)
	{
		return true;
	}

	HISM->ClearInstances();
	for (const TPair<FIntVector, FPlacedPiece>& Pair : Placed)
	{
		if (Pair.Value.PieceId == PieceId)
		{
			HISM->AddInstance(MakePieceTransform(Pair.Key, Pair.Value.RotationIdx));
		}
	}
	return true;
}

void ABuildGridManager::GetPlacedPieces(TArray<FPlacedPieceSaveData>& OutPieces) const
{
	OutPieces.Reset();
	OutPieces.Reserve(Placed.Num());

	for (const TPair<FIntVector, FPlacedPiece>& Pair : Placed)
	{
		FPlacedPieceSaveData D;
		D.Cell = Pair.Key;
		D.PieceId = Pair.Value.PieceId;
		D.RotationIdx = Pair.Value.RotationIdx;
		OutPieces.Add(D);
	}
}

void ABuildGridManager::ClearAllPieces()
{
	Placed.Reset();
	for (TPair<FName, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Pair : HismByPiece)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearInstances();
		}
	}
}

