# Sync production melodia -> ue5.8-eval fork (run from either clone root).
param(
    [ValidateSet("pull-production", "push-eval")]
    [string]$Direction = "pull-production"
)

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $root

if (-not (Test-Path ".git")) {
    Write-Error "Not a git repository: $root"
}

function Ensure-Remote($name, $url) {
    $existing = git remote get-url $name 2>$null
    if (-not $existing) {
        git remote add $name $url
        Write-Host "Added remote $name -> $url"
    }
}

Ensure-Remote "production" "https://github.com/fromage3900/melodia.git"
Ensure-Remote "ue58" "https://github.com/fromage3900/melodia-ue58.git"

$branch = git branch --show-current
if ($branch -ne "ue5.8-eval") {
    Write-Warning "Current branch is '$branch' (expected ue5.8-eval). Continue? Ctrl+C to abort."
    Start-Sleep -Seconds 3
}

if ($Direction -eq "pull-production") {
    git fetch production
    git merge production/master -m "sync: merge production master into ue5.8-eval"
    Write-Host "Merged production/master. Resolve conflicts, rebuild on UE 5.8, then push to ue58."
}
else {
    git push -u ue58 ue5.8-eval:main
    git push -u origin ue5.8-eval
    Write-Host "Pushed ue5.8-eval to origin and ue58 (main)."
}
