$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fp = "/Game/_PROJECT/04_Materials/SDF/M_SDF_SierpinskiTetrahedron"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 90
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "  NET_ERROR" -ForegroundColor Red; return $null }
}

Write-Host "=== Fixing Sierpinski Wiring ===" -ForegroundColor Cyan

# Wire each param to specific pin using direct expression names
$wiring = @(
    @{ from = "MaterialExpressionTextureCoordinate_1"; pin = "UV" },
    @{ from = "MaterialExpressionTime_1"; pin = "Time" },
    @{ from = "MaterialExpressionScalarParameter_4"; pin = "IterationDepth" },
    @{ from = "MaterialExpressionScalarParameter_5"; pin = "Scale" },
    @{ from = "MaterialExpressionScalarParameter_6"; pin = "RotationSpeed" },
    @{ from = "MaterialExpressionScalarParameter_7"; pin = "GlowIntensity" },
    @{ from = "MaterialExpressionVectorParameter_3"; pin = "BaseColor" },
    @{ from = "MaterialExpressionVectorParameter_4"; pin = "EdgeColor" },
    @{ from = "MaterialExpressionVectorParameter_5"; pin = "InkColor" }
)

$customNode = "MaterialExpressionCustom_0"

foreach ($w in $wiring) {
    $r = Call-Mono "connect_expressions" @{
        asset_path = $fp
        from_expression = $w.from
        to_expression = $customNode
        to_pin = $w.pin
    }
    try { $result = $r | ConvertFrom-Json; $status = if ($result.connected) { "OK" } else { "FAIL: $($r)" } } catch { $status = "RESP: $($r)" }
    Write-Host "  $($w.from) -> $($w.pin) = $status"
}

# Wire output to EmissiveColor
Call-Mono "connect_expressions" @{ asset_path = $fp; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
Write-Host "  Custom -> EmissiveColor = wired"

# Recompile
Call-Mono "recompile_material" @{ asset_path = $fp } | Out-Null
Start-Sleep -Seconds 5
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
if ($r) {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else { Write-Host "  FAILED" -ForegroundColor Red; if ($s.compile_errors) { Write-Host ($s.compile_errors | Out-String) -ForegroundColor Red } }
}
