// Fill out your copyright notice in the Description page of Project Settings.

#include "MelodiaMelusina_PROD.h"
#include "MelodiaDevCheats.h"
#include "Modules/ModuleManager.h"

class FMelodiaMelusina_PRODModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		MelodiaDevCheats::RegisterConsoleCommands();
	}

	virtual void ShutdownModule() override
	{
		MelodiaDevCheats::UnregisterConsoleCommands();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMelodiaMelusina_PRODModule, MelodiaMelusina_PROD, "MelodiaMelusina_PROD");
