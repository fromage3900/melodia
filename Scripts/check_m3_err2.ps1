$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$arguments = [ordered]@{ action = "get_compilation_stats"; asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" }
$body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
$r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 30
$text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
$s = $text.text | ConvertFrom-Json
Write-Host "Compiled: $($s.is_compiled)"
# Just show first error fully
$e = @($s.compile_errors)
Write-Host "Total errors: $($e.Count)"
if ($e.Count -gt 0) {
    $err = $e[0] -replace '\\n',"`n" -replace '\\r',""
    Write-Host "ERROR 0:`n$err" -ForegroundColor Red
}
if ($e.Count -gt 1) {
    $err = $e[1] -replace '\\n',"`n" -replace '\\r',""
    Write-Host "ERROR 1:`n$err" -ForegroundColor Red
}
