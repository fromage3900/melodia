$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fp = "/Game/_PROJECT/04_Materials/SDF/M_SDF_MengerSponge"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 120
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 10 -Compress)
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

Write-Host "=== Testing: build_material_graph with full spec ===" -ForegroundColor Cyan

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'
$mengerCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*RotationSpeed;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;float depthFrac=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=(ro+rd*t)*1.5;
  float d=1e10;float s=1.0;
  [loop]
  for(int i=0;i<int(Iterations);i++){
    float3 q=abs(p)-float3(1,1,1)*s;
    float box=length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0)-BevelRadius*s;
    float3 crosses=float3(max(abs(p.y),abs(p.z)),max(abs(p.x),abs(p.z)),max(abs(p.x),abs(p.y)));
    float crossD=min(min(crosses.x,crosses.y),crosses.z)-s*0.333;
    float menger=max(box,-crossD);
    d=min(d,menger/s);
    p=abs(p)-float3(1,1,1)*s;s/=3.0;
    depthFrac=float(i)/max(Iterations,1.0);
  }
  if(d<0.001){hit=t;break;}
  t+=d*0.8;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=lerp(InteriorColor,BaseColor,depthFrac);
float3 L=normalize(float3(0.5,0.5,-0.7));
col*=(0.2+0.8*saturate(dot(nrm,L)));
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=EdgeColor*fres*0.4;
return saturate(col);
"@

# Build everything in ONE call: nodes + custom_hlsl_node + connections + outputs
$graphSpec = @{
    nodes = @(
        @{ id = "TC"; class = "TextureCoordinate"; pos = @(-1200, -200) },
        @{ id = "TM"; class = "Time"; pos = @(-1200, -140) },
        @{ id = "IT"; class = "ScalarParameter"; pos = @(-1200, -80); props = @{ ParameterName = "Iterations" } },
        @{ id = "BR"; class = "ScalarParameter"; pos = @(-1200, -20); props = @{ ParameterName = "BevelRadius" } },
        @{ id = "RS"; class = "ScalarParameter"; pos = @(-1200, 40); props = @{ ParameterName = "RotationSpeed" } },
        @{ id = "BC"; class = "VectorParameter"; pos = @(-1200, 100); props = @{ ParameterName = "BaseColor" } },
        @{ id = "IC2"; class = "VectorParameter"; pos = @(-1200, 160); props = @{ ParameterName = "InteriorColor" } },
        @{ id = "EC"; class = "VectorParameter"; pos = @(-1200, 220); props = @{ ParameterName = "EdgeColor" } }
    )
    custom_hlsl_nodes = @(
        @{
            id = "SDF"
            code = $mengerCode
            output_type = "CMOT_Float3"
            inputs = @("UV","Time","Iterations","BevelRadius","RotationSpeed","BaseColor","InteriorColor","EdgeColor")
            pos = @(0, 0)
        }
    )
    connections = @(
        @{ from = "TC"; to = "SDF"; to_pin = "UV" },
        @{ from = "TM"; to = "SDF"; to_pin = "Time" },
        @{ from = "IT"; to = "SDF"; to_pin = "Iterations" },
        @{ from = "BR"; to = "SDF"; to_pin = "BevelRadius" },
        @{ from = "RS"; to = "SDF"; to_pin = "RotationSpeed" },
        @{ from = "BC"; to = "SDF"; to_pin = "BaseColor" },
        @{ from = "IC2"; to = "SDF"; to_pin = "InteriorColor" },
        @{ from = "EC"; to = "SDF"; to_pin = "EdgeColor" }
    )
    outputs = @(
        @{ from = "SDF"; to = "EmissiveColor" }
    )
}

$r = Call-Mono "build_material_graph" @{ asset_path = $fp; clear_existing = $true; graph_spec = $graphSpec }
Write-Host "Build result: $r" -ForegroundColor Yellow

Start-Sleep -Seconds 5

# Check
$r = Call-Mono "export_material_graph" @{ asset_path = $fp }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Nodes: $($g.nodes.Count) Custom: $($g.custom_hlsl_nodes.Count) Connections: $($g.connections.Count)" -ForegroundColor Cyan
} catch {}

# Compile
Call-Mono "recompile_material" @{ asset_path = $fp } | Out-Null
Start-Sleep -Seconds 5
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fp }
try {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else { Write-Host "  FAILED" -ForegroundColor Red; if ($s.compile_errors) { Write-Host ($s.compile_errors[0]) -ForegroundColor Red } }
} catch {}

# Save
$r = Call-Mono "save_material" @{ asset_path = $fp }
Write-Host "  Save: $r" -ForegroundColor Yellow

# Re-check after save
Start-Sleep -Seconds 3
$r = Call-Mono "export_material_graph" @{ asset_path = $fp }
try {
    $g = $r | ConvertFrom-Json
    Write-Host "  Post-save: Nodes=$($g.nodes.Count) Custom=$($g.custom_hlsl_nodes.Count) Connections=$($g.connections.Count)" -ForegroundColor $(if ($g.connections.Count -gt 0) { "Green" } else { "Red" })
} catch {}
