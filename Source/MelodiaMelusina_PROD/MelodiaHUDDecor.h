#pragma once

#include "Styling/SlateBrush.h"

class FSlateWindowElementList;
struct FGeometry;

/** Loads Kenney CC0 PNG decorations from Content/Melodia/UI/Sprites/Kenney for Slate HUD. */
namespace MelodiaHUDDecor
{
	void Warmup();

	bool IsReady();

	const FSlateBrush* GetStarBrush();
	const FSlateBrush* GetSparkBrush();
	const FSlateBrush* GetMagicBrush();
	const FSlateBrush* GetFrameBrush();

	void PaintDecorImage(
		FSlateWindowElementList& OutDrawElements,
		const FGeometry& Geometry,
		int32 LayerId,
		const FSlateBrush* Brush,
		const FVector2f& Position,
		const FVector2f& Size,
		const FLinearColor& Tint);
}
