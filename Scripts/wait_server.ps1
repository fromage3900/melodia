$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
Write-Host "Waiting for Monolith server..." -ForegroundColor Yellow
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Seconds 5
    try {
        $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = @{ action = "get_all_materials" } } } | ConvertTo-Json -Depth 10 -Compress
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 15
        Write-Host "Server UP at attempt $i" -ForegroundColor Green
        exit 0
    } catch { Write-Host "  [$i] still down..." }
}
Write-Host "Server did not come up after 5 minutes" -ForegroundColor Red
