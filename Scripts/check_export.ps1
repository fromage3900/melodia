$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fullPath = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 120
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 10 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

Write-Host "=== Checking Mandelbulb via export_material_graph ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = $fullPath }
Write-Host "Result length: $($r.Length)" -ForegroundColor Yellow
# Just show first 2000 chars
Write-Host $r.Substring(0, [Math]::Min(2000, $r.Length))
