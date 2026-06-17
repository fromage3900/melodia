// Build piece library lookup helpers.

#include "BuildPieceLibrary.h"
#include "BuildPieceDefinition.h"

UBuildPieceDefinition* UBuildPieceLibrary::FindById(FName Id) const
{
	for (UBuildPieceDefinition* Piece : Pieces)
	{
		if (Piece && Piece->Id == Id)
		{
			return Piece;
		}
	}
	return nullptr;
}

