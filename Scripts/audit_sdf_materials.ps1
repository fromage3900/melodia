# SDF Material Audit Script - checks compilation stats for all SDF materials via Monolith MCP
$url = "http://localhost:9316/mcp"

$materials = @(
    # SDF main folder
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Baroque",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_CosmicPortal",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_CrystallineSpire",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_EscherGeometry_Enhanced",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloralMagic",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_GothicArchitecture",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_GothicArchitecture_Enhanced",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Grass_Field",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_InfinityMirror",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Klein_Bottle",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_MagicOrb",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_MandelbulbSlice",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_MetalShards",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mobius_Strip",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Musical",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_OrnamentLayer",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_OrnamentLayer_Enhanced",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_Penrose_Staircase",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_RayMarch_Gothic",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_StarburstGem",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_TrueParallax",
    "/Game/_PROJECT/04_Materials/SDF/M_MusicalSDF_PulsingGeometry",
    "/Game/_PROJECT/04_Materials/SDF/M_HybridStone_SDF",
    "/Game/_PROJECT/04_Materials/SDF/M_HybridWater_SDF_Inst",
    # SDF underwater
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_AbyssalVent",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_Anemone",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_Bioluminescence",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_BubbleColumn",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_Caustics_Underwater",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_CoralBranching",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_FishSchool_Caustics",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_KelpCurtain",
    "/Game/_PROJECT/04_Materials/SDF/Underwater/M_SDF_ThermalGlow",
    # Baroque
    "/Game/_PROJECT/04_Materials/baroque/M_SDF_GildedStucco",
    "/Game/_PROJECT/04_Materials/baroque/M_SDF_GildedFiligree",
    "/Game/_PROJECT/04_Materials/baroque/M_SDF_RoseWindow",
    # SDF instances
    "/Game/_PROJECT/04_Materials/SDF/MI_Architecture_GothicCathedral",
    "/Game/_PROJECT/04_Materials/SDF/MI_Architecture_ModernGeo",
    "/Game/_PROJECT/04_Materials/SDF/MI_Architecture_RomanBastille",
    "/Game/_PROJECT/04_Materials/SDF/MI_Baroque_GildedRose",
    "/Game/_PROJECT/04_Materials/SDF/MI_Baroque_InkwellRelief",
    "/Game/_PROJECT/04_Materials/SDF/MI_Music_AquaWave",
    "/Game/_PROJECT/04_Materials/SDF/MI_Music_NeonPulse",
    "/Game/_PROJECT/04_Materials/SDF/MI_Music_SunsetEQ",
    "/Game/_PROJECT/04_Materials/SDF/MI_MusicalSDF_AmberGlow",
    "/Game/_PROJECT/04_Materials/SDF/MI_MusicalSDF_CyanPulse",
    "/Game/_PROJECT/04_Materials/SDF/MI_Parallax_FLAT_TEST",
    "/Game/_PROJECT/04_Materials/SDF/MI_SDF_Gothic_RoseGold",
    "/Game/_PROJECT/04_Materials/SDF/M_SDF_TrueParallax_Inst",
    # Baroque instances
    "/Game/_PROJECT/04_Materials/baroque/MI_SDF_GildedFiligree",
    "/Game/_PROJECT/04_Materials/baroque/MI_SDF_BaroqueScrollwork",
    "/Game/_PROJECT/04_Materials/baroque/MI_SDF_RoseWindow"
)

$results = @()
foreach ($mat in $materials) {
    $name = $mat.Split("/")[-1]
    $body = "{`"jsonrpc`":`"2.0`",`"id`":1,`"method`":`"tools/call`",`"params`":{`"name`":`"material_query`",`"arguments`":{`"action`":`"get_compilation_stats`",`"asset_path`":`"$mat`"}}}"
    try {
        $r = Invoke-RestMethod -Uri $url -Method Post -Body $body -ContentType "application/json" -TimeoutSec 30
        $text = $r.result.content[0].text
        $data = $text | ConvertFrom-Json
        if ($data.compile_errors) {
            $status = "BROKEN"
            $errStr = ($data.compile_errors -join "; ")
        } elseif ($data.is_compiled) {
            $status = "OK"
            $errStr = ""
        } else {
            $status = "FAIL"
            $errStr = "not compiled"
        }
        $ps = if ($data.num_pixel_shader_instructions) { $data.num_pixel_shader_instructions } else { 0 }
        $vs = if ($data.num_vertex_shader_instructions) { $data.num_vertex_shader_instructions } else { 0 }
        $expr = if ($data.expression_count) { $data.expression_count } else { 0 }
        Write-Host "$name | $status | PS:$ps VS:$vs Expr:$expr $errStr"
        $results += [PSCustomObject]@{Name=$name; Status=$status; PS=$ps; VS=$vs; Expr=$expr; Errors=$errStr; Path=$mat}
    } catch {
        Write-Host "$name | ERROR | $($_.Exception.Message)"
        $results += [PSCustomObject]@{Name=$name; Status="ERROR"; PS=0; VS=0; Expr=0; Errors=$_.Exception.Message; Path=$mat}
    }
    Start-Sleep -Milliseconds 200
}

Write-Host "`n=== SUMMARY ==="
$ok = ($results | Where-Object { $_.Status -eq "OK" }).Count
$broken = ($results | Where-Object { $_.Status -eq "BROKEN" }).Count
$fail = ($results | Where-Object { $_.Status -eq "FAIL" }).Count
$error_ = ($results | Where-Object { $_.Status -eq "ERROR" }).Count
Write-Host "OK: $ok | BROKEN: $broken | FAIL: $fail | ERROR: $error_"
Write-Host "`nBroken/Error materials:"
$results | Where-Object { $_.Status -ne "OK" } | ForEach-Object { Write-Host "  $($_.Name): $($_.Errors)" }
