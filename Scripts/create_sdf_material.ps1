# Helper function to create an SDF material via Monolith MCP
function New-SDFMaterial {
    param(
        [string]$AssetPath,
        [string]$Description,
        [string]$HlslCode,
        [hashtable[]]$Params,       # array of @{Name="X"; Type="scalar"|"vector"; Default=value; Pos=@(x,y)}
        [hashtable[]]$VectorDefaults # array of @{Name="X"; Value=@(r,g,b,a)}
    )
    $url = "http://localhost:9316/mcp"
    Write-Host "Creating: $AssetPath" -ForegroundColor Cyan

    # 1. Create material
    $b = @{ jsonrpc="2.0"; id=1; method="tools/call"; params=@{ name="material_query"; arguments=@{
        action="create_material"; asset_path=$AssetPath; blend_mode="Opaque"; shading_model="Unlit"; two_sided=$true
    }}} | ConvertTo-Json -Depth 5 -Compress
    $r = Invoke-RestMethod -Uri $url -Method Post -Body $b -ContentType "application/json" -TimeoutSec 30
    Write-Host "  Created: $($r.result.content[0].text)" -ForegroundColor Gray

    # 2. Build graph spec
    $nodes = @()
    $connections = @()
    $inputs = @()
    $yOffset = 0
    foreach ($p in $Params) {
        $nodeId = $p.Name -replace '[^a-zA-Z0-9]', ''
        if ($p.Type -eq "scalar") {
            $nodes += @{ id = $nodeId; class = "ScalarParameter"; pos = @(-900, $yOffset); props = @{ ParameterName = $p.Name; DefaultValue = $p.Default } }
            $inputs += @{ name = $p.Name; type = "float" }
        } elseif ($p.Type -eq "vector") {
            $nodes += @{ id = $nodeId; class = "VectorParameter"; pos = @(-900, $yOffset); props = @{ ParameterName = $p.Name } }
            $inputs += @{ name = $p.Name; type = "float3" }
        } elseif ($p.Type -eq "texcoord") {
            $nodes += @{ id = $nodeId; class = "TextureCoordinate"; pos = @(-900, $yOffset) }
            $inputs += @{ name = $p.Name; type = "float2" }
        } elseif ($p.Type -eq "time") {
            $nodes += @{ id = $nodeId; class = "Time"; pos = @(-900, $yOffset) }
            $inputs += @{ name = $p.Name; type = "float" }
        }
        $connections += @{ from = $nodeId; to = "SDFCore"; to_pin = $p.Name }
        $yOffset += 120
    }

    $graphSpec = @{
        nodes = $nodes
        custom_hlsl_nodes = @(
            @{
                id = "SDFCore"
                description = $Description
                output_type = "CMOT_Float3"
                pos = @(-200, 0)
                code = $HlslCode
                inputs = $inputs
                additional_outputs = @()
            }
        )
        connections = $connections
        outputs = @(
            @{ from = "SDFCore"; to_property = "EmissiveColor" }
        )
    }

    $bodyObj = @{
        jsonrpc = "2.0"; id = 1; method = "tools/call"
        params = @{ name = "material_query"; arguments = @{
            action = "build_material_graph"
            asset_path = $AssetPath
            clear_existing = $true
            graph_spec = $graphSpec
        }}
    }
    $bodyJson = $bodyObj | ConvertTo-Json -Depth 15 -Compress
    $r = Invoke-RestMethod -Uri $url -Method Post -Body $bodyJson -ContentType "application/json" -TimeoutSec 90
    $result = $r.result.content[0].text | ConvertFrom-Json
    Write-Host "  Graph: nodes=$($result.nodes_created) connections=$($result.connections_made) errors=$($result.errors.Count)" -ForegroundColor Yellow
    if ($result.errors.Count -gt 0) {
        foreach ($e in $result.errors) { Write-Host "    ERR: $($e | ConvertTo-Json -Compress)" -ForegroundColor Red }
    }

    # 3. Set vector parameter defaults
    if ($VectorDefaults) {
        # Get expression names first
        $exprBody = @{ jsonrpc="2.0"; id=1; method="tools/call"; params=@{ name="material_query"; arguments=@{
            action="get_all_expressions"; asset_path=$AssetPath
        }}} | ConvertTo-Json -Depth 5 -Compress
        $exprR = Invoke-RestMethod -Uri $url -Method Post -Body $exprBody -ContentType "application/json" -TimeoutSec 30
        $exprData = $exprR.result.content[0].text | ConvertFrom-Json
        
        foreach ($vd in $VectorDefaults) {
            $exprName = ($exprData.expressions | Where-Object { $_.parameter_name -eq $vd.Name } | Select-Object -First 1).name
            if ($exprName) {
                $setBody = @{ jsonrpc="2.0"; id=1; method="tools/call"; params=@{ name="material_query"; arguments=@{
                    action="set_expression_property"; asset_path=$AssetPath; expression_name=$exprName; property_name="DefaultValue"; value=$vd.Value
                }}} | ConvertTo-Json -Depth 5 -Compress
                Invoke-RestMethod -Uri $url -Method Post -Body $setBody -ContentType "application/json" -TimeoutSec 30 | Out-Null
                Write-Host "  Set $($vd.Name) = $($vd.Value -join ',')" -ForegroundColor Gray
            }
        }
    }

    # 4. Recompile
    $rcBody = @{ jsonrpc="2.0"; id=1; method="tools/call"; params=@{ name="material_query"; arguments=@{
        action="recompile_material"; asset_path=$AssetPath
    }}} | ConvertTo-Json -Depth 5 -Compress
    Invoke-RestMethod -Uri $url -Method Post -Body $rcBody -ContentType "application/json" -TimeoutSec 60 | Out-Null

    # 5. Get stats
    $stBody = @{ jsonrpc="2.0"; id=1; method="tools/call"; params=@{ name="material_query"; arguments=@{
        action="get_compilation_stats"; asset_path=$AssetPath
    }}} | ConvertTo-Json -Depth 5 -Compress
    $stR = Invoke-RestMethod -Uri $url -Method Post -Body $stBody -ContentType "application/json" -TimeoutSec 30
    $stats = $stR.result.content[0].text | ConvertFrom-Json
    if ($stats.is_compiled) {
        Write-Host "  OK: PS=$($stats.num_pixel_shader_instructions) VS=$($stats.num_vertex_shader_instructions) Expr=$($stats.expression_count)" -ForegroundColor Green
    } else {
        Write-Host "  FAILED: $($stats.compile_errors -join '; ')" -ForegroundColor Red
    }
    Start-Sleep -Milliseconds 500
}
