// Jump + hold-to-glide exploration locomotion (portfolio locomotion showcase).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MelodiaGlideComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS(Blueprintable, ClassGroup=(Melodia), meta=(BlueprintSpawnableComponent))
class MELODIAMELUSINA_PROD_API UMelodiaGlideComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMelodiaGlideComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float JumpZVelocity = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float GlideGravityScale = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float GlideAirControl = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float GlideForwardAccel = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float MinAirTimeBeforeGlide = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float MaxGlideDuration = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Glide")
	float GlideStaminaRegenPerSecond = 0.65f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Glide")
	bool bIsGliding = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Glide")
	bool bJumpHeld = false;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Glide")
	float GlideStamina = 1.0f;

	UFUNCTION(BlueprintCallable, Category="Melodia|Glide")
	void ApplyMovementDefaults();

	UFUNCTION(BlueprintCallable, Category="Melodia|Glide")
	void NotifyJumpPressed();

	UFUNCTION(BlueprintCallable, Category="Melodia|Glide")
	void NotifyJumpReleased();

private:
	float AirborneTime = 0.0f;
	float DefaultGravityScale = 1.0f;
	float DefaultAirControl = 0.25f;
	float GlideTimeRemaining = 0.0f;
	bool bDefaultsCaptured = false;

	UCharacterMovementComponent* ResolveMovement() const;
	ACharacter* ResolveCharacter() const;
	void EndGlide(UCharacterMovementComponent* Movement);
};
