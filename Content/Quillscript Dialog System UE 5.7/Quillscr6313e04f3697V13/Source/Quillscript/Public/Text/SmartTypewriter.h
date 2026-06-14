// Copyright Bruno Caxito. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "SmartTypewriter.generated.h"

class UAudioComponent;

#pragma region Delegates

DECLARE_DYNAMIC_DELEGATE(FTextCompletedDelegate);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FTextPrintedDelegate, const FText&, Text, const FString&, Character, const int32&, Index);

#pragma endregion


/**
 * Handles the typing effect for text.
 */
UCLASS(BlueprintType)
class QUILLSCRIPT_API ASmartTypewriter final : public AInfo
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter")
	void Initialize(const FText& InText,
		const FTextPrintedDelegate& InPrintedDelegate,
		const FTextCompletedDelegate& InCompletedDelegate,
		const float InInterval, USoundBase* InSound,
		const bool bInAudioOverlap,
		const bool bInSanitize);


	/// Timer Controls
	#pragma region Timer

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void TypeText();

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void Finish();

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void ChangeSpeed(const float NewInterval = 0.02);

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls", meta = ( AdvancedDisplay = "Duration" ))
	void Pause(const float Duration = -1) const;

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void Unpause() const;

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void ClearTimer();

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void AddSubstringDelay(const FString& Substring, const float Delay = 0.5);

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void AddSubstringsDelays(const TMap<FString, float>& Substrings);

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	void RemoveSubstringDelay(const FString& Substring);

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls", meta = ( AdvancedDisplay = "OverlapSound" ))
	void ChangeSound(USoundBase* NewSound = nullptr, const bool OverlapSound = false);

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	bool IsPaused() const;

	UFUNCTION(BlueprintCallable, Category = "Quillscript|Typewriter|Timer Controls")
	bool IsPrintCompleted() const;

	#pragma endregion Timer


	/// Getters and Setters
	#pragma region GetSet

	UFUNCTION(BlueprintGetter)
	FORCEINLINE int32 GetIndex() const { return this->Index; }

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FTimerHandle& GetTimerHandle() { return this->TimerHandle; }

	#pragma endregion GetSet


	/// Events
	#pragma region Events

	UPROPERTY(BlueprintReadOnly, Category = "Quillscript|Typewriter|Events")
	FTextPrintedDelegate OnPrinted;

	UPROPERTY(BlueprintReadOnly, Category = "Quillscript|Typewriter|Events")
	FTextCompletedDelegate OnCompleted;

	#pragma endregion Events


private:
	/// Properties
	#pragma region Properties

	FText Text;
	FTextPrintedDelegate PrintedDelegate;
	FTextCompletedDelegate CompletedDelegate;
	float Interval{ 0.02 };
	bool bOverlapSound{ true };
	bool bSanitize{ true };

	// Sound played when typing.
	UPROPERTY()
	TObjectPtr<USoundBase> Sound{ nullptr };

	// The current position where the text is sliced.
	UPROPERTY(BlueprintGetter = GetIndex, Category = "Quillscript|Typewriter|Properties")
	int32 Index{ 1 };

	// Maps the substring to the delay time.
	TMap<FString, float> SubstringDelays;

	#pragma endregion Properties


	/// References
	#pragma region References

	UPROPERTY(BlueprintGetter = GetTimerHandle, Category = "Quillscript|Typewriter|References")
	FTimerHandle TimerHandle;

	#pragma endregion References


	/// Internal Use
	#pragma region Internal

	// If true, it will add a temporary closing tag to the text.
	bool bAddTemporaryClosingTag{ false };

	// Audio component used to play the typing sound.
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent{ nullptr };

	// Split the text into parts at the given index.
	void GetPartialText();

	// Replace delay shot tags to the real rich text tags.
	static FText ReplaceDelayTags(const FText& InText);

	// Parse the delay tag and return the delay time.
	static float ParseDelayTag(FString TagNameAndAttributes);

	#pragma endregion Internal
};