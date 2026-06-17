# Single-instance Melodia editor launcher (bypasses Epic VersionSelector double-open issues).
param(
    [string]$ProjectPath = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$LocalPathsFile = Join-Path $ScriptDir "local-paths.ps1"
if (Test-Path $LocalPathsFile) {
    . $LocalPathsFile
    Write-Host "Loaded local paths from $LocalPathsFile"
}

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    if ($MelodiaProjectPath) {
        $ProjectPath = $MelodiaProjectPath
    }
    else {
        $ProjectPath = (Join-Path (Split-Path -Parent $ScriptDir) "Melodia.uproject")
    }
}

if ($MelodiaEngineExe) {
    $EngineExe = $MelodiaEngineExe
}
else {
    $EngineExe = "G:\MooaToon-main\MooaToon-main\MooaToon-Engine-Precompiled\Windows\Engine\Binaries\Win64\UnrealEditor.exe"
}

$MutexName = "Global\MelodiaEditorSingleInstance_9c5d3a24"
$LaunchGuardSeconds = 90

if (-not (Test-Path $EngineExe)) {
    Write-Error "MooaToon UnrealEditor not found: $EngineExe`nCopy Scripts/local-paths.ps1.example to Scripts/local-paths.ps1 and set MelodiaEngineExe."
    exit 1
}

if (-not (Test-Path $ProjectPath)) {
    Write-Error "Project not found: $ProjectPath"
    exit 1
}

# Block rapid double-clicks / duplicate launch attempts while startup is in progress.
$mutex = New-Object System.Threading.Mutex($false, $MutexName)
$ownsMutex = $false
try {
    $ownsMutex = $mutex.WaitOne(0)
    if (-not $ownsMutex) {
        Write-Host "Melodia Editor is already starting. Wait for the existing window or run: taskkill /F /IM UnrealEditor.exe"
        exit 0
    }

    # If an editor for this project is already running, do not spawn another.
    $existing = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -and ($_.CommandLine -match [regex]::Escape($ProjectPath) -or $_.CommandLine -match 'Melodia\\Melodia\.uproject') }
    if ($existing) {
        Write-Host "Melodia Editor is already running (PID $($existing[0].ProcessId))."
        exit 0
    }

    $iniOverrides = @(
        '-ini:EditorPerProjectUserSettings:[/Script/LiveCoding.LiveCodingSettings]:bEnabled=false'
        '-ini:EditorPerProjectUserSettings:[/Script/LiveCoding.LiveCodingSettings]:Startup=Manual'
        '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorLoadingSavingSettings]:bForceCompilationAtStartup=false'
        '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorLoadingSavingSettings]:RestoreOpenAssetTabsOnRestart=False'
        '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorLoadingSavingSettings]:bDetectChangesOnStartup=False'
    )

    $argumentList = @(
        "`"$ProjectPath`""
        '-Unattended'
    ) + $iniOverrides

    Write-Host "Launching Melodia (single instance, Live Coding off, startup modals suppressed)..."
    Start-Process -FilePath $EngineExe -ArgumentList $argumentList | Out-Null

    # Hold the mutex through the fragile startup window.
    Start-Sleep -Seconds $LaunchGuardSeconds
}
finally {
    if ($ownsMutex) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}
