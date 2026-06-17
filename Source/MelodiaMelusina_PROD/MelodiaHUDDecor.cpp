#include "MelodiaHUDDecor.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

namespace
{
struct FDecorEntry
{
	FString FileName;
	TSharedPtr<FSlateDynamicImageBrush> Brush;
	FVector2D Size = FVector2D::ZeroVector;
};

TArray<FDecorEntry>& GetDecorEntries()
{
	static TArray<FDecorEntry> Entries;
	return Entries;
}

bool bDecorInitialized = false;

UTexture2D* CreateTextureFromPNG(const FString& AbsolutePath, int32& OutWidth, int32& OutHeight)
{
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *AbsolutePath))
	{
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		return nullptr;
	}

	OutWidth = static_cast<int32>(ImageWrapper->GetWidth());
	OutHeight = static_cast<int32>(ImageWrapper->GetHeight());
	TArray<uint8> UncompressedBGRA;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
	{
		return nullptr;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(OutWidth, OutHeight, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}

	Texture->NeverStream = true;
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		return nullptr;
	}

	void* TextureData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
	PlatformData->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

void LoadDecorBrush(const FString& FileName)
{
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Melodia/UI/Sprites/Kenney"), FileName));
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*AbsolutePath))
	{
		return;
	}

	int32 Width = 0;
	int32 Height = 0;
	UTexture2D* Texture = CreateTextureFromPNG(AbsolutePath, Width, Height);
	if (!Texture || Width <= 0 || Height <= 0)
	{
		return;
	}

	FDecorEntry Entry;
	Entry.FileName = FileName;
	Entry.Size = FVector2D(static_cast<float>(Width), static_cast<float>(Height));
	Entry.Brush = MakeShared<FSlateDynamicImageBrush>(Texture, Entry.Size, FName(*FileName));
	GetDecorEntries().Add(Entry);
}

const FSlateBrush* FindDecorBrush(const TCHAR* FileName)
{
	for (const FDecorEntry& Entry : GetDecorEntries())
	{
		if (Entry.FileName.Equals(FileName) && Entry.Brush.IsValid())
		{
			return Entry.Brush.Get();
		}
	}
	return nullptr;
}

FPaintGeometry DecorPaintBox(const FGeometry& Geometry, const FVector2f Position, const FVector2f Size)
{
	return Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position));
}
}

void MelodiaHUDDecor::Warmup()
{
	if (bDecorInitialized)
	{
		return;
	}

	LoadDecorBrush(TEXT("star_04.png"));
	LoadDecorBrush(TEXT("spark_04.png"));
	LoadDecorBrush(TEXT("magic_03.png"));
	LoadDecorBrush(TEXT("window_03.png"));
	LoadDecorBrush(TEXT("star_05.png"));
	LoadDecorBrush(TEXT("light_02.png"));
	bDecorInitialized = true;
}

bool MelodiaHUDDecor::IsReady()
{
	return bDecorInitialized && GetDecorEntries().Num() > 0;
}

const FSlateBrush* MelodiaHUDDecor::GetStarBrush()
{
	return FindDecorBrush(TEXT("star_04.png"));
}

const FSlateBrush* MelodiaHUDDecor::GetSparkBrush()
{
	return FindDecorBrush(TEXT("spark_04.png"));
}

const FSlateBrush* MelodiaHUDDecor::GetMagicBrush()
{
	return FindDecorBrush(TEXT("magic_03.png"));
}

const FSlateBrush* MelodiaHUDDecor::GetFrameBrush()
{
	return FindDecorBrush(TEXT("window_03.png"));
}

void MelodiaHUDDecor::PaintDecorImage(
	FSlateWindowElementList& OutDrawElements,
	const FGeometry& Geometry,
	const int32 LayerId,
	const FSlateBrush* Brush,
	const FVector2f& Position,
	const FVector2f& Size,
	const FLinearColor& Tint)
{
	if (!Brush)
	{
		return;
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		DecorPaintBox(Geometry, Position, Size),
		Brush,
		ESlateDrawEffect::None,
		Tint);
}
