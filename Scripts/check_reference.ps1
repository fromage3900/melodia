$ErrorActionPreference = "Continue"
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
        return ($r | ConvertTo-Json -Depth 10 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

# Check reference material M_SDF_Baroque (known working, 622 PS)
Write-Host "=== Reference: M_SDF_Baroque ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Baroque" }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count) | Custom: $($g.custom_hlsl_nodes.Count) | Connections: $($g.connections.Count) | Outputs: $($g.outputs.Count)" -ForegroundColor Green
    if ($g.connections.Count -gt 0) {
        Write-Host "  Connections found! This material persists correctly." -ForegroundColor Green
        foreach ($c in $g.connections) { Write-Host "    $($c.from) -> $($c.to) ($($c.to_pin))" }
    } else {
        Write-Host "  No connections - reference also has the issue!" -ForegroundColor Red
    }
    if ($g.outputs.Count -gt 0) {
        foreach ($o in $g.outputs) { Write-Host "  Output: $($o.from) -> $($o.to_property)" }
    }
} catch { Write-Host "  Parse error" -ForegroundColor Red }

# Also check Mandelbulb_Master which was rebuilt with fresh connections
Write-Host "`n=== Fresh: M_SDF_Mandelbulb_Master ===" -ForegroundColor Cyan
$r = Call-Mono "export_material_graph" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master" }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count) | Custom: $($g.custom_hlsl_nodes.Count) | Connections: $($g.connections.Count) | Outputs: $($g.outputs.Count)" -ForegroundColor Green
    if ($g.connections.Count -gt 0) {
        foreach ($c in $g.connections) { Write-Host "    $($c.from) -> $($c.to) ($($c.to_pin))" }
    }
    if ($g.outputs.Count -gt 0) {
        foreach ($o in $g.outputs) { Write-Host "  Output: $($o.from) -> $($o.to_property)" }
    }
} catch { Write-Host "  Parse error" -ForegroundColor Red }
