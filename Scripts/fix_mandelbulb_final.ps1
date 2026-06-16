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

Write-Host "=== Fixing Mandelbulb last 2 connections ===" -ForegroundColor Cyan
$custom = "MaterialExpressionCustom_1"

# Fix GlowColor (VectorParameter_4) and InkColor (VectorParameter_5)
$r = Call-Mono "connect_expressions" @{ asset_path = $fp; from_expression = "MaterialExpressionVectorParameter_4"; to_expression = $custom; to_pin = "GlowColor" }
try { $cr = $r | ConvertFrom-Json; Write-Host "  GlowColor (VP_4) = $(if($cr.connected){'OK'}else{'FAIL'})" } catch { Write-Host "  GlowColor ERR: $r" }

$r = Call-Mono "connect_expressions" @{ asset_path = $fp; from_expression = "MaterialExpressionVectorParameter_5"; to_expression = $custom; to_pin = "InkColor" }
try { $cr = $r | ConvertFrom-Json; Write-Host "  InkColor (VP_5) = $(if($cr.connected){'OK'}else{'FAIL'})" } catch { Write-Host "  InkColor ERR: $r" }

Call-Mono "recompile_material" @{ asset_path = $fp } | Out-Null
Start-Sleep -Seconds 8
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
try {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else { Write-Host "  FAILED" -ForegroundColor Red; if ($s.compile_errors) { foreach ($e in @($s.compile_errors)) { Write-Host "  $($e.Substring(0,[Math]::Min(300,$e.Length)))" -ForegroundColor Red } } }
} catch { Write-Host "  Parse error" -ForegroundColor Yellow }
