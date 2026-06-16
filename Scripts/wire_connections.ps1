$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] }
    $body = @{
        jsonrpc = "2.0"; id = 1; method = "tools/call"
        params = @{ name = "material_query"; arguments = $arguments }
    } | ConvertTo-Json -Depth 20 -Compress
    $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 60
    $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
    if ($text) { return $text.text }
    return ($r | ConvertTo-Json -Depth 3 -Compress)
}

# Wire all inputs
$wires = @(
    @{ from = "MaterialExpressionTextureCoordinate_0"; to = "MaterialExpressionCustom_0"; to_pin = "UV" }
    @{ from = "MaterialExpressionTime_0"; to = "MaterialExpressionCustom_0"; to_pin = "Time" }
    @{ from = "MaterialExpressionScalarParameter_0"; to = "MaterialExpressionCustom_0"; to_pin = "Power" }
    @{ from = "MaterialExpressionScalarParameter_1"; to = "MaterialExpressionCustom_0"; to_pin = "MaxIter" }
    @{ from = "MaterialExpressionScalarParameter_2"; to = "MaterialExpressionCustom_0"; to_pin = "SliceOff" }
    @{ from = "MaterialExpressionScalarParameter_3"; to = "MaterialExpressionCustom_0"; to_pin = "RotSpd" }
    @{ from = "MaterialExpressionScalarParameter_4"; to = "MaterialExpressionCustom_0"; to_pin = "ScaleParam" }
    @{ from = "MaterialExpressionScalarParameter_5"; to = "MaterialExpressionCustom_0"; to_pin = "AOStr" }
    @{ from = "MaterialExpressionVectorParameter_0"; to = "MaterialExpressionCustom_0"; to_pin = "BaseCol" }
    @{ from = "MaterialExpressionVectorParameter_1"; to = "MaterialExpressionCustom_0"; to_pin = "GlowCol" }
    @{ from = "MaterialExpressionVectorParameter_2"; to = "MaterialExpressionCustom_0"; to_pin = "InkCol" }
)

foreach ($w in $wires) {
    Write-Host "  Wiring $($w.to_pin)..."
    $r = Call-Mono "connect_expressions" @{
        asset_path = $path
        from_expression = $w.from
        to_expression = $w.to
        to_pin = $w.to_pin
    }
    Write-Host "    $r"
}

# Connect output to EmissiveColor
Write-Host "`nWiring output to EmissiveColor..."
$r = Call-Mono "connect_expressions" @{
    asset_path = $path
    from_expression = "MaterialExpressionCustom_0"
    to_property = "EmissiveColor"
}
Write-Host "  Output: $r"

# Recompile
Write-Host "`nRecompiling..."
$r = Call-Mono "recompile_material" @{ asset_path = $path }
Write-Host "  $r"

# Get stats
Write-Host "`nGetting stats..."
Start-Sleep -Seconds 5
$r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
Write-Host "  STATS: $r" -ForegroundColor Yellow
