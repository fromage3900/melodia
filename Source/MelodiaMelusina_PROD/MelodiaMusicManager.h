// Quartz-backed rhythm clock actor for the Melodia combat loop.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaMusicManager.generated.h"

class UAudioComponent;
class UQuartzClockHandle;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMelodiaBeatEvent, int32, BeatIndex);

UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaMusicManager : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaMusicManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category="Melodia|Music")
	FMelodiaBeatEvent OnBeatTickEvent;

	UPROPERTY(BlueprintAssignable, Category="Melodia|Music")
	FMelodiaBeatEvent OnDownbeatEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Music")
	FName ClockName = TEXT("MelodiaBattleClock");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Music", meta=(ClampMin="1.0"))
	float CurrentBPM = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Music")
	bool bAutoStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melodia|Music")
	TObjectPtr<USoundBase> DefaultSoundTrack;

	UFUNCTION(BlueprintCallable, Category="Melodia|Music")
	void PlayMusic(USoundBase* SoundTrack, float InBPM);

	UFUNCTION(BlueprintCallable, Category="Melodia|Music")
	void StartBattleMusic(USoundBase* SoundTrack, float InBPM);

	UFUNCTION(BlueprintCallable, Category="Melodia|Music")
	void StopMusic();

	UFUNCTION(BlueprintPure, Category="Melodia|Music")
	float GetCurrentBeatPosition() const;

	UFUNCTION(BlueprintPure, Category="Melodia|Music")
	float GetBeatLength() const;

	UFUNCTION(BlueprintPure, Category="Melodia|Music")
	float GetBeatPhase() const;

	UFUNCTION(BlueprintPure, Category="Melodia|Music")
	float GetTimeToNextBeatMs() const;

	UFUNCTION(BlueprintPure, Category="Melodia|Music")
	bool IsQuartzClockActive() const;

	UFUNCTION(BlueprintImplementableEvent, Category="Melodia|Music")
	void OnBeatTick(int32 BeatIndex);

	UFUNCTION(BlueprintImplementableEvent, Category="Melodia|Music")
	void OnDownbeat(int32 BeatIndex);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melodia|Music")
	TObjectPtr<UAudioComponent> MusicAudioComponent;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Music")
	TObjectPtr<UQuartzClockHandle> ClockHandle;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Music")
	float PlaybackStartTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Music")
	int32 BeatCounter = -1;

	UPROPERTY(BlueprintReadOnly, Category="Melodia|Music")
	bool bUsingQuartz = false;

	void StartQuartzClock(float InBPM);
	void BroadcastBeatIfNeeded();
};
