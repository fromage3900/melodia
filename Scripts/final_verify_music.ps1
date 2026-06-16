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

function Rewire-And-Recompile($path, $inputNames) {
    $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
    if (-not $r) { Write-Host "  No expressions" -ForegroundColor Red; return $false }
    try { $parsed = $r | ConvertFrom-Json } catch { Write-Host "  Parse fail" -ForegroundColor Red; return $false }
    $exprs = $parsed.expressions
    if (-not $exprs -or $exprs.Count -eq 0) { Write-Host "  Empty" -ForegroundColor Red; return $false }
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" } | Select-Object -First 1).name
    $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
    for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
        Call-Mono "connect_expressions" @{ asset_path = $path; from_expression = $paramExprs[$i].name; to_expression = $customNode; to_pin = $inputNames[$i] } | Out-Null
        Start-Sleep -Seconds 1
    }
    Call-Mono "connect_expressions" @{ asset_path = $path; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    Start-Sleep -Seconds 2
    Call-Mono "recompile_material" @{ asset_path = $path } | Out-Null
    Start-Sleep -Seconds 10
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) {
                Write-Host "  REWIRED+COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green
                Call-Mono "save_material" @{ asset_path = $path } | Out-Null
                return $true
            }
        } catch {}
    }
    return $false
}

# Check all 5 materials
Write-Host "=== FINAL VERIFICATION ===" -ForegroundColor Cyan
$materials = @{
    "M_SDF_SheetMusic_Score" = @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")
    "M_SDF_TrebleClef_Ornament" = @()
    "M_SDF_FloatingNotes" = @()
    "M_SDF_GrandStaff_CrossSection" = @()
    "M_SDF_VinylRecord" = @()
}

foreach ($m in $materials.Keys | Sort-Object) {
    $path = "/Game/_PROJECT/04_Materials/SDF/$m"
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled -and $s.num_pixel_shader_instructions -gt 84) {
                Write-Host "$m : OK ($($s.num_pixel_shader_instructions) PS)" -ForegroundColor Green
            } elseif ($s.is_compiled -and $s.num_pixel_shader_instructions -eq 84) {
                Write-Host "$m : NEEDS REWIRE (84 PS)" -ForegroundColor Yellow
                $inputNames = $materials[$m]
                if ($inputNames.Count -gt 0) {
                    $ok = Rewire-And-Recompile $path $inputNames
                    if ($ok) { Write-Host "  -> FIXED" -ForegroundColor Green }
                }
            } else {
                Write-Host "$m : FAIL (compiled=$($s.is_compiled) PS=$($s.num_pixel_shader_instructions))" -ForegroundColor Red
            }
        } catch { Write-Host "$m : parse error" -ForegroundColor Yellow }
    } else { Write-Host "$m : NET error" -ForegroundColor Red }
    Start-Sleep -Seconds 1
}
