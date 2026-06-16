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

# Check M1
Write-Host "=== M1 SheetMusic_Score ===" -ForegroundColor Cyan
$r = Call-Mono "get_compilation_stats" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_SheetMusic_Score" }
if ($r) {
    try { $s = $r | ConvertFrom-Json; Write-Host "Compiled=$($s.is_compiled) PS=$($s.num_pixel_shader_instructions)" } catch { Write-Host "Parse fail" }
}

# Try export for M2
Write-Host "`n=== M2 TrebleClef - export ===" -ForegroundColor Cyan
Start-Sleep -Seconds 3
$r = Call-Mono "export_material_graph" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_TrebleClef_Ornament" }
if ($r) {
    try {
        $exp = $r | ConvertFrom-Json
        Write-Host "Nodes=$($exp.nodes.Count) Custom=$($exp.custom_hlsl_nodes.Count) Connections=$($exp.connections.Count)"
        foreach ($n in $exp.nodes) { Write-Host "  Node: $($n.name) class=$($n.class)" }
        foreach ($n in $exp.custom_hlsl_nodes) { Write-Host "  Custom: $($n.name)" }
    } catch { Write-Host "Export parse fail" }
} else { Write-Host "Export returned null" }

# Try get_all_expressions for M2 with long wait
Write-Host "`n=== M2 TrebleClef - get_all_expressions with 15s wait ===" -ForegroundColor Cyan
Start-Sleep -Seconds 15
$r = Call-Mono "get_all_expressions" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_TrebleClef_Ornament" }
if ($r) {
    try {
        $parsed = $r | ConvertFrom-Json
        if ($parsed.expressions) {
            Write-Host "Got $($parsed.expressions.Count) expressions!"
            foreach ($e in $parsed.expressions) { Write-Host "  $($e.name) - $($e.class)" }
        } else { Write-Host "No expressions field" }
    } catch { Write-Host "Parse fail: $($_.Exception.Message)" }
} else { Write-Host "get_all_expressions returned null" }
