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

$materials = @(
    "M_SDF_SheetMusic_Score",
    "M_SDF_TrebleClef_Ornament",
    "M_SDF_FloatingNotes",
    "M_SDF_GrandStaff_CrossSection",
    "M_SDF_VinylRecord"
)

foreach ($m in $materials) {
    $path = "/Game/_PROJECT/04_Materials/SDF/$m"
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $path }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            $color = if ($s.is_compiled) { "Green" } else { "Red" }
            Write-Host "$m : compiled=$($s.is_compiled) PS=$($s.num_pixel_shader_instructions) expr=$($s.expression_count)" -ForegroundColor $color
        } catch { Write-Host "$m : parse error" -ForegroundColor Yellow }
    } else { Write-Host "$m : NET error" -ForegroundColor Red }
    Start-Sleep -Seconds 1
}
