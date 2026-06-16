$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$fullPath = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master"

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

Write-Host "=== Rebuilding M_SDF_Mandelbulb_Master ===" -ForegroundColor Cyan

# Delete and recreate
Call-Mono "delete_asset" @{ asset_path = $fullPath } | Out-Null
Start-Sleep -Seconds 2
Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null

$nodes = @(
    @{ id = "TC"; class = "TextureCoordinate"; pos = @(-1200, -300) },
    @{ id = "TM"; class = "Time"; pos = @(-1200, -240) },
    @{ id = "PW"; class = "ScalarParameter"; pos = @(-1200, -180); props = @{ ParameterName = "Power" } },
    @{ id = "MI"; class = "ScalarParameter"; pos = @(-1200, -120); props = @{ ParameterName = "MaxIterations" } },
    @{ id = "SO"; class = "ScalarParameter"; pos = @(-1200, -60); props = @{ ParameterName = "SliceOffset" } },
    @{ id = "RS"; class = "ScalarParameter"; pos = @(-1200, 0); props = @{ ParameterName = "RotationSpeed" } },
    @{ id = "SC"; class = "ScalarParameter"; pos = @(-1200, 60); props = @{ ParameterName = "Scale" } },
    @{ id = "BR"; class = "ScalarParameter"; pos = @(-1200, 120); props = @{ ParameterName = "BevelRadius" } },
    @{ id = "AO"; class = "ScalarParameter"; pos = @(-1200, 180); props = @{ ParameterName = "AOStrength" } },
    @{ id = "BC"; class = "VectorParameter"; pos = @(-1200, 240); props = @{ ParameterName = "BaseColor" } },
    @{ id = "GC"; class = "VectorParameter"; pos = @(-1200, 300); props = @{ ParameterName = "GlowColor" } },
    @{ id = "IC"; class = "VectorParameter"; pos = @(-1200, 360); props = @{ ParameterName = "InkColor" } }
)
Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'

# Mandelbulb HLSL with #define macro pattern
$mandelbulbCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.0);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*RotationSpeed;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float pw=max(Power,2.0);
float Scl=max(Scale,0.5);
float t=0.0;float hit=-1.0;float4 trap=float4(1e10,1e10,1e10,1e10);
float aoAcc=0.0;float aoW=1.0;
[loop]
for(int step=0;step<100;step++){
  float3 z=(ro+rd*t)*Scl;
  float dr=1.0;float r=0.0;float4 tr=float4(1e10,1e10,1e10,1e10);
  [loop]
  for(int i=0;i<int(MaxIterations);i++){
    r=length(z);
    if(r>2.0)break;
    float theta=acos(clamp(z.z/r,-1.0,1.0));
    float phi=atan2(z.y,z.x);
    dr=pow(r,pw-1.0)*pw*dr+1.0;
    float zr=pow(r,pw);
    theta*=pw;phi*=pw;
    z=zr*float3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
    z+=(ro+rd*t)*Scl;
    tr=min(tr,float4(abs(z),r));
  }
  trap=min(trap,tr);
  float d=0.5*log(r)*r/dr;
  float slc=z.y-SliceOffset;
  d=max(d,-slc*0.5);
  aoAcc+=aoW*d;aoW*=0.75;
  if(d<BevelRadius){hit=t;break;}
  t+=d*0.8;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 col=lerp(InkColor,BaseColor,saturate(trap.w*2.0));
col=lerp(col,GlowColor,saturate(trap.x*0.5));
float ao=saturate(1.0-aoAcc*AOStrength*0.1);
col*=ao;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 L=normalize(float3(0.5,0.5,-0.7));
col*=(0.15+0.85*saturate(dot(nrm,L)));
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=GlowColor*fres*0.4;
return saturate(col);
"@

$inputNames = @("UV","Time","Power","MaxIterations","SliceOffset","RotationSpeed","Scale","BevelRadius","AOStrength","BaseColor","GlowColor","InkColor")
$inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
$r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $mandelbulbCode; description = "Mandelbulb Master"; output_type = "CMOT_Float3"; inputs = $inputObjs }
$cn = ($r | ConvertFrom-Json).expression_name
Write-Host "  Custom=$cn inputs=$(($r | ConvertFrom-Json).input_count)"

# Get expressions with retry
$exprs = $null
for ($attempt = 1; $attempt -le 5; $attempt++) {
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
Write-Host "  Wiring $($paramExprs.Count) params to $($inputNames.Count) pins..."
for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
    $r = Call-Mono "connect_expressions" @{
        asset_path = $fullPath; from_expression = $paramExprs[$i].name
        to_expression = $customNode; to_pin = $inputNames[$i]
    }
    try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR: $r" }
    Write-Host "    [$i] $($paramExprs[$i].name) -> $($inputNames[$i]) = $st"
}
Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null

# Compile
Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
Start-Sleep -Seconds 8
$r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
try {
    $s = $r | ConvertFrom-Json
    if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
    else {
        Write-Host "  FAILED" -ForegroundColor Red
        if ($s.compile_errors) {
            $errors = @($s.compile_errors)
            foreach ($e in $errors) { Write-Host "  $($e.Substring(0,[Math]::Min(300,$e.Length)))" -ForegroundColor Red }
        }
    }
} catch { Write-Host "  Parse error" -ForegroundColor Yellow }
