// Reflection bridge between the native rhythm model and the TurnBasedJRPG template units.

#include "MelodiaJRPGBridgeLibrary.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "MelodiaPartySubsystem.h"
#include "UObject/UnrealType.h"

namespace
{
const FName EnemyArrayName(TEXT("enemyUnits"));
const FName PartyArrayName(TEXT("playerUnits"));
const FName CurrentBattleName(TEXT("currentBattle"));
const FName CurrentAttackingUnitName(TEXT("currentAttackingUnit"));
const FName CurrentTargetUnitName(TEXT("currentTargetUnit"));
const FName CurrentHPName(TEXT("currentHP"));
const FName GetUnitStatsName(TEXT("GetUnitStats"));
const FName SetHPName(TEXT("SetHP"));
const FName DealDamageName(TEXT("DealDamage"));
constexpr double InvalidVitals = -1.0;

// Reads a Float/Double/Int numeric property from an arbitrary container into OutValue.
bool ReadNumericFromContainer(const FProperty* Property, const void* Container, double& OutValue)
{
	if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		OutValue = DoubleProp->GetPropertyValue_InContainer(Container);
		return true;
	}
	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		OutValue = FloatProp->GetPropertyValue_InContainer(Container);
		return true;
	}
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		OutValue = static_cast<double>(IntProp->GetPropertyValue_InContainer(Container));
		return true;
	}
	if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		OutValue = static_cast<double>(Int64Prop->GetPropertyValue_InContainer(Container));
		return true;
	}
	return false;
}

bool WriteNumericToContainer(const FProperty* Property, void* Container, double Value)
{
	if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		DoubleProp->SetPropertyValue_InContainer(Container, Value);
		return true;
	}
	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		FloatProp->SetPropertyValue_InContainer(Container, static_cast<float>(Value));
		return true;
	}
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		IntProp->SetPropertyValue_InContainer(Container, FMath::RoundToInt(Value));
		return true;
	}
	if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		Int64Prop->SetPropertyValue_InContainer(Container, static_cast<int64>(FMath::RoundToInt(Value)));
		return true;
	}
	return false;
}

bool PropertyNameLooksLikeBattleUI(const FString& Name)
{
	return Name.Contains(TEXT("UI"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Widget"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Dialogue"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Viewport"), ESearchCase::IgnoreCase);
}

void RemoveUserWidgetFromViewport(UObject* Object)
{
	if (UUserWidget* Widget = Cast<UUserWidget>(Object))
	{
		Widget->RemoveFromParent();
	}
}

void HideBattleCameraActor(UObject* Object)
{
	if (AActor* CameraActor = Cast<AActor>(Object))
	{
		CameraActor->SetActorHiddenInGame(true);
		CameraActor->SetActorTickEnabled(false);
	}
}

void ClearObjectPropertyIfPresent(UObject* Container, FProperty* Property)
{
	if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		ObjectProperty->SetObjectPropertyValue_InContainer(Container, nullptr);
	}
}

void TeardownUIPropertiesOnObject(UObject* Container)
{
	if (!Container)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Container->GetClass()); It; ++It)
	{
		FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(*It);
		if (!ObjectProperty)
		{
			continue;
		}

		const FString PropertyName = ObjectProperty->GetName();
		UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Container);
		if (!Value)
		{
			continue;
		}

		if (PropertyNameLooksLikeBattleUI(PropertyName))
		{
			RemoveUserWidgetFromViewport(Value);
			ClearObjectPropertyIfPresent(Container, *It);
			continue;
		}

		if (PropertyName.Contains(TEXT("Camera"), ESearchCase::IgnoreCase)
			&& !PropertyName.Contains(TEXT("Component"), ESearchCase::IgnoreCase))
		{
			HideBattleCameraActor(Value);
		}
	}
}

void SetControllerBattleFlag(AActor* BattleController, const FName PropertyName, const bool bValue)
{
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(BattleController->GetClass()->FindPropertyByName(PropertyName)))
	{
		BoolProperty->SetPropertyValue_InContainer(BattleController, bValue);
	}
}
}

bool UMelodiaJRPGBridgeLibrary::GetUnitArray(AActor* BattleController, const FName ArrayPropertyName, TArray<UObject*>& OutUnits)
{
	OutUnits.Reset();
	if (!BattleController)
	{
		return false;
	}

	if (GetUnitArrayFromObject(BattleController, ArrayPropertyName, OutUnits))
	{
		return true;
	}

	if (UObject* CurrentBattle = GetCurrentBattleObject(BattleController))
	{
		return GetUnitArrayFromObject(CurrentBattle, ArrayPropertyName, OutUnits);
	}

	return false;
}

UObject* UMelodiaJRPGBridgeLibrary::GetCurrentBattleObject(AActor* BattleController)
{
	if (!BattleController)
	{
		return nullptr;
	}

	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(BattleController->GetClass()->FindPropertyByName(CurrentBattleName));
	return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(BattleController) : nullptr;
}

bool UMelodiaJRPGBridgeLibrary::GetUnitArrayFromObject(UObject* SourceObject, const FName ArrayPropertyName, TArray<UObject*>& OutUnits)
{
	if (!SourceObject)
	{
		return false;
	}

	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(SourceObject->GetClass()->FindPropertyByName(ArrayPropertyName));
	if (!ArrayProperty)
	{
		return false;
	}

	const FObjectPropertyBase* InnerObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
	if (!InnerObjectProperty)
	{
		return false;
	}

	FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(SourceObject));
	for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
	{
		if (UObject* Unit = InnerObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index)))
		{
			OutUnits.Add(Unit);
		}
	}

	return OutUnits.Num() > 0;
}

bool UMelodiaJRPGBridgeLibrary::GetNumericProperty(UObject* Object, const FName PropertyName, double& OutValue)
{
	if (!Object)
	{
		return false;
	}

	const FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	return Property && ReadNumericFromContainer(Property, Object, OutValue);
}

bool UMelodiaJRPGBridgeLibrary::SetNumericProperty(UObject* Object, const FName PropertyName, const double Value)
{
	if (!Object)
	{
		return false;
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
	return Property && WriteNumericToContainer(Property, Object, Value);
}

bool UMelodiaJRPGBridgeLibrary::SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
{
	if (!Object)
	{
		return false;
	}

	FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Object->GetClass()->FindPropertyByName(PropertyName));
	if (!ObjectProperty)
	{
		return false;
	}

	ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
	return true;
}

double UMelodiaJRPGBridgeLibrary::GetUnitCurrentHP(UObject* Unit)
{
	double Value = 0.0;
	return GetNumericProperty(Unit, CurrentHPName, Value) ? Value : 0.0;
}

double UMelodiaJRPGBridgeLibrary::GetUnitMaxHP(UObject* Unit)
{
	if (!Unit)
	{
		return 0.0;
	}

	// The template stores maxHP inside the S_UnitStats struct returned by GetUnitStats().
	if (UFunction* StatsFunction = Unit->FindFunction(GetUnitStatsName))
	{
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(StatsFunction->ParmsSize, 1)));
		FMemory::Memzero(Params, StatsFunction->ParmsSize);
		for (TFieldIterator<FProperty> It(StatsFunction); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				It->InitializeValue_InContainer(Params);
			}
		}

		Unit->ProcessEvent(StatsFunction, Params);

		double MaxHP = 0.0;
		bool bFound = false;
		for (TFieldIterator<FProperty> It(StatsFunction); It; ++It)
		{
			FProperty* Param = *It;
			if (!Param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				continue;
			}

			if (const FStructProperty* StructProp = CastField<FStructProperty>(Param))
			{
				const void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Params);
				for (TFieldIterator<FProperty> MemberIt(StructProp->Struct); MemberIt; ++MemberIt)
				{
					if (MemberIt->GetAuthoredName() == TEXT("maxHP"))
					{
						bFound = ReadNumericFromContainer(*MemberIt, StructPtr, MaxHP);
						break;
					}
				}
			}

			if (bFound)
			{
				break;
			}
		}

		for (TFieldIterator<FProperty> It(StatsFunction); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm))
			{
				It->DestroyValue_InContainer(Params);
			}
		}

		if (bFound && MaxHP > 0.0)
		{
			return MaxHP;
		}
	}

	// Fall back to current HP so callers never divide by zero.
	const double CurrentHP = GetUnitCurrentHP(Unit);
	return CurrentHP > 0.0 ? CurrentHP : 1.0;
}

bool UMelodiaJRPGBridgeLibrary::CallUnitNumericFunction(UObject* Unit, const FName FunctionName, const double Value)
{
	if (!Unit)
	{
		return false;
	}

	UFunction* Function = Unit->FindFunction(FunctionName);
	if (!Function)
	{
		return false;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Function->ParmsSize, 1)));
	FMemory::Memzero(Params, Function->ParmsSize);
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Parm))
		{
			It->InitializeValue_InContainer(Params);
		}
	}

	bool bWroteArgument = false;
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		FProperty* Param = *It;
		if (Param->HasAnyPropertyFlags(CPF_Parm) && !Param->HasAnyPropertyFlags(CPF_ReturnParm) && !Param->HasAnyPropertyFlags(CPF_OutParm))
		{
			if (WriteNumericToContainer(Param, Params, Value))
			{
				bWroteArgument = true;
			}
			break;
		}
	}

	if (bWroteArgument)
	{
		Unit->ProcessEvent(Function, Params);
	}

	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Parm))
		{
			It->DestroyValue_InContainer(Params);
		}
	}

	return bWroteArgument;
}

bool UMelodiaJRPGBridgeLibrary::CallControllerDealDamage(AActor* BattleController, UObject* Attacker, UObject* Target, const float Damage)
{
	if (!BattleController || !Attacker || !Target)
	{
		return false;
	}

	UFunction* DealDamageFunction = BattleController->FindFunction(DealDamageName);
	if (!DealDamageFunction)
	{
		return false;
	}

	SetObjectProperty(BattleController, CurrentAttackingUnitName, Attacker);
	SetObjectProperty(BattleController, CurrentTargetUnitName, Target);

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(DealDamageFunction->ParmsSize, 1)));
	FMemory::Memzero(Params, DealDamageFunction->ParmsSize);
	for (TFieldIterator<FProperty> It(DealDamageFunction); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Parm))
		{
			It->InitializeValue_InContainer(Params);
		}
	}

	int32 InputsWritten = 0;
	for (TFieldIterator<FProperty> It(DealDamageFunction); It; ++It)
	{
		FProperty* Param = *It;
		if (!Param->HasAnyPropertyFlags(CPF_Parm) || Param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}

		if (InputsWritten == 0)
		{
			if (WriteNumericToContainer(Param, Params, FMath::Max(0.0f, Damage)))
			{
				++InputsWritten;
			}
			continue;
		}

		if (InputsWritten == 1)
		{
			if (WriteNumericToContainer(Param, Params, 1.0))
			{
				++InputsWritten;
			}
			break;
		}
	}

	const bool bCanCall = InputsWritten >= 2;
	if (bCanCall)
	{
		BattleController->ProcessEvent(DealDamageFunction, Params);
	}

	for (TFieldIterator<FProperty> It(DealDamageFunction); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Parm))
		{
			It->DestroyValue_InContainer(Params);
		}
	}

	return bCanCall;
}

void UMelodiaJRPGBridgeLibrary::SetUnitHP(UObject* Unit, const double NewHP)
{
	if (!Unit)
	{
		return;
	}

	// Write the authoritative value first, then let the template's SetHP event broadcast OnHPSet (refreshes the HP bar).
	SetNumericProperty(Unit, CurrentHPName, NewHP);
	CallUnitNumericFunction(Unit, SetHPName, NewHP);
}

UObject* UMelodiaJRPGBridgeLibrary::GetActiveEnemyUnit(AActor* BattleController)
{
	TArray<UObject*> Enemies;
	if (!GetUnitArray(BattleController, EnemyArrayName, Enemies))
	{
		return nullptr;
	}

	for (UObject* Enemy : Enemies)
	{
		if (GetUnitCurrentHP(Enemy) > 0.0)
		{
			return Enemy;
		}
	}

	return Enemies.Num() > 0 ? Enemies.Last() : nullptr;
}

UObject* UMelodiaJRPGBridgeLibrary::GetActivePartyUnit(AActor* BattleController)
{
	TArray<UObject*> Party;
	if (!GetUnitArray(BattleController, PartyArrayName, Party))
	{
		return nullptr;
	}

	for (UObject* Member : Party)
	{
		if (GetUnitCurrentHP(Member) > 0.0)
		{
			return Member;
		}
	}

	return Party.Num() > 0 ? Party.Last() : nullptr;
}

bool UMelodiaJRPGBridgeLibrary::HasJRPGUnits(AActor* BattleController)
{
	TArray<UObject*> Units;
	return GetUnitArray(BattleController, EnemyArrayName, Units) || GetUnitArray(BattleController, PartyArrayName, Units);
}

FMelodiaJRPGVitals UMelodiaJRPGBridgeLibrary::ReadJRPGVitals(AActor* BattleController)
{
	FMelodiaJRPGVitals Vitals;

	TArray<UObject*> Enemies;
	if (GetUnitArray(BattleController, EnemyArrayName, Enemies))
	{
		Vitals.bHasEnemies = true;
		for (UObject* Enemy : Enemies)
		{
			Vitals.EnemyHP += FMath::Max(0.0, GetUnitCurrentHP(Enemy));
			Vitals.EnemyMaxHP += FMath::Max(0.0, GetUnitMaxHP(Enemy));
		}
	}

	TArray<UObject*> Party;
	if (GetUnitArray(BattleController, PartyArrayName, Party))
	{
		Vitals.bHasParty = true;
		for (UObject* Member : Party)
		{
			Vitals.PartyHP += FMath::Max(0.0, GetUnitCurrentHP(Member));
			Vitals.PartyMaxHP += FMath::Max(0.0, GetUnitMaxHP(Member));
		}
	}

	return Vitals;
}

float UMelodiaJRPGBridgeLibrary::SetActiveEnemyHP(AActor* BattleController, const float NewHP)
{
	UObject* Enemy = GetActiveEnemyUnit(BattleController);
	if (!Enemy)
	{
		return static_cast<float>(InvalidVitals);
	}

	SetUnitHP(Enemy, FMath::Max(0.0f, NewHP));

	TArray<UObject*> Enemies;
	GetUnitArray(BattleController, EnemyArrayName, Enemies);
	double Total = 0.0;
	for (UObject* Unit : Enemies)
	{
		Total += FMath::Max(0.0, GetUnitCurrentHP(Unit));
	}
	return static_cast<float>(Total);
}

float UMelodiaJRPGBridgeLibrary::DamageActiveEnemy(AActor* BattleController, const float Damage)
{
	UObject* Enemy = GetActiveEnemyUnit(BattleController);
	if (!Enemy)
	{
		return static_cast<float>(InvalidVitals);
	}

	if (UObject* PartyMember = GetActivePartyUnit(BattleController))
	{
		if (!CallControllerDealDamage(BattleController, PartyMember, Enemy, Damage))
		{
			const double NewHP = FMath::Max(0.0, GetUnitCurrentHP(Enemy) - FMath::Max(0.0f, Damage));
			SetUnitHP(Enemy, NewHP);
		}
	}
	else
	{
		const double NewHP = FMath::Max(0.0, GetUnitCurrentHP(Enemy) - FMath::Max(0.0f, Damage));
		SetUnitHP(Enemy, NewHP);
	}

	TArray<UObject*> Enemies;
	GetUnitArray(BattleController, EnemyArrayName, Enemies);
	double Total = 0.0;
	for (UObject* Unit : Enemies)
	{
		Total += FMath::Max(0.0, GetUnitCurrentHP(Unit));
	}
	return static_cast<float>(Total);
}

float UMelodiaJRPGBridgeLibrary::DamageParty(AActor* BattleController, const float Damage)
{
	UObject* Member = GetActivePartyUnit(BattleController);
	if (!Member)
	{
		return static_cast<float>(InvalidVitals);
	}

	if (UObject* Enemy = GetActiveEnemyUnit(BattleController))
	{
		if (!CallControllerDealDamage(BattleController, Enemy, Member, Damage))
		{
			const double NewHP = FMath::Max(0.0, GetUnitCurrentHP(Member) - FMath::Max(0.0f, Damage));
			SetUnitHP(Member, NewHP);
		}
	}
	else
	{
		const double NewHP = FMath::Max(0.0, GetUnitCurrentHP(Member) - FMath::Max(0.0f, Damage));
		SetUnitHP(Member, NewHP);
	}

	TArray<UObject*> Party;
	GetUnitArray(BattleController, PartyArrayName, Party);
	double Total = 0.0;
	for (UObject* Unit : Party)
	{
		Total += FMath::Max(0.0, GetUnitCurrentHP(Unit));
	}
	return static_cast<float>(Total);
}

bool UMelodiaJRPGBridgeLibrary::AreEnemiesDefeated(AActor* BattleController)
{
	TArray<UObject*> Enemies;
	if (!GetUnitArray(BattleController, EnemyArrayName, Enemies))
	{
		return false;
	}

	for (UObject* Enemy : Enemies)
	{
		if (GetUnitCurrentHP(Enemy) > 0.0)
		{
			return false;
		}
	}

	return true;
}

bool UMelodiaJRPGBridgeLibrary::IsPartyDefeated(AActor* BattleController)
{
	TArray<UObject*> Party;
	if (!GetUnitArray(BattleController, PartyArrayName, Party))
	{
		return false;
	}

	for (UObject* Member : Party)
	{
		if (GetUnitCurrentHP(Member) > 0.0)
		{
			return false;
		}
	}

	return true;
}

void UMelodiaJRPGBridgeLibrary::RestorePartyVitals(AActor* BattleController)
{
	TArray<UObject*> Party;
	if (!GetUnitArray(BattleController, PartyArrayName, Party))
	{
		return;
	}

	for (UObject* Member : Party)
	{
		SetUnitHP(Member, GetUnitMaxHP(Member));
	}
}

bool UMelodiaJRPGBridgeLibrary::TryFleeBattle(AActor* BattleController)
{
	if (!BattleController)
	{
		return false;
	}

	const FName FleeCandidates[] = {
		TEXT("Flee"),
		TEXT("FleeBattle"),
		TEXT("TryFlee"),
		TEXT("RunAway"),
		TEXT("AttemptFlee"),
		TEXT("EndBattle"),
	};

	for (const FName FunctionName : FleeCandidates)
	{
		if (UFunction* Function = BattleController->FindFunction(FunctionName))
		{
			BattleController->ProcessEvent(Function, nullptr);
			UE_LOG(LogTemp, Log, TEXT("Melodia JRPG bridge invoked flee via %s."), *FunctionName.ToString());
			return true;
		}
	}

	return false;
}

void UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(AActor* BattleController)
{
	if (!BattleController)
	{
		return;
	}

	TeardownUIPropertiesOnObject(BattleController);
	if (UObject* CurrentBattle = GetCurrentBattleObject(BattleController))
	{
		TeardownUIPropertiesOnObject(CurrentBattle);
	}

	const FName TeardownFunctions[] = {
		TEXT("CloseBattleUI"),
		TEXT("HideBattleUI"),
		TEXT("RemoveBattleUI"),
		TEXT("EndBattleUI"),
		TEXT("ReturnToExplore"),
		TEXT("ReturnToField"),
		TEXT("CloseSkillDialogue"),
	};

	for (const FName FunctionName : TeardownFunctions)
	{
		if (UFunction* Function = BattleController->FindFunction(FunctionName))
		{
			BattleController->ProcessEvent(Function, nullptr);
		}
	}

	SetControllerBattleFlag(BattleController, TEXT("isInBattle"), false);
	SetControllerBattleFlag(BattleController, TEXT("bIsInBattle"), false);
	SetControllerBattleFlag(BattleController, TEXT("inBattle"), false);

	UE_LOG(LogTemp, Log, TEXT("Melodia JRPG bridge tore down Phoenix battle UI on %s."), *BattleController->GetName());
}

bool UMelodiaJRPGBridgeLibrary::TrySetUnitDisplayName(UObject* Unit, const FText& DisplayName)
{
	if (!Unit || DisplayName.IsEmpty())
	{
		return false;
	}

	const FName NameCandidates[] = {
		TEXT("unitName"),
		TEXT("UnitName"),
		TEXT("displayName"),
		TEXT("DisplayName"),
		TEXT("characterName"),
		TEXT("CharacterName"),
	};

	for (const FName PropertyName : NameCandidates)
	{
		if (FTextProperty* TextProperty = CastField<FTextProperty>(Unit->GetClass()->FindPropertyByName(PropertyName)))
		{
			TextProperty->SetPropertyValue_InContainer(Unit, DisplayName);
			return true;
		}

		if (FStrProperty* StringProperty = CastField<FStrProperty>(Unit->GetClass()->FindPropertyByName(PropertyName)))
		{
			StringProperty->SetPropertyValue_InContainer(Unit, DisplayName.ToString());
			return true;
		}

		if (FNameProperty* NameProperty = CastField<FNameProperty>(Unit->GetClass()->FindPropertyByName(PropertyName)))
		{
			NameProperty->SetPropertyValue_InContainer(Unit, FName(*DisplayName.ToString()));
			return true;
		}
	}

	return false;
}

bool UMelodiaJRPGBridgeLibrary::SyncPartyUnitsFromSubsystem(UObject* WorldContextObject, AActor* BattleController)
{
	if (!BattleController)
	{
		return false;
	}

	UMelodiaPartySubsystem* Party = UMelodiaPartySubsystem::Get(WorldContextObject ? WorldContextObject : BattleController);
	if (!Party)
	{
		return false;
	}

	TArray<UObject*> PlayerUnits;
	if (!GetUnitArray(BattleController, PartyArrayName, PlayerUnits))
	{
		return false;
	}

	int32 ActiveCount = 0;
	for (int32 Index = 0; Index < PlayerUnits.Num(); ++Index)
	{
		UObject* Unit = PlayerUnits[Index];
		if (!Unit)
		{
			continue;
		}

		const bool bSlotActive = Party->PartySlots.IsValidIndex(Index)
			&& Party->PartySlots[Index].bActiveInBattle
			&& !Party->PartySlots[Index].MemberId.IsNone();

		if (AActor* UnitActor = Cast<AActor>(Unit))
		{
			UnitActor->SetActorHiddenInGame(!bSlotActive);
			UnitActor->SetActorEnableCollision(bSlotActive);
		}

		if (bSlotActive)
		{
			TrySetUnitDisplayName(Unit, Party->PartySlots[Index].DisplayName);
			if (GetUnitCurrentHP(Unit) <= 0.0)
			{
				SetUnitHP(Unit, GetUnitMaxHP(Unit));
			}
			++ActiveCount;
		}
		else
		{
			SetUnitHP(Unit, 0.0);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia JRPG bridge synced %d active party units (%d Phoenix slots)."), ActiveCount, PlayerUnits.Num());
	return ActiveCount > 0;
}
