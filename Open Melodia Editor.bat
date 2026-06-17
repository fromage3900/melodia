@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\LaunchMelodiaEditor.ps1" -ProjectPath "%~dp0Melodia.uproject"
endlocal
