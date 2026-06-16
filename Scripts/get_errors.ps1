$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$arguments = [ordered]@{ action = "get_compilation_stats"; asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_SheetMusic_Score" }
$body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
$r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 30
$text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
$s = $text.text | ConvertFrom-Json
Write-Host "Compiled: $($s.is_compiled)"
Write-Host "PS: $($s.num_pixel_shader_instructions)"
foreach ($e in $s.compile_errors) {
    $clean = $e -replace '\\n',"`n" -replace '\\r',""
    Write-Host "ERROR:`n$clean" -ForegroundColor Red
}
