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
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

# Verify remaining materials individually with longer waits
$remaining = @("M_SDF_BaroqueColumn", "M_SDF_FlyingButtress", "M_SDF_GildedAltar")

foreach ($mat in $remaining) {
    $fp = "$sdfPath/$mat"
    Write-Host "`n--- $mat ---" -ForegroundColor Yellow
    Call-Mono "recompile_material" @{ asset_path = $fp } | Out-Null
    Start-Sleep -Seconds 8
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
    try {
        $s = $r | ConvertFrom-Json
        if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
        else { Write-Host "  FAILED" -ForegroundColor Red }
    } catch { Write-Host "  Parse error: $r" -ForegroundColor Yellow }
}

# Also spot-check one 84PS material with export_material_graph to verify wiring
Write-Host "`n--- Spot check: M_SDF_SierpinskiTetrahedron ---" -ForegroundColor Yellow
$r = Call-Mono "export_material_graph" @{ asset_path = "$sdfPath/M_SDF_SierpinskiTetrahedron" }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count), Custom HLSL: $($g.custom_hlsl_nodes.Count), Connections: $($g.connections.Count)" -ForegroundColor Cyan
    if ($g.connections) {
        foreach ($c in $g.connections) { Write-Host "    $($c.from) -> $($c.to) ($($c.to_pin))" }
    }
} catch { Write-Host "  Parse error" -ForegroundColor Red }
