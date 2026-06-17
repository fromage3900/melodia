// Build-mode controller component: raycast -> grid cell, preview ghost, place/remove.

#include "BuildModeComponent.h"

#include "BuildGridManager.h"
#include "BuildPieceLibrary.h"
#include "BuildPieceDefinition.h"
#include "BuildPreviewGhost.h"

#include "BuildSaveGame.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

UBuildModeComponent::UBuildModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuildModeComponent::BeginPlay()
{
	Super::BeginPlay();

	Grid = Cast<ABuildGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ABuildGridManager::StaticClass()));

	if (bAutoBindPlayerInput)
	{
		BindBuildInput();
	}
}

void UBuildModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Ghost)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UBuildModeComponent::SetBuildModeEnabled(bool bEnabled)
{
	bBuildMode = bEnabled;

	if (!bBuildMode && Ghost)
	{
		Ghost->SetActorHiddenInGame(true);
	}

	if (bBuildMode)
	{
		EnsureGhost();
	}
}

UInputComponent* UBuildModeComponent::ResolveInputComponent() const
{
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->InputComponent)
		{
			return OwnerPawn->InputComponent;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			return PlayerController->InputComponent;
		}
	}

	return nullptr;
}

bool UBuildModeComponent::BindBuildInput()
{
	if (bInputBound)
	{
		return true;
	}

	UInputComponent* InputComponent = ResolveInputComponent();
	if (!InputComponent)
	{
		return false;
	}

	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &UBuildModeComponent::ToggleBuildMode);

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UBuildModeComponent::Place);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UBuildModeComponent::Remove);

	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &UBuildModeComponent::RotateLeft);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &UBuildModeComponent::RotateRight);

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &UBuildModeComponent::CyclePrev);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &UBuildModeComponent::CycleNext);

	InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &UBuildModeComponent::SaveBuildSlotKey);
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &UBuildModeComponent::LoadBuildSlotKey);

	bInputBound = true;
	return true;
}

void UBuildModeComponent::CyclePiece(int32 Delta)
{
	if (!PieceLibrary || PieceLibrary->Pieces.Num() == 0)
	{
		return;
	}

	PieceIndex = (PieceIndex + Delta) % PieceLibrary->Pieces.Num();
	if (PieceIndex < 0)
	{
		PieceIndex += PieceLibrary->Pieces.Num();
	}

	UpdateGhost();
}

void UBuildModeComponent::RotateYaw90(int32 DeltaSteps)
{
	RotationIdx = static_cast<uint8>((RotationIdx + (DeltaSteps % 4) + 4) % 4);
	UpdateGhost();
}

void UBuildModeComponent::ToggleBuildMode()
{
	SetBuildModeEnabled(!bBuildMode);
}

void UBuildModeComponent::RotateLeft()
{
	RotateYaw90(-1);
}

void UBuildModeComponent::RotateRight()
{
	RotateYaw90(1);
}

void UBuildModeComponent::CyclePrev()
{
	CyclePiece(-1);
}

void UBuildModeComponent::CycleNext()
{
	CyclePiece(1);
}

void UBuildModeComponent::SaveBuildSlotKey()
{
	SaveBuildSlot();
}

void UBuildModeComponent::LoadBuildSlotKey()
{
	LoadBuildSlot();
}

const UBuildPieceDefinition* UBuildModeComponent::GetSelectedPieceDef() const
{
	if (!PieceLibrary || PieceLibrary->Pieces.Num() == 0)
	{
		return nullptr;
	}

	const int32 ClampedIndex = FMath::Clamp(PieceIndex, 0, PieceLibrary->Pieces.Num() - 1);
	return PieceLibrary->Pieces[ClampedIndex].Get();
}

void UBuildModeComponent::EnsureGhost()
{
	if (Ghost || !PreviewGhostClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = GetOwner();

	Ghost = World->SpawnActor<ABuildPreviewGhost>(PreviewGhostClass, FTransform::Identity, Params);
	if (Ghost)
	{
		Ghost->SetActorHiddenInGame(false);
		UpdateGhost();
	}
}

bool UBuildModeComponent::UpdateTargetFromTrace()
{
	bHasTarget = false;
	bCanPlace = false;

	if (!Grid)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	APawn* Pawn = Cast<APawn>(Owner);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End = Start + (ViewRot.Vector() * TraceDistanceCm);

	FHitResult Hit;
	FCollisionQueryParams Q(TEXT("BuildModeTrace"), /*bTraceComplex*/ false, Owner);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q);
	if (!bHit)
	{
		return false;
	}

	// Place on the hit face by offsetting half a cell along the normal.
	const FVector SnapWorld = Hit.ImpactPoint + (Hit.ImpactNormal * (Grid->GridSizeCm * 0.5f));
	CurrentCell = Grid->WorldToCell(SnapWorld);

	const UBuildPieceDefinition* Def = GetSelectedPieceDef();
	const FName PieceId = Def ? Def->Id : NAME_None;

	bHasTarget = true;
	bCanPlace = Def && Grid->CanPlaceAtCell(CurrentCell, PieceId, RotationIdx);
	return true;
}

void UBuildModeComponent::UpdateGhost()
{
	if (!Ghost)
	{
		return;
	}

	const UBuildPieceDefinition* Def = GetSelectedPieceDef();
	Ghost->SetMesh(Def ? Def->Mesh.Get() : nullptr);
}

void UBuildModeComponent::DrawBuildHUD() const
{
	if (!GEngine)
	{
		return;
	}

	const UBuildPieceDefinition* Def = GetSelectedPieceDef();
	const FString PieceName = Def ? Def->Id.ToString() : TEXT("(none)");
	const FString Status = bCanPlace ? TEXT("OK") : TEXT("BLOCKED");
	const FString Msg = FString::Printf(
		TEXT("[Build Mode] Piece: %s | Rot: %d | %s\nB toggle | LMB place | RMB remove | Q/E rotate | 1/2 cycle | F5 save | F9 load"),
		*PieceName, static_cast<int32>(RotationIdx), *Status);

	GEngine->AddOnScreenDebugMessage(424242, 0.0f, bCanPlace ? FColor::Green : FColor::Red, Msg);
}

void UBuildModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBuildMode)
	{
		return;
	}

	EnsureGhost();
	if (!Ghost || !Grid)
	{
		return;
	}

	Ghost->SetActorHiddenInGame(false);

	if (UpdateTargetFromTrace())
	{
		const float Yaw = static_cast<float>(RotationIdx % 4) * 90.0f;
		Ghost->SetActorLocation(Grid->CellToWorld(CurrentCell));
		Ghost->SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
		Ghost->SetValidity(bCanPlace);
	}
	else
	{
		Ghost->SetValidity(false);
	}

	DrawBuildHUD();
}

void UBuildModeComponent::Place()
{
	if (!bBuildMode || !Grid || !bHasTarget || !bCanPlace)
	{
		return;
	}

	const UBuildPieceDefinition* Def = GetSelectedPieceDef();
	if (!Def)
	{
		return;
	}

	Grid->PlaceAtCell(CurrentCell, Def->Id, RotationIdx);
}

void UBuildModeComponent::Remove()
{
	if (!bBuildMode || !Grid || !bHasTarget)
	{
		return;
	}

	Grid->RemoveAtCell(CurrentCell);
}

bool UBuildModeComponent::SaveBuildSlot()
{
	if (!Grid)
	{
		return false;
	}

	UBuildSaveGame* Save = Cast<UBuildSaveGame>(UGameplayStatics::CreateSaveGameObject(UBuildSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	Grid->GetPlacedPieces(Save->Pieces);
	return UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, SaveUserIndex);
}

bool UBuildModeComponent::LoadBuildSlot()
{
	if (!Grid)
	{
		return false;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	UBuildSaveGame* Save = Cast<UBuildSaveGame>(Loaded);
	if (!Save)
	{
		return false;
	}

	Grid->ClearAllPieces();

	TArray<FPlacedPieceSaveData> Pieces = Save->Pieces;
	Pieces.Sort([](const FPlacedPieceSaveData& A, const FPlacedPieceSaveData& B)
	{
		if (A.Cell.Z != B.Cell.Z) return A.Cell.Z < B.Cell.Z;
		if (A.Cell.X != B.Cell.X) return A.Cell.X < B.Cell.X;
		return A.Cell.Y < B.Cell.Y;
	});

	for (const FPlacedPieceSaveData& P : Pieces)
	{
		Grid->PlaceAtCell(P.Cell, P.PieceId, P.RotationIdx);
	}

	return true;
}

