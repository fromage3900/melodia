$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fullPath = "/Game/_PROJECT/04_Materials/SDF/M_SDF_BaroqueColumn"

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

Write-Host "=== Building M_SDF_BaroqueColumn ===" -ForegroundColor Cyan

# Delete and recreate
Call-Mono "delete_asset" @{ asset_path = $fullPath } | Out-Null
Start-Sleep -Seconds 2
Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null

$nodes = @(
    @{ id = "TC"; class = "TextureCoordinate"; pos = @(-1200, -200) },
    @{ id = "TM"; class = "Time"; pos = @(-1200, -140) },
    @{ id = "CR"; class = "ScalarParameter"; pos = @(-1200, -80); props = @{ ParameterName = "ColumnRadius" } },
    @{ id = "FC"; class = "ScalarParameter"; pos = @(-1200, -20); props = @{ ParameterName = "FlutingCount" } },
    @{ id = "CD"; class = "ScalarParameter"; pos = @(-1200, 40); props = @{ ParameterName = "CapitalDetail" } },
    @{ id = "BC"; class = "VectorParameter"; pos = @(-1200, 100); props = @{ ParameterName = "BaseColor" } },
    @{ id = "GC"; class = "VectorParameter"; pos = @(-1200, 160); props = @{ ParameterName = "GoldColor" } },
    @{ id = "IC"; class = "VectorParameter"; pos = @(-1200, 220); props = @{ ParameterName = "InkColor" } }
)
Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'
$columnCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.5,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*0.08;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float cr=max(ColumnRadius,0.05);
float t=0.0;float hit=-1.0;float capitalMask=0.0;float fluteMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float py=p.y;
  float entasis=cr*(1.0-0.08*pow(abs(py-0.5)/1.5,2.0));
  float shaft=length(p.xz)-entasis;
  float flutes=sin(atan2(p.z,p.x)*FlutingCount)*0.02*cr;
  shaft+=flutes;
  fluteMask=abs(flutes)*20.0;
  float capitalY=max(py-1.2,0.0);
  float capitalR=length(p.xz)-(cr*1.4+0.1*sin(py*20.0)*CapitalDetail*max(1.5-py,0.0));
  float capital=max(capitalR,-(py-1.5));
  if(capitalY>0.0){shaft=capital;capitalMask=1.0;}
  float baseY=max(-py-1.0,0.0);
  float torus1=abs(length(p.xz)-(cr*1.2))-0.04;
  float torus2=abs(length(p.xz)-(cr*1.3))-0.03;
  float base=max(min(torus1,torus2),-(-py-1.3));
  if(baseY>0.0){shaft=base;capitalMask=0.0;}
  float d=shaft;
  if(d<0.001){hit=t;break;}
  t+=d*0.8;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=BaseColor;
col=lerp(col,GoldColor,capitalMask*0.7);
col+=fluteMask*0.1*BaseColor;
float3 L=normalize(float3(0.5,0.7,-0.5));
col*=(0.2+0.8*saturate(dot(nrm,L)));
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=GoldColor*fres*0.15*capitalMask;
col=lerp(InkColor,col,saturate(0.4+0.6*abs(dot(nrm,L))));
return saturate(col);
"@

$inputNames = @("UV","Time","ColumnRadius","FlutingCount","CapitalDetail","BaseColor","GoldColor","InkColor")
$inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
$r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $columnCode; description = "M_SDF_BaroqueColumn"; output_type = "CMOT_Float3"; inputs = $inputObjs }
$cn = ($r | ConvertFrom-Json).expression_name
Write-Host "  Custom=$cn inputs=$(($r | ConvertFrom-Json).input_count)"

# Get expressions with retry
$exprs = $null
for ($attempt = 1; $attempt -le 4; $attempt++) {
    Start-Sleep -Seconds 4
    $r = Call-Mono "get_all_expressions" @{ asset_path = $fullPath }
    try {
        $parsed = $r | ConvertFrom-Json
        if ($parsed.expressions) { $exprs = $parsed.expressions; break }
    } catch { Write-Host "  Attempt $attempt failed..." -ForegroundColor Yellow }
}
if (-not $exprs) { Write-Host "FATAL: No expressions found" -ForegroundColor Red; exit 1 }

$customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" }).name
if (-not $customNode) { $customNode = $cn }
$paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
Write-Host "  Wiring $($paramExprs.Count) params..."
for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
    $r = Call-Mono "connect_expressions" @{
        asset_path = $fullPath; from_expression = $paramExprs[$i].name
        to_expression = $customNode; to_pin = $inputNames[$i]
    }
    try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR: $r" }
    Write-Host "    [$i] $($paramExprs[$i].name) -> $($inputNames[$i]) = $st"
}
Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null

Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
Start-Sleep -Seconds 5
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
try {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else { Write-Host "  FAILED" -ForegroundColor Red; if ($s.compile_errors) { Write-Host ($s.compile_errors[0]) -ForegroundColor Red } }
} catch { Write-Host "  Parse error" -ForegroundColor Yellow }
