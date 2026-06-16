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

Write-Host "=== Extracting expression names from export ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = $fullPath }
try {
    $graph = $r | ConvertFrom-Json
    Write-Host "Found $($graph.nodes.Count) nodes" -ForegroundColor Green
    foreach ($n in $graph.nodes) {
        $pName = ""
        if ($n.props -and $n.props.ParameterName) { $pName = " ($($n.props.ParameterName))" }
        Write-Host "  $($n.id) [$($n.class)]$pName"
    }
    if ($graph.custom_hlsl_nodes) {
        Write-Host "Custom HLSL nodes:" -ForegroundColor Yellow
        foreach ($n in $graph.custom_hlsl_nodes) {
            Write-Host "  $($n.id) inputs=$($n.inputs.Count)"
        }
    }
    if ($graph.connections) {
        Write-Host "Connections: $($graph.connections.Count)" -ForegroundColor Yellow
    } else {
        Write-Host "No connections found" -ForegroundColor Red
    }
} catch {
    Write-Host "Parse error: $_" -ForegroundColor Red
}

# Now wire the connections manually
Write-Host "`n=== Wiring connections ===" -ForegroundColor Cyan
$customNode = "MaterialExpressionCustom_1"
$inputNames = @("UV","Time","Power","MaxIterations","SliceOffset","RotationSpeed","Scale","BevelRadius","AOStrength","BaseColor","GlowColor","InkColor")
$paramNames = @(
    "MaterialExpressionTextureCoordinate_1",
    "MaterialExpressionTime_1",
    "MaterialExpressionScalarParameter_6",
    "MaterialExpressionScalarParameter_7",
    "MaterialExpressionScalarParameter_8",
    "MaterialExpressionScalarParameter_9",
    "MaterialExpressionScalarParameter_10",
    "MaterialExpressionScalarParameter_11",
    "MaterialExpressionScalarParameter_12",
    "MaterialExpressionVectorParameter_5",
    "MaterialExpressionVectorParameter_6",
    "MaterialExpressionVectorParameter_7"
)

for ($i = 0; $i -lt $paramNames.Count -and $i -lt $inputNames.Count; $i++) {
    $r = Call-Mono "connect_expressions" @{
        asset_path = $fullPath; from_expression = $paramNames[$i]
        to_expression = $customNode; to_pin = $inputNames[$i]
    }
    try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR: $r" }
    Write-Host "  [$i] $($paramNames[$i]) -> $($inputNames[$i]) = $st"
}
Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
Write-Host "  Custom -> EmissiveColor"

# Recompile
Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
Start-Sleep -Seconds 8
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
try {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else {
        Write-Host "  FAILED" -ForegroundColor Red
        if ($s.compile_errors) {
            foreach ($e in @($s.compile_errors)) { Write-Host "  $($e.Substring(0,[Math]::Min(300,$e.Length)))" -ForegroundColor Red }
        }
    }
} catch { Write-Host "  Parse error" -ForegroundColor Yellow }
