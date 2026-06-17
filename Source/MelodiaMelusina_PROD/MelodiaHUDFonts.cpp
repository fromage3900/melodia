#include "MelodiaHUDFonts.h"

#include "Misc/Paths.h"

namespace
{
FString RoleToFileName(const MelodiaHUDFonts::ERole Role)
{
	switch (Role)
	{
	case MelodiaHUDFonts::ERole::Bold:
		return TEXT("LifeSavers-Bold.ttf");
	case MelodiaHUDFonts::ERole::Title:
		return TEXT("CherryBombOne-Regular.ttf");
	case MelodiaHUDFonts::ERole::Accent:
		return TEXT("TwinkleStar-Regular.ttf");
	case MelodiaHUDFonts::ERole::Musical:
		return TEXT("NotoMusic-Regular.ttf");
	case MelodiaHUDFonts::ERole::Body:
	default:
		return TEXT("LifeSavers-Regular.ttf");
	}
}

FString BuildFontPath(const MelodiaHUDFonts::ERole Role)
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Melodia/UI/Fonts"), RoleToFileName(Role));
}
}

FSlateFontInfo MelodiaHUDFonts::Get(const ERole Role, const int32 Size)
{
	const int32 ClampedSize = FMath::Clamp(Size, 8, 72);
	const FString FontPath = BuildFontPath(Role);
	const FString CacheKey = FString::Printf(TEXT("%s_%d"), *FontPath, ClampedSize);

	static TMap<FString, FSlateFontInfo> Cache;
	if (const FSlateFontInfo* Cached = Cache.Find(CacheKey))
	{
		return *Cached;
	}

	FSlateFontInfo FontInfo(FPaths::ConvertRelativePathToFull(FontPath), static_cast<float>(ClampedSize));
	Cache.Add(CacheKey, FontInfo);
	return FontInfo;
}

void MelodiaHUDFonts::Warmup()
{
	for (const ERole Role : { ERole::Body, ERole::Bold, ERole::Title, ERole::Accent, ERole::Musical })
	{
		(void)Get(Role, 14);
	}
}
