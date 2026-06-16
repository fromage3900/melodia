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

# Check 3 materials that had 84PS via export_material_graph
$check = @("M_SDF_FractalOrnament", "M_SDF_MengerSponge", "M_SDF_GothicRoseWindow")

foreach ($mat in $check) {
    $fp = "$sdfPath/$mat"
    Write-Host "`n--- $mat ---" -ForegroundColor Cyan
    $r = Call-Mono "export_material_graph" @{ asset_path = $fp }
    try {
        $g = $r | ConvertFrom-Json
        Write-Host "  Nodes: $($g.nodes.Count) | Custom: $($g.custom_hlsl_nodes.Count) | Connections: $($g.connections.Count)"
        if ($g.custom_hlsl_nodes.Count -gt 0) {
            $ch = $g.custom_hlsl_nodes[0]
            Write-Host "  Custom HLSL: $($ch.id) | inputs: $($ch.inputs.Count) | code length: $($ch.code.Length)"
        }
        if ($g.connections.Count -gt 0) {
            foreach ($c in $g.connections) { Write-Host "    $($c.from) -> $($c.to) ($($c.to_pin))" }
        }
    } catch { Write-Host "  Parse error" -ForegroundColor Red }
    Start-Sleep -Seconds 1
}
