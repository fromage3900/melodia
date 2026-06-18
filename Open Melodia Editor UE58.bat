@echo off
REM UE 5.8 evaluation — stock Epic install (NOT MooaToon).
REM Edit UE58_ENGINE below to match your Epic Launcher path.

set UE58_ENGINE=C:\Program Files\Epic Games\UE_5.8
set PROJECT=%~dp0Melodia.uproject

if not exist "%UE58_ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" (
    echo UE 5.8 not found at: %UE58_ENGINE%
    echo Install UE 5.8 from Epic Launcher and edit this batch file.
    pause
    exit /b 1
)

start "" "%UE58_ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -Unattended
