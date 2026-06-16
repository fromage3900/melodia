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
    } catch { Write-Host "  NET: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

$path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes"

# Check current state
$r = Call-Mono "export_material_graph" @{ asset_path = $path }
if ($r) {
    $exp = $r | ConvertFrom-Json
    Write-Host "Nodes=$($exp.nodes.Count) Custom=$($exp.custom_hlsl_nodes.Count) Connections=$($exp.connections.Count)"
    if ($exp.custom_hlsl_nodes.Count -gt 0) {
        $code = $exp.custom_hlsl_nodes[0].code
        Write-Host "HLSL length=$($code.Length)"
        if ($code -match 'fract') { Write-Host "WARNING: Code still contains 'fract'!" -ForegroundColor Red }
        if ($code -match 'frac\(') { Write-Host "Code contains 'frac' (correct)" -ForegroundColor Green }
        if ($code -match 'int noteKind') { Write-Host "Code contains 'int noteKind'" -ForegroundColor Yellow }
    }
}

# Force recompile
Write-Host "`nRecompiling..."
Call-Mono "recompile_material" @{ asset_path = $path } | Out-Null
Start-Sleep -Seconds 12

$r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
if ($r) {
    try {
        $s = $r | ConvertFrom-Json
        Write-Host "Compiled=$($s.is_compiled) PS=$($s.num_pixel_shader_instructions)"
        if (-not $s.is_compiled) {
            $e = @($s.compile_errors)
            Write-Host "Errors: $($e.Count)"
            if ($e.Count -gt 0) {
                $err = $e[0] -replace '\\n',"`n" -replace '\\r',""
                Write-Host $err -ForegroundColor Red
            }
        } else {
            Write-Host "SUCCESS!" -ForegroundColor Green
            Call-Mono "save_material" @{ asset_path = $path } | Out-Null
        }
    } catch { Write-Host "Parse error" }
}
