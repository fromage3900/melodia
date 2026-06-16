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

function Rewire-Material($path, $inputNames) {
    Write-Host "`n=== Rewiring $path ===" -ForegroundColor Cyan
    $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
    if (-not $r) { Write-Host "  get_all_expressions failed, trying export..." -ForegroundColor Yellow
        Start-Sleep -Seconds 5
        $er = Call-Mono "export_material_graph" @{ asset_path = $path }
        if ($er) {
            $exp = $er | ConvertFrom-Json
            Write-Host "  Export: nodes=$($exp.nodes.Count) custom=$($exp.custom_hlsl_nodes.Count)"
            $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
        }
    }
    if (-not $r) { Write-Host "  FAILED to get expressions" -ForegroundColor Red; return $false }
    try {
        $parsed = $r | ConvertFrom-Json
        if (-not $parsed.expressions -or $parsed.expressions.Count -eq 0) {
            Write-Host "  Empty expressions" -ForegroundColor Red; return $false
        }
        $exprs = $parsed.expressions
    } catch { Write-Host "  Parse fail" -ForegroundColor Red; return $false }
    
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" } | Select-Object -First 1).name
    $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
    Write-Host "  Custom=$customNode Params=$($paramExprs.Count) Pins=$($inputNames.Count)"
    
    for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
        $pn = $paramExprs[$i].name
        $r = Call-Mono "connect_expressions" @{ asset_path = $path; from_expression = $pn; to_expression = $customNode; to_pin = $inputNames[$i] }
        try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR" }
        Write-Host "    [$i] $pn -> $($inputNames[$i]) = $st"
        Start-Sleep -Seconds 1
    }
    Call-Mono "connect_expressions" @{ asset_path = $path; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    Start-Sleep -Seconds 2
    Call-Mono "recompile_material" @{ asset_path = $path } | Out-Null
    Write-Host "  Compiling..."
    Start-Sleep -Seconds 10
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green; Call-Mono "save_material" @{ asset_path = $path } | Out-Null; return $true }
            else {
                Write-Host "  COMPILE FAIL" -ForegroundColor Red
                $e = @($s.compile_errors)
                if ($e.Count -gt 0) { Write-Host "  $($e[0].Substring(0,[Math]::Min(400,$e[0].Length)))" -ForegroundColor Red }
                return $false
            }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow; return $false }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow; return $false }
}

# Rewire M1: SheetMusic_Score (7 inputs)
Rewire-Material "/Game/_PROJECT/04_Materials/SDF/M_SDF_SheetMusic_Score" @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")

Start-Sleep -Seconds 5

# Rewire M3: FloatingNotes (6 inputs)
Rewire-Material "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" @("UV","Time","NoteColorA","NoteColorB","AccentColor","GlowIntensity")
