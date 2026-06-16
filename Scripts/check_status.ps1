$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$sdfPath = "/Game/_PROJECT/04_Materials/SDF"

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

# Check GildedAltar status
Write-Host "=== GildedAltar check ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = "$sdfPath/M_SDF_GildedAltar" }
if ($r -match "Failed to load") {
    Write-Host "  GildedAltar CORRUPTED - rebuilding..." -ForegroundColor Red
    # Delete and rebuild using the same pattern as build_gothic2.ps1
    Call-Mono "delete_asset" @{ asset_path = "$sdfPath/M_SDF_GildedAltar" } | Out-Null
    Start-Sleep -Seconds 3
    Write-Host "  Deleted, waiting for server..." -ForegroundColor Yellow
    Start-Sleep -Seconds 5
} else {
    Write-Host "  GildedAltar OK" -ForegroundColor Green
    try { $g = $r | ConvertFrom-Json; Write-Host "  Nodes: $($g.nodes.Count) Connections: $($g.connections.Count)" } catch {}
}

# Check Sierpinski wiring
Write-Host "`n=== Sierpinski wiring check ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = "$sdfPath/M_SDF_SierpinskiTetrahedron" }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count) Custom: $($g.custom_hlsl_nodes.Count) Connections: $($g.connections.Count)" -ForegroundColor Green
    if ($g.connections.Count -gt 0) {
        foreach ($c in $g.connections) {
            Write-Host "    $($c.from) -> $($c.to) $($c.to_pin)"
        }
    } else {
        Write-Host "  NO CONNECTIONS - needs rewiring!" -ForegroundColor Red
    }
} catch { Write-Host "  Parse error" -ForegroundColor Red }
