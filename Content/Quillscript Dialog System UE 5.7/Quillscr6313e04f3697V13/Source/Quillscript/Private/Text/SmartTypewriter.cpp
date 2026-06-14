// Copyright Bruno Caxito. All Rights Reserved.

#include "Text/SmartTypewriter.h"

#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Text/SmartTextBlockDecorator.h"
#include "Utils/Lexer.h"
#include "Utils/Tools.h"


void ASmartTypewriter::Initialize(const FText& InText,
							const FTextPrintedDelegate& InPrintedDelegate,
							const FTextCompletedDelegate& InCompletedDelegate,
							const float InInterval,
							USoundBase* InSound,
							const bool bInAudioOverlap,
							const bool bInSanitize)
{
	if (this->GetWorld())
	{
		// Clear the timer.
		this->ClearTimer();

		// Reset variables.
		this->Index = 1;
		this->bAddTemporaryClosingTag = false;

		// Initialize variables.
		this->Text = this->ReplaceDelayTags(InText);
		this->PrintedDelegate = InPrintedDelegate;
		this->CompletedDelegate = InCompletedDelegate;
		this->Interval = InInterval;
		this->Sound = InSound;
		this->bOverlapSound = bInAudioOverlap;
		this->bSanitize = bInSanitize;

		// Start typing.
		this->TypeText();
	}
}

void ASmartTypewriter::TypeText()
{
	// Stop if the text is printed.
	if (this->Index > this->Text.ToString().Len())
		return;

	// Instant print if the interval is zero.
	if (this->Interval <= 0)
	{
		this->Index = this->Text.ToString().Len();
		this->GetPartialText();
		return;
	}

	// Set the timer function to type the text.
	this->GetWorld()->GetTimerManager().SetTimer(
		this->TimerHandle,

		[this]()
		{
			this->GetPartialText();
		},

		this->Interval,
		true,
		0
	);
}

void ASmartTypewriter::Finish()
{
	this->Index = this->Text.ToString().Len();
	this->bAddTemporaryClosingTag = false;
	this->Unpause();
}

void ASmartTypewriter::ChangeSpeed(const float NewInterval)
{
	this->ClearTimer();
	this->Interval = NewInterval;
	this->TypeText();
}

void ASmartTypewriter::Pause(const float Duration) const
{
	if (IsValid(this) && IsValid(this->GetWorld()))
	{
		if (auto& TimerManager{ this->GetWorld()->GetTimerManager() }; TimerManager.TimerExists(this->TimerHandle))
		{
			TimerManager.PauseTimer(this->TimerHandle);

			if (Duration > 0)
			{
				FTimerHandle PauseDurationTimerHandle;

				TimerManager.SetTimer(
					PauseDurationTimerHandle,

					[this]()
					{
						this->Unpause();
					},

					Duration,
					false
				);
			}
		}
	}
}

void ASmartTypewriter::Unpause() const
{
	if (IsValid(this) && IsValid(this->GetWorld()))
		if (auto& TimerManager{ this->GetWorld()->GetTimerManager() }; TimerManager.TimerExists(this->TimerHandle) && TimerManager.IsTimerPaused(this->TimerHandle))
			TimerManager.UnPauseTimer(this->TimerHandle);
}

void ASmartTypewriter::ClearTimer()
{
	if (IsValid(this) && IsValid(this->GetWorld()))
	{
		this->GetWorld()->GetTimerManager().ClearTimer(this->TimerHandle);
		this->TimerHandle = FTimerHandle();
	}
}

void ASmartTypewriter::AddSubstringDelay(const FString& Substring, const float Delay)
{
	this->SubstringDelays.Add(Substring, Delay);
}

void ASmartTypewriter::AddSubstringsDelays(const TMap<FString, float>& Substrings)
{
	this->SubstringDelays.Append(Substrings);
}

void ASmartTypewriter::RemoveSubstringDelay(const FString& Substring)
{
	this->SubstringDelays.Remove(Substring);
}

void ASmartTypewriter::ChangeSound(USoundBase* NewSound, const bool OverlapSound)
{
	this->Sound = NewSound;
	this->bOverlapSound = OverlapSound;

	if (IsValid(this->AudioComponent))
	{
		this->AudioComponent->Stop();
		this->AudioComponent->SetSound(this->Sound);
	}
}

bool ASmartTypewriter::IsPaused() const
{
	if (IsValid(this) && IsValid(this->GetWorld()))
		if (const auto& TimerManager{ this->GetWorld()->GetTimerManager() }; TimerManager.TimerExists(this->TimerHandle))
			return TimerManager.IsTimerPaused(this->TimerHandle);

	return false;
}

bool ASmartTypewriter::IsPrintCompleted() const
{
	return this->Index > this->Text.ToString().Len();
}

void ASmartTypewriter::GetPartialText()
{
	FString TextAsString{ Text.ToString() };

	if (bSanitize)
		TextAsString.TrimStartAndEndInline();

	// Print letter by letter. (typewrite)
	if (const int32 TextLength{ TextAsString.Len() }; this->Index <= TextLength)
	{
		const FString StartingSubstring{ TextAsString.Left(this->Index) };
		const FString EndingSubstring{ TextAsString.RightChop(this->Index) };

		// It's opening a 'Rich Text Tag'.
		if (StartingSubstring.EndsWith("<"))
		{
			// It's a closing tag.
			if (EndingSubstring.StartsWith("/>"))
			{
				this->Index += 2;
				this->bAddTemporaryClosingTag = false;
			}

			// It's an opening tag.
			else
			{
				// Find the tag closing character.
				FString TagNameAndAttributes{ "<" };

				for (; this->Index < TextLength; this->Index += 1)
				{
					const FString TagNameCurrentChar{ TextAsString.Mid(this->Index, 1) };
					const FString TagNamePreviousChar{ this->Index > 0 ? TextAsString.Mid(this->Index - 1, 1) : "" };
					TagNameAndAttributes += TagNameCurrentChar;

					if (TagNameCurrentChar == ">" && TagNamePreviousChar != FLexer::EscapeCharacter)
					{
						// Pause for the given time if this is a delay tag.
						if (TagNameAndAttributes.StartsWith("<" + USmartTextBlockDecorator::DelayTagName))
							this->Pause(this->ParseDelayTag(TagNameAndAttributes));

						this->Index += 1;
						this->bAddTemporaryClosingTag = true;
						break;
					}
				}
			}
		}

		// Get string to show.
		FString PrintString{ TextAsString.Left(this->Index) };

		// Pause for the given time if this is a substring delay, but it's not the last character.
		if (this->Index < TextLength)
		{
			for (const auto& SubstringDelay : this->SubstringDelays)
			{
				if (PrintString.EndsWith(SubstringDelay.Key))
				{
					this->Pause(SubstringDelay.Value);
					break;
				}
			}
		}

		// Add a temporary 'Rich Text Close Tag'.
		if (this->bAddTemporaryClosingTag)
			PrintString += "</>";

		// Add a letter to the text block.
		if (PrintedDelegate.ExecuteIfBound(FText::FromString(PrintString), PrintString.Right(1), this->Index)){ /* ~ */ }
		this->Index += 1;

		// Play the typing sound.
		if (IsValid(this->Sound))
		{
			if (this->bOverlapSound)
				this->AudioComponent = UGameplayStatics::CreateSound2D(this, this->Sound);

			if (!this->AudioComponent)
				this->AudioComponent = UGameplayStatics::CreateSound2D(this, this->Sound);

			this->AudioComponent->SetSound(this->Sound);
			this->AudioComponent->Play();
		}
	}

	// Print completed.
	else
	{
		// Stop this loop.
		this->GetWorld()->GetTimerManager().ClearTimer(this->TimerHandle);

		// Call the On Completed Delegate.
		if (CompletedDelegate.ExecuteIfBound()){ /* ~ */ }
	}
}

FText ASmartTypewriter::ReplaceDelayTags(const FText& InText)
{
	const FString TokenOpen{ "__TKN_DELAY_OPEN__" };
	const FString TokenClose{ "__TKN_DELAY_CLOSE__" };

	FString TextAsString{ InText.ToString() };

	// Replace escaped symbols with tokens.
	TextAsString.ReplaceInline(*( FLexer::EscapeCharacter + USmartTextBlockDecorator::DelayShortTagOpen ), *TokenOpen);
	TextAsString.ReplaceInline(*( FLexer::EscapeCharacter + USmartTextBlockDecorator::DelayShortTagClose ), *TokenClose);

	// Replace symbols for delay tag.
	TextAsString.ReplaceInline(*USmartTextBlockDecorator::DelayShortTagOpen, *("<" + USmartTextBlockDecorator::DelayTagName + " value=\""));
	TextAsString.ReplaceInline(*USmartTextBlockDecorator::DelayShortTagClose, TEXT("\"></>"));

	// Replace tokens back.
	TextAsString.ReplaceInline(*TokenOpen, *USmartTextBlockDecorator::DelayShortTagOpen);
	TextAsString.ReplaceInline(*TokenClose, *USmartTextBlockDecorator::DelayShortTagClose);

	return TXT(TextAsString);
}

float ASmartTypewriter::ParseDelayTag(FString TagNameAndAttributes)
{
	// Remove the tag name.
	TagNameAndAttributes.RemoveFromStart("<" + USmartTextBlockDecorator::DelayTagName);
	TagNameAndAttributes.RemoveFromEnd(">");
	TagNameAndAttributes.ReplaceInline(TEXT("value="), TEXT(""));
	TagNameAndAttributes.ReplaceInline(TEXT("\""), TEXT(""));
	TagNameAndAttributes.TrimStartAndEndInline();

	// Convert what left to a float.
	return USmartTextBlockDecorator::ConvertDelayToSeconds(*TagNameAndAttributes);
}