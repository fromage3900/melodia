// Copyright Melodia Project. All Rights Reserved.

#include "PCGMelodiaProjectLandscapeSettings.h"
#include "MelodiaPCGTerrain.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "PCGPin.h"

FPCGElementPtr UPCGMelodiaProjectLandscapeSettings::CreateElement() const
{
	return MakeShared<FPCGMelodiaProjectLandscapeElement>();
}

bool FPCGMelodiaProjectLandscapeElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGMelodiaProjectLandscapeElement::ExecuteInternal);
	check(Context);

	const UPCGMelodiaProjectLandscapeSettings* Settings =
		Context->GetInputSettings<UPCGMelodiaProjectLandscapeSettings>();
	check(Settings);

	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	UWorld* World = MelodiaPCGTerrain::GetWorldFromPCGContext(Context);

	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGPointData* InputPointData = Cast<UPCGPointData>(Input.Data);
		if (!InputPointData)
		{
			continue;
		}

		UPCGPointData* OutputPointData = FPCGContext::NewObject_AnyThread<UPCGPointData>(Context);
		OutputPointData->InitializeFromData(InputPointData);
		MelodiaPCGTerrain::ApplyToPointData(OutputPointData, World, Settings->TerrainProjection);

		FPCGTaggedData& Output = Context->OutputData.TaggedData.AddDefaulted_GetRef();
		Output.Data = OutputPointData;
		Output.Pin = PCGPinConstants::DefaultOutputLabel;
	}

	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
