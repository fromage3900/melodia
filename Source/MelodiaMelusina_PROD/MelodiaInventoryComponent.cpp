// Lightweight inventory for exploration rewards and quest items.

#include "MelodiaInventoryComponent.h"

UMelodiaInventoryComponent::UMelodiaInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMelodiaInventoryComponent::AddItem(const FName ItemId, const FString& DisplayName, const int32 Quantity, const FLinearColor IconTint)
{
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	for (FMelodiaInventorySlot& Slot : Slots)
	{
		if (Slot.ItemId == ItemId)
		{
			Slot.Quantity += Quantity;
			Slot.DisplayName = DisplayName.IsEmpty() ? Slot.DisplayName : DisplayName;
			TotalItemsCollected += Quantity;
			return true;
		}
	}

	if (Slots.Num() >= FMath::Max(1, MaxSlots))
	{
		return false;
	}

	FMelodiaInventorySlot NewSlot;
	NewSlot.ItemId = ItemId;
	NewSlot.DisplayName = DisplayName;
	NewSlot.Quantity = Quantity;
	NewSlot.IconTint = IconTint;
	Slots.Add(NewSlot);
	TotalItemsCollected += Quantity;
	return true;
}

int32 UMelodiaInventoryComponent::GetItemCount(const FName ItemId) const
{
	for (const FMelodiaInventorySlot& Slot : Slots)
	{
		if (Slot.ItemId == ItemId)
		{
		 return Slot.Quantity;
		}
	}
	return 0;
}

void UMelodiaInventoryComponent::SeedStarterKit()
{
	AddItem(TEXT("ReveriePotion"), TEXT("Reverie Tonic"), 2, FLinearColor(0.42f, 0.92f, 0.72f, 1.0f));
	AddItem(TEXT("SheetMusic"), TEXT("Cracked Sheet"), 1, FLinearColor(0.98f, 0.88f, 0.62f, 1.0f));
}
