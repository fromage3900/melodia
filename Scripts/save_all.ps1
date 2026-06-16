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

# Save all 10 materials to ensure graph data persists
$materials = @(
    "M_SDF_Mandelbulb_Master",
    "M_SDF_JuliaSet_Quaternion",
    "M_SDF_SierpinskiTetrahedron",
    "M_SDF_FractalOrnament",
    "M_SDF_MengerSponge",
    "M_SDF_GothicRoseWindow",
    "M_SDF_CathedralVault",
    "M_SDF_BaroqueColumn",
    "M_SDF_FlyingButtress",
    "M_SDF_GildedAltar"
)

Write-Host "=== Saving all materials ===" -ForegroundColor Cyan
foreach ($mat in $materials) {
    $fp = "$sdfPath/$mat"
    $r = Call-Mono "save_material" @{ asset_path = $fp }
    Write-Host "  $mat : $r" -ForegroundColor $(if ($r -match "saved|success|true" -or $r -match "\{") { "Green" } else { "Yellow" })
    Start-Sleep -Seconds 1
}

# Now re-check Sierpinski
Write-Host "`n=== Post-save check: Sierpinski ===" -ForegroundColor Cyan
Start-Sleep -Seconds 3
$r = Call-Mono "export_material_graph" @{ asset_path = "$sdfPath/M_SDF_SierpinskiTetrahedron" }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count) Custom: $($g.custom_hlsl_nodes.Count) Connections: $($g.connections.Count)" -ForegroundColor $(if ($g.connections.Count -gt 0) { "Green" } else { "Red" })
} catch { Write-Host "  Parse error" -ForegroundColor Red }
