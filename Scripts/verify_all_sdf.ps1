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
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

$materials = @(
    "M_SDF_Mandelbulb_Master",
    "M_SDF_JuliaSet_Quaternion",
    "M_SDF_SierpinskiTetrahedron",
    "M_SDF_FractalOrnament",
    "M_SDF_MengerSponge",
    "M_SDF_GothicRoseWindow",
    "M_SDF_CathedralVault",
    "M_SDF_BaroqueColumn",
    "M_SDF_FlyingButtress",
    "M_SDF_GildedAltar"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  SDF MATERIAL LIBRARY VERIFICATION" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$totalOK = 0; $totalFail = 0
$results = @()

foreach ($mat in $materials) {
    $fp = "$sdfPath/$mat"
    Write-Host "`n--- $mat ---" -ForegroundColor Yellow
    
    # Recompile
    Call-Mono "recompile_material" @{ asset_path = $fp } | Out-Null
    Start-Sleep -Seconds 3
    
    # Stats
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
    $compiled = $false; $ps = 0; $exprCount = 0
    try {
        $s = $r | ConvertFrom-Json
        $compiled = $s.is_compiled
        $ps = $s.num_pixel_shader_instructions
        $exprCount = $s.num_expressions
    } catch { Write-Host "  Stats parse error" -ForegroundColor Red }
    
    # Validate
    $vr = Call-Mono "validate_material" @{ asset_path = $fp }
    $warnings = @()
    try {
        $v = $vr | ConvertFrom-Json
        if ($v.warnings) { $warnings = @($v.warnings) }
    } catch {}
    
    if ($compiled) {
        $totalOK++
        $status = "OK"
        $color = "Green"
        $budgetNote = ""
        # Budget check
        if ($mat -match "Mandelbulb|Julia") { $budget = 500 }
        elseif ($mat -match "Sierpinski|Menger") { $budget = 400 }
        else { $budget = 500 }
        if ($ps -gt $budget) { $budgetNote = " [OVER BUDGET $budget]" ; $color = "Yellow" }
        Write-Host "  $status | $ps PS | $exprCount exprs | $($warnings.Count) warnings $budgetNote" -ForegroundColor $color
    } else {
        $totalFail++
        Write-Host "  FAIL | $ps PS" -ForegroundColor Red
    }
    
    $results += [PSCustomObject]@{ Material = $mat; Status = if ($compiled) { "OK" } else { "FAIL" }; PS = $ps; Exprs = $exprCount; Warnings = $warnings.Count }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  SUMMARY: $totalOK OK / $totalFail FAIL / $($materials.Count) total" -ForegroundColor $(if ($totalFail -eq 0) { "Green" } else { "Red" })
Write-Host "========================================" -ForegroundColor Cyan

Write-Host "`n--- Results Table ---" -ForegroundColor Cyan
$results | Format-Table -AutoSize
