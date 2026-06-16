$ErrorActionPreference = "Continue"
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
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

function Get-Expressions-Retry($path, $maxRetries = 6) {
    for ($attempt = 1; $attempt -le $maxRetries; $attempt++) {
        Start-Sleep -Seconds 5
        $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
        try {
            $parsed = $r | ConvertFrom-Json
            if ($parsed.expressions) { return $parsed.expressions }
        } catch { Write-Host "    attempt $attempt failed..." -ForegroundColor Yellow }
    }
    return $null
}

function Build-SDF($name, $paramDefs, $hlslCode, $inputNames) {
    $fullPath = "/Game/_PROJECT/04_Materials/SDF/$name"
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    $delR = Call-Mono "delete_asset" @{ asset_path = $fullPath }
    Write-Host "  Deleted old..."
    Start-Sleep -Seconds 3
    Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    $nodes = @(); $posY = -200
    foreach ($p in $paramDefs) { $n = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }; if ($p.props) { $n.props = $p.props }; $nodes += $n; $posY += 60 }
    Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null
    Write-Host "  Graph built, settling..."
    Start-Sleep -Seconds 8
    $inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
    $r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $hlslCode; description = $name; output_type = "CMOT_Float3"; inputs = $inputObjs }
    if (-not $r) { Write-Host "  FATAL: null response" -ForegroundColor Red; return }
    $cn = ($r | ConvertFrom-Json).expression_name
    Write-Host "  Custom=$cn inputs=$(($r | ConvertFrom-Json).input_count)"
    Start-Sleep -Seconds 8
    $exprs = Get-Expressions-Retry $fullPath
    if (-not $exprs) { Write-Host "  FATAL: no expressions" -ForegroundColor Red; return }
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" }).name
    if (-not $customNode) { $customNode = $cn }
    $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
    Write-Host "  Wiring $($paramExprs.Count) params..."
    for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
        $r = Call-Mono "connect_expressions" @{
            asset_path = $fullPath; from_expression = $paramExprs[$i].name
            to_expression = $customNode; to_pin = $inputNames[$i]
        }
        try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR" }
        Write-Host "    [$i] $($paramExprs[$i].name) -> $($inputNames[$i]) = $st"
    }
    Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 8
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
            else {
                Write-Host "  COMPILE FAIL" -ForegroundColor Red
                $e = @($s.compile_errors)
                if ($e.Count -gt 0) {
                    foreach ($err in $e[0..([Math]::Min(3,$e.Count-1))]) {
                        $clean = ($err -replace '\\n',"`n" -replace '\\r','')
                        Write-Host "  $clean" -ForegroundColor Red
                    }
                }
            }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow }
    } else { Write-Host "  STATS null (server may be down)" -ForegroundColor Yellow }
    Call-Mono "save_material" @{ asset_path = $fullPath } | Out-Null
}

# First: recompile M1 (SheetMusic_Score - already wired)
Write-Host "`n=== Recompiling M_SDF_SheetMusic_Score ===" -ForegroundColor Cyan
$m1Path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_SheetMusic_Score"
Call-Mono "recompile_material" @{ asset_path = $m1Path } | Out-Null
Start-Sleep -Seconds 8
$r = Call-Mono "get_compilation_stats" @{ asset_path = $m1Path }
if ($r) {
    try {
        $s = $r | ConvertFrom-Json
        if ($s.is_compiled) { Write-Host "  M1 COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
        else {
            Write-Host "  M1 FAIL" -ForegroundColor Red
            $e = @($s.compile_errors)
            if ($e.Count -gt 0) { Write-Host "  $($e[0].Substring(0,[Math]::Min(300,$e[0].Length)))" -ForegroundColor Red }
        }
    } catch { Write-Host "  M1 parse error" -ForegroundColor Yellow }
} else { Write-Host "  M1 STATS null" -ForegroundColor Yellow }
Call-Mono "save_material" @{ asset_path = $m1Path } | Out-Null

# M2: M_SDF_TrebleClef_Ornament
$clefCode = @'
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.0);
float3 rd=normalize(float3(uvc*0.85,1.5));
float tt=Time*0.12;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;
float ornHit=0.0;
[loop]
for(int step=0;step<90;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  float bodyY=p.y;
  float spiralPhase=bodyY*4.5+0.5;
  float spiralR=0.12+0.08*sin(spiralPhase*2.0);
  float mainBody=length(float2(p.x-spiralR*sin(spiralPhase*3.0),p.z))-spiralR;
  float bodyCap=max(-bodyY-0.9,bodyY-0.95);
  mainBody=max(mainBody,bodyCap);
  if(mainBody<d){d=mainBody;ornHit=0.0;}
  float topCurl=length(float2(p.x-0.08,p.y-0.85))-0.06;
  float topCurl2=length(float2(p.x+0.05,p.y-0.92))-0.035;
  float topOrn=min(topCurl,topCurl2);
  topOrn=max(topOrn,abs(p.z)-0.025);
  if(topOrn<d){d=topOrn;ornHit=1.0;}
  float botCurl=length(float2(p.x+0.06,p.y+0.8))-0.05;
  float botDrop=length(float2(p.x,p.y+0.92))-0.03;
  float botOrn=min(botCurl,botDrop);
  botOrn=max(botOrn,abs(p.z)-0.02);
  if(botOrn<d){d=botOrn;ornHit=1.0;}
  float stem=abs(p.x+0.01)-0.012;
  float stemY=max(-p.y-0.85,p.y-0.9);
  stem=max(stem,stemY);
  stem=max(stem,abs(p.z)-0.012);
  if(stem<d){d=stem;ornHit=0.0;}
  float dotTop=length(float3(p.x-0.04,p.y-0.95,p.z))-0.025;
  float dotBot=length(float3(p.x+0.02,p.y+0.9,p.z))-0.02;
  float dots=min(dotTop,dotBot);
  if(dots<d){d=dots;ornHit=1.0;}
  float baseH=abs(p.y+1.05)-0.04;
  float baseR=length(float2(p.x,p.z))-0.15;
  float base=max(baseH,baseR);
  float baseRim=length(float2(p.x,p.z))-0.18;
  baseRim=max(baseRim,abs(p.y+1.02)-0.015);
  base=min(base,baseRim);
  if(base<d){d=base;ornHit=1.0;}
  if(d<0.003){hit=t;break;}
  t+=d*0.7;if(t>20.0)break;
}
float3 bg=lerp(float3(0.04,0.02,0.1),float3(0.01,0.005,0.04),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(float3(hp.x+eps,hp.y,hp.z))-length(float3(hp.x-eps,hp.y,hp.z)),
  length(float3(hp.x,hp.y+eps,hp.z))-length(float3(hp.x,hp.y-eps,hp.z)),
  length(float3(hp.x,hp.y,hp.z+eps))-length(float3(hp.x,hp.y,hp.z-eps))
));
nrm=normalize(nrm);
float3 goldCol=BaseColor;
float3 ornCol=AccentColor;
float3 col=lerp(goldCol,ornCol,ornHit);
float3 L=normalize(float3(0.4,0.7,-0.5));
float diff=saturate(dot(nrm,L));
float3 H=normalize(L+normalize(-rd));
float spec=pow(saturate(dot(nrm,H)),Metallic*80.0+20.0);
float fresnel=pow(1.0-saturate(dot(nrm,normalize(-rd))),3.0);
col=col*(0.25+0.75*diff)+spec*GlowIntensity*0.5;
col+=fresnel*AccentColor*GlowIntensity*0.3;
return saturate(col);
'@
Build-SDF "M_SDF_TrebleClef_Ornament" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="BC";class="VectorParameter";props=@{ParameterName="BaseColor"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}},
    @{id="ME";class="ScalarParameter";props=@{ParameterName="Metallic"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}}
) $clefCode @("UV","Time","BaseColor","AccentColor","Metallic","GlowIntensity")

Write-Host "`n=== M1 RECOMPILE + M2 COMPLETE ===" -ForegroundColor Yellow
