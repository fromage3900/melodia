// Phoenix JRPG template adapter.

#include "MelodiaJRPGPresenter.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaJRPGBridgeLibrary.h"
#include "MelodiaBattleBridgeLibrary.h"
#include "UObject/UnrealType.h"

bool UMelodiaJRPGPresenter::InitializeEncounter(
	UObject* WorldContextObject,
	const FMelodiaEncounterDefinition& Encounter,
	const bool bSuppressPhoenixBattleUI)
{
	AActor* BattleController = Encounter.BattleController;
	AActor* BattleData = Encounter.BattleData;
	if (!BattleController || !BattleData)
	{
		return false;
	}

	if (FObjectPropertyBase* CurrentBattleProperty = CastField<FObjectPropertyBase>(
		BattleController->GetClass()->FindPropertyByName(TEXT("currentBattle"))))
	{
		CurrentBattleProperty->SetObjectPropertyValue_InContainer(BattleController, BattleData);
	}

	const FName InitCandidates[] = { TEXT("StartBattle"), TEXT("InitBattle"), TEXT("SwitchToBattleMode") };
	for (const FName FunctionName : InitCandidates)
	{
		UFunction* Function = BattleController->FindFunction(FunctionName);
		if (!Function)
		{
			continue;
		}

		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Function->ParmsSize, 1)));
		FMemory::Memzero(Params, Function->ParmsSize);
		APlayerController* PlayerController = WorldContextObject
			? UGameplayStatics::GetPlayerController(WorldContextObject, 0)
			: nullptr;
		int32 ObjectInputIndex = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}

			It->InitializeValue_InContainer(Params);
			if (It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				continue;
			}

			if (FObjectPropertyBase* ObjectParam = CastField<FObjectPropertyBase>(*It))
			{
				const FString ParamName = It->GetAuthoredName();
				UObject* Value = nullptr;
				if (ParamName.Contains(TEXT("Controller")) || ParamName.Contains(TEXT("Player")))
				{
					Value = PlayerController;
				}
				else if (ParamName.Contains(TEXT("Battle")))
				{
					Value = BattleData;
				}
				else
				{
					Value = ObjectInputIndex == 0 ? BattleData : Cast<UObject>(PlayerController);
				}

				ObjectParam->SetObjectPropertyValue_InContainer(Params, Value);
				++ObjectInputIndex;
			}
		}

		BattleController->ProcessEvent(Function, Params);

		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				It->DestroyValue_InContainer(Params);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Melodia JRPG presenter initialized encounter via %s."), *FunctionName.ToString());
		break;
	}

	if (bSuppressPhoenixBattleUI)
	{
		UMelodiaJRPGBridgeLibrary::SyncPartyUnitsFromSubsystem(WorldContextObject, BattleController);
		PrepareBattlePresentation(WorldContextObject, BattleController, true);
	}
	else
	{
		UMelodiaJRPGBridgeLibrary::EnsurePhoenixBattleUnitsVisible(BattleController);
		UMelodiaJRPGBridgeLibrary::SyncPartyUnitsFromSubsystem(WorldContextObject, BattleController);
	}

	return true;
}

bool UMelodiaJRPGPresenter::PrepareBattlePresentation(
	UObject* WorldContextObject,
	AActor* BattleController,
	const bool bStripPhoenixUI)
{
	if (!BattleController)
	{
		return false;
	}

	BattleController->SetActorHiddenInGame(false);
	BattleController->SetActorEnableCollision(true);
	UMelodiaJRPGBridgeLibrary::EnsurePhoenixBattleUnitsVisible(BattleController);

	if (bStripPhoenixUI)
	{
		UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(BattleController, EMelodiaPhoenixTeardownScope::WidgetsOnly);
	}

	if (APlayerController* PlayerController = WorldContextObject
		? UGameplayStatics::GetPlayerController(WorldContextObject, 0)
		: nullptr)
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->bShowMouseCursor = false;
		UMelodiaJRPGBridgeLibrary::ApplyPhoenixBattleCameraView(PlayerController, BattleController);
	}

	return true;
}

void UMelodiaJRPGPresenter::TeardownPresentation(AActor* BattleController)
{
	UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(BattleController, EMelodiaPhoenixTeardownScope::Full);
}

bool UMelodiaJRPGPresenter::TryFleePresentation(AActor* BattleController)
{
	return UMelodiaJRPGBridgeLibrary::TryFleeBattle(BattleController);
}

bool UMelodiaJRPGPresenter::TryRoutePhoenixSkillMenuToSession(
	UObject* WorldContextObject,
	AActor* BattleController,
	const int32 MenuSkillIndex)
{
	(void)BattleController;
	return UMelodiaBattleBridgeLibrary::PhoenixSubmitSkillByMenuIndex(WorldContextObject, MenuSkillIndex);
}
