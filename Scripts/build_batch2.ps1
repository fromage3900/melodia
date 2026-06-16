$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$sdfPath = "/Game/_PROJECT/04_Materials/SDF"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{ jsonrpc = "2.0"; id = 1; method = "tools/call"; params = @{ name = "material_query"; arguments = $arguments } } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 90
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "  NET_ERROR" -ForegroundColor Red; return $null }
}

function Build-SDF($name, $paramDefs, $hlslCode, $inputNames) {
    $fullPath = "$sdfPath/$name"
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    $nodes = @(); $posY = -200
    foreach ($p in $paramDefs) { $n = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }; if ($p.props) { $n.props = $p.props }; $nodes += $n; $posY += 60 }
    Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null
    $inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
    $r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $hlslCode; description = $name; output_type = "CMOT_Float3"; inputs = $inputObjs }
    $cn = ($r | ConvertFrom-Json).expression_name
    Write-Host "  Custom=$cn inputs=$(($r | ConvertFrom-Json).input_count)"
    # Get expressions
    $exprResult = Call-Mono "get_all_expressions" @{ asset_path = $fullPath }
    try {
        $exprs = ($exprResult | ConvertFrom-Json).expressions
        $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" }).name
        if (-not $customNode) { $customNode = $cn }
        $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
        # Wire each param to SPECIFIC input pin using to_pin
        Write-Host "  Wiring $($paramExprs.Count) params to $($inputNames.Count) pins..."
        for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
            $r = Call-Mono "connect_expressions" @{
                asset_path = $fullPath
                from_expression = $paramExprs[$i].name
                to_expression = $customNode
                to_pin = $inputNames[$i]
            }
            $connResult = $r | ConvertFrom-Json
            $status = if ($connResult.connected) { "OK" } else { "FAIL" }
            Write-Host "    [$i] $($paramExprs[$i].name) -> $($inputNames[$i]) = $status"
        }
        Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    } catch { Write-Host "  Wiring error: $_" -ForegroundColor Red }
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 5
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        $s = $r | ConvertFrom-Json
        if ($s.is_compiled) { Write-Host "  OK: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
        else { Write-Host "  FAIL" -ForegroundColor Red; $e = @($s.compile_errors); if ($e.Count -gt 0) { $err = $e[0] -replace '\\n',' ' -replace '\\r',''; Write-Host "  $($err.Substring(0,[Math]::Min(250,$err.Length)))" -ForegroundColor Red } }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow }
}

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'

# ============================================================
# 3: M_SDF_SierpinskiTetrahedron
# ============================================================
$sierpinskiCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.0);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*RotationSpeed;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float Scl=max(Scale,0.5);
float t=0.0;float hit=-1.0;float depthFrac=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=(ro+rd*t)*Scl;
  float sc=1.0;float d=1e10;
  [loop]
  for(int i=0;i<int(IterationDepth);i++){
    p=abs(p)-float3(1.0,1.0,1.0)*sc;
    if(p.x<p.y){float tmp=p.x;p.x=p.y;p.y=tmp;}
    if(p.x<p.z){float tmp=p.x;p.x=p.z;p.z=tmp;}
    if(p.y<p.z){float tmp=p.y;p.y=p.z;p.z=tmp;}
    p-=float3(1.0,1.0,1.0)*sc*(Scl-1.0);
    sc*=Scl;
    d=min(d,(length(max(abs(p)-float3(1,1,1)*sc,0.0))-0.1*sc)/sc);
  }
  if(d<0.001){hit=t;depthFrac=float(step)/80.0;break;}
  t+=d*0.8;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=lerp(EdgeColor,BaseColor,depthFrac);
col+=EdgeColor*GlowIntensity*(1.0-depthFrac);
float3 L=normalize(float3(0.5,0.5,-0.7));
col*=(0.15+0.85*saturate(dot(nrm,L)));
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=EdgeColor*fres*0.4;
col=lerp(InkColor,col,saturate(depthFrac*2.0+0.2));
return saturate(col);
"@
Build-SDF "M_SDF_SierpinskiTetrahedron" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="ID";class="ScalarParameter";props=@{ParameterName="IterationDepth"}},
    @{id="SC";class="ScalarParameter";props=@{ParameterName="Scale"}},
    @{id="RS";class="ScalarParameter";props=@{ParameterName="RotationSpeed"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="BC";class="VectorParameter";props=@{ParameterName="BaseColor"}},
    @{id="EC";class="VectorParameter";props=@{ParameterName="EdgeColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}}
) $sierpinskiCode @("UV","Time","IterationDepth","Scale","RotationSpeed","GlowIntensity","BaseColor","EdgeColor","InkColor")

# ============================================================
# 4: M_SDF_FractalOrnament
# ============================================================
$ornamentCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*0.1;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;float ornMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float d=1e10;float sc=1.0;float ang=0.0;
  [loop]
  for(int i=0;i<int(RecursionDepth);i++){
    float r=length(p.xy);
    float a=atan2(p.y,p.x)+ang;
    float petals=6.0;
    float pa=a-floor(a/(3.14159*2.0/petals))*(3.14159*2.0/petals);
    float2 pp=float2(cos(pa),sin(pa))*r;
    float ring=abs(r-0.4*sc)-BevelRadius*sc;
    float petal=length(pp-float2(0.35*sc,0.0))-0.08*sc;
    float shape=min(ring,petal);
    d=min(d,shape);
    ornMask=max(ornMask,1.0-float(i)/max(RecursionDepth,1.0));
    p*=0.618;ang+=RotationPerLevel*3.14159/180.0;sc*=0.618;
  }
  if(d<0.001){hit=t;break;}
  t+=d*0.8;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(length(abs(hp+float3(eps,0,0))-0.3)-length(abs(hp-float3(eps,0,0))-0.3),length(abs(hp+float3(0,eps,0))-0.3)-length(abs(hp-float3(0,eps,0))-0.3),length(abs(hp+float3(0,0,eps))-0.3)-length(abs(hp-float3(0,0,eps))-0.3)));
float3 col=lerp(InkColor,BaseColor,ornMask);
col+=GoldColor*ornMask*Detail;
float3 L=normalize(float3(0.5,0.5,-0.7));
col*=(0.2+0.8*saturate(dot(nrm,L)));
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=GoldColor*fres*0.3;
return saturate(col);
"@
Build-SDF "M_SDF_FractalOrnament" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="RD";class="ScalarParameter";props=@{ParameterName="RecursionDepth"}},
    @{id="RPL";class="ScalarParameter";props=@{ParameterName="RotationPerLevel"}},
    @{id="BR";class="ScalarParameter";props=@{ParameterName="BevelRadius"}},
    @{id="DT";class="ScalarParameter";props=@{ParameterName="Detail"}},
    @{id="BC";class="VectorParameter";props=@{ParameterName="BaseColor"}},
    @{id="GC";class="VectorParameter";props=@{ParameterName="GoldColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}}
) $ornamentCode @("UV","Time","RecursionDepth","RotationPerLevel","BevelRadius","Detail","BaseColor","GoldColor","InkColor")

# ============================================================
# 5: M_SDF_MengerSponge
# ============================================================
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
Build-SDF "M_SDF_MengerSponge" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="IT";class="ScalarParameter";props=@{ParameterName="Iterations"}},
    @{id="BR";class="ScalarParameter";props=@{ParameterName="BevelRadius"}},
    @{id="RS";class="ScalarParameter";props=@{ParameterName="RotationSpeed"}},
    @{id="BC";class="VectorParameter";props=@{ParameterName="BaseColor"}},
    @{id="IC2";class="VectorParameter";props=@{ParameterName="InteriorColor"}},
    @{id="EC";class="VectorParameter";props=@{ParameterName="EdgeColor"}}
) $mengerCode @("UV","Time","Iterations","BevelRadius","RotationSpeed","BaseColor","InteriorColor","EdgeColor")

Write-Host "`n=== BATCH COMPLETE ===" -ForegroundColor Green
