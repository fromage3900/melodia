// Quartz-backed rhythm clock actor for the Melodia combat loop.

#include "MelodiaMusicManager.h"

#include "Components/AudioComponent.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Quartz/QuartzSubsystem.h"
#include "Sound/QuartzQuantizationUtilities.h"

AMelodiaMusicManager::AMelodiaMusicManager()
{
	PrimaryActorTick.bCanEverTick = true;

	MusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicAudioComponent"));
	RootComponent = MusicAudioComponent;
	MusicAudioComponent->bAutoActivate = false;
}

void AMelodiaMusicManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		PlayMusic(DefaultSoundTrack, CurrentBPM);
	}
}

void AMelodiaMusicManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BroadcastBeatIfNeeded();
}

void AMelodiaMusicManager::PlayMusic(USoundBase* SoundTrack, const float InBPM)
{
	CurrentBPM = FMath::Max(1.0f, InBPM);
	PlaybackStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	BeatCounter = -1;

	if (SoundTrack && MusicAudioComponent)
	{
		MusicAudioComponent->SetSound(SoundTrack);
		MusicAudioComponent->Play();
	}

	StartQuartzClock(CurrentBPM);
}

void AMelodiaMusicManager::StartBattleMusic(USoundBase* SoundTrack, const float InBPM)
{
	PlayMusic(SoundTrack, InBPM);
}

void AMelodiaMusicManager::StopMusic()
{
	if (MusicAudioComponent)
	{
		MusicAudioComponent->Stop();
	}

	if (ClockHandle)
	{
		UQuartzClockHandle* LocalHandle = ClockHandle;
		ClockHandle->StopClock(this, true, LocalHandle);
		ClockHandle = LocalHandle;
	}

	bUsingQuartz = false;
	BeatCounter = -1;
}

float AMelodiaMusicManager::GetCurrentBeatPosition() const
{
	const float RuntimeSeconds = IsQuartzClockActive()
		? ClockHandle->GetEstimatedRunTime(this)
		: (GetWorld() ? GetWorld()->GetTimeSeconds() - PlaybackStartTime : 0.0f);

	return RuntimeSeconds / GetBeatLength();
}

float AMelodiaMusicManager::GetBeatLength() const
{
	return 60.0f / FMath::Max(1.0f, CurrentBPM);
}

float AMelodiaMusicManager::GetBeatPhase() const
{
	const float BeatPosition = GetCurrentBeatPosition();
	const float Fraction = FMath::Frac(BeatPosition);
	return Fraction < 0.0f ? Fraction + 1.0f : Fraction;
}

float AMelodiaMusicManager::GetTimeToNextBeatMs() const
{
	return (1.0f - GetBeatPhase()) * GetBeatLength() * 1000.0f;
}

bool AMelodiaMusicManager::IsQuartzClockActive() const
{
	return ClockHandle && bUsingQuartz && ClockHandle->IsClockRunning(this);
}

void AMelodiaMusicManager::StartQuartzClock(const float InBPM)
{
	bUsingQuartz = false;

	UWorld* World = GetWorld();
	UQuartzSubsystem* QuartzSubsystem = World ? UQuartzSubsystem::Get(World) : nullptr;
	if (!QuartzSubsystem || !QuartzSubsystem->IsQuartzEnabled())
	{
		return;
	}

	FQuartzClockSettings Settings;
	ClockHandle = QuartzSubsystem->CreateNewClock(this, ClockName, Settings, true, true);
	if (!ClockHandle)
	{
		return;
	}

	FQuartzQuantizationBoundary Boundary;
	Boundary.Quantization = EQuartzCommandQuantization::Beat;

	FOnQuartzCommandEventBP EmptyDelegate;
	UQuartzClockHandle* LocalHandle = ClockHandle;
	ClockHandle->SetBeatsPerMinute(this, Boundary, EmptyDelegate, LocalHandle, InBPM);
	ClockHandle = LocalHandle;

	LocalHandle = ClockHandle;
	ClockHandle->StartClock(this, LocalHandle);
	ClockHandle = LocalHandle;

	// Quartz clock commands are evaluated by the audio engine, so an immediate
	// IsClockRunning check can be false even when the start command is valid.
	bUsingQuartz = ClockHandle != nullptr;
}

void AMelodiaMusicManager::BroadcastBeatIfNeeded()
{
	const int32 CurrentBeat = FMath::FloorToInt(GetCurrentBeatPosition());
	if (CurrentBeat <= BeatCounter)
	{
		return;
	}

	BeatCounter = CurrentBeat;
	OnBeatTickEvent.Broadcast(BeatCounter);
	OnBeatTick(BeatCounter);

	if (BeatCounter % 4 == 0)
	{
		OnDownbeatEvent.Broadcast(BeatCounter);
		OnDownbeat(BeatCounter);
	}
}
