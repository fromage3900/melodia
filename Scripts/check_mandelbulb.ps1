$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fp = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 120
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

Write-Host "=== Investigating M_SDF_Mandelbulb_Master ===" -ForegroundColor Cyan
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
Write-Host "Stats: $r" -ForegroundColor Yellow
Write-Host ""
$r = Call-Mono "get_all_expressions" @{ asset_path = $fp }
Write-Host "Expressions: $r" -ForegroundColor Yellow
Write-Host ""
$r = Call-Mono "validate_material" @{ asset_path = $fp }
Write-Host "Validation: $r" -ForegroundColor Yellow
