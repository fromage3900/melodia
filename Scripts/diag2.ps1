$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"

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

# Full export dump for M2
Write-Host "=== M2 Raw Export ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_TrebleClef_Ornament" }
Write-Host $r

# Full compilation stats for M1
Write-Host "`n=== M1 Raw Compilation Stats ===" -ForegroundColor Cyan
$r = Call-Mono "get_compilation_stats" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_SheetMusic_Score" }
Write-Host $r

# Try a known working material to verify API
Write-Host "`n=== Verify API with known good material ===" -ForegroundColor Cyan
$r = Call-Mono "get_all_expressions" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FractalOrnament" }
Write-Host ($r.Substring(0, [Math]::Min(500, $r.Length)))
