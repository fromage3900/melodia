#include "MelodiaGlideComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MelodiaRhythmHUDWidget.h"

UMelodiaGlideComponent::UMelodiaGlideComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UCharacterMovementComponent* UMelodiaGlideComponent::ResolveMovement() const
{
	if (const ACharacter* Character = ResolveCharacter())
	{
		return Character->GetCharacterMovement();
	}
	return nullptr;
}

ACharacter* UMelodiaGlideComponent::ResolveCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UMelodiaGlideComponent::ApplyMovementDefaults()
{
	UCharacterMovementComponent* Movement = ResolveMovement();
	if (!Movement)
	{
		return;
	}

	Movement->JumpZVelocity = JumpZVelocity;
	Movement->MaxWalkSpeed = FMath::Max(Movement->MaxWalkSpeed, 450.0f);
	Movement->AirControl = DefaultAirControl;
	DefaultGravityScale = Movement->GravityScale;
	DefaultAirControl = Movement->AirControl;
	bDefaultsCaptured = true;
	GlideStamina = 1.0f;
}

void UMelodiaGlideComponent::NotifyJumpPressed()
{
	bJumpHeld = true;
}

void UMelodiaGlideComponent::NotifyJumpReleased()
{
	bJumpHeld = false;
}

void UMelodiaGlideComponent::EndGlide(UCharacterMovementComponent* Movement)
{
	if (!Movement)
	{
		return;
	}

	if (bIsGliding && bDefaultsCaptured)
	{
		Movement->GravityScale = DefaultGravityScale;
		Movement->AirControl = DefaultAirControl;
	}

	bIsGliding = false;
}

void UMelodiaGlideComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = ResolveCharacter();
	UCharacterMovementComponent* Movement = ResolveMovement();
	if (!Character || !Movement)
	{
		return;
	}

	if (!bDefaultsCaptured)
	{
		ApplyMovementDefaults();
	}

	if (Movement->IsMovingOnGround())
	{
		AirborneTime = 0.0f;
		EndGlide(Movement);
		GlideStamina = FMath::Clamp(GlideStamina + GlideStaminaRegenPerSecond * DeltaTime, 0.0f, 1.0f);
		return;
	}

	AirborneTime += DeltaTime;

	const bool bCanStartGlide = AirborneTime >= MinAirTimeBeforeGlide && GlideStamina > 0.01f;
	const bool bWantGlide = bJumpHeld && bCanStartGlide;

	if (bWantGlide)
	{
		if (!bIsGliding)
		{
			bIsGliding = true;
			GlideTimeRemaining = MaxGlideDuration;
		}

		Movement->GravityScale = GlideGravityScale;
		Movement->AirControl = GlideAirControl;

		const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
		Character->AddMovementInput(Forward, GlideForwardAccel * DeltaTime * 0.01f);

		GlideTimeRemaining -= DeltaTime;
		GlideStamina = FMath::Max(0.0f, GlideStamina - (DeltaTime / FMath::Max(MaxGlideDuration, 0.1f)));

		if (GlideTimeRemaining <= 0.0f || GlideStamina <= 0.01f)
		{
			EndGlide(Movement);
		}
	}
	else
	{
		EndGlide(Movement);
		if (!bJumpHeld)
		{
			GlideStamina = FMath::Clamp(GlideStamina + GlideStaminaRegenPerSecond * 0.35f * DeltaTime, 0.0f, 1.0f);
		}
	}
}
