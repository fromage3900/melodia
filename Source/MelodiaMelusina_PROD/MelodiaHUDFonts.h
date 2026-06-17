#pragma once

#include "Fonts/SlateFontInfo.h"

/** Runtime TTF font lookup for native Slate HUD painting (no .uasset required). */
namespace MelodiaHUDFonts
{
	enum class ERole : uint8
	{
		Body,
		Bold,
		Title,
		Accent,
		Musical,
	};

	/** Returns FSlateFontInfo backed by Content/Melodia/UI/Fonts/*.ttf */
	FSlateFontInfo Get(ERole Role, int32 Size);

	/** Pre-register font paths (optional; first Get() also works). */
	void Warmup();
}
