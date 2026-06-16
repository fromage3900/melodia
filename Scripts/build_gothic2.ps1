$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$sdfPath = "/Game/_PROJECT/04_Materials/SDF"

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

function Get-Expressions-Retry($path, $maxRetries = 3) {
    for ($attempt = 1; $attempt -le $maxRetries; $attempt++) {
        Start-Sleep -Seconds 3
        $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
        try {
            $parsed = $r | ConvertFrom-Json
            if ($parsed.expressions) { return $parsed.expressions }
        } catch { Write-Host "    get_all_expressions attempt $attempt failed, retrying..." -ForegroundColor Yellow }
    }
    return $null
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
    $exprs = Get-Expressions-Retry $fullPath
    if (-not $exprs) { Write-Host "  FATAL: Could not get expressions after retries" -ForegroundColor Red; return }
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
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 5
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  OK: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
            else { Write-Host "  FAIL" -ForegroundColor Red; $e = @($s.compile_errors); if ($e.Count -gt 0) { $err = $e[0] -replace '\\n',' ' -replace '\\r',''; Write-Host "  $($err.Substring(0,[Math]::Min(300,$err.Length)))" -ForegroundColor Red } }
        } catch { Write-Host "  Parse error: $r" -ForegroundColor Yellow }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow }
}

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'

# ============================================================
# 5.3: M_SDF_BaroqueColumn
# ============================================================
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
Build-SDF "M_SDF_BaroqueColumn" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="CR";class="ScalarParameter";props=@{ParameterName="ColumnRadius"}},
    @{id="FC";class="ScalarParameter";props=@{ParameterName="FlutingCount"}},
    @{id="CD";class="ScalarParameter";props=@{ParameterName="CapitalDetail"}},
    @{id="BC";class="VectorParameter";props=@{ParameterName="BaseColor"}},
    @{id="GC";class="VectorParameter";props=@{ParameterName="GoldColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}}
) $columnCode @("UV","Time","ColumnRadius","FlutingCount","CapitalDetail","BaseColor","GoldColor","InkColor")

# ============================================================
# 5.4: M_SDF_FlyingButtress
# ============================================================
$buttressCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.5);
float3 rd=normalize(float3(uvc*0.8,1.5));
float tt=Time*0.04;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float span=max(ArchSpan,0.5);float rise=max(ArchRise,0.3);
float t=0.0;float hit=-1.0;float traceryMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float px=p.x/span;float py=p.y;float pz=p.z;
  float halfW=Thickness*0.5;
  float archY=rise*(1.0-pow(abs(px),1.8));
  float archDist=py-archY;
  float side=abs(abs(px)-1.0)-0.15;
  float arch=max(abs(archDist)-halfW,side);
  float d=arch;
  float piers=min(abs(px-1.0),abs(px+1.0))-0.12;
  float pierH=max(-py-0.5,py-rise-0.3);
  float pier=max(piers,pierH);
  d=min(d,pier);
  float N=max(floor(TraceryDetail*4.0+2.0),2.0);
  for(int i=0;i<int(N);i++){
    float cx=lerp(-0.7,0.7,(float(i)+0.5)/N);
    float cy2=archY-0.15;
    float hole=length(float2(px-cx,py-cy2))-0.06*TraceryDetail;
    if(hole<d){d=hole;traceryMask=1.0;}
  }
  if(d<0.002){hit=t;break;}
  t+=d*0.7;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.003;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=StoneColor;
float3 L=normalize(float3(0.4,0.6,-0.7));
col*=(0.2+0.8*saturate(dot(nrm,L)));
float ao=pow(saturate(1.0-abs(hp.y)*0.3),2.0);
col*=(0.6+0.4*ao);
col=lerp(col,ShadowColor,traceryMask*0.5);
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=StoneColor*fres*0.15;
return saturate(col);
"@
Build-SDF "M_SDF_FlyingButtress" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="AS";class="ScalarParameter";props=@{ParameterName="ArchSpan"}},
    @{id="AR";class="ScalarParameter";props=@{ParameterName="ArchRise"}},
    @{id="TH";class="ScalarParameter";props=@{ParameterName="Thickness"}},
    @{id="TD";class="ScalarParameter";props=@{ParameterName="TraceryDetail"}},
    @{id="SC";class="VectorParameter";props=@{ParameterName="StoneColor"}},
    @{id="SHC";class="VectorParameter";props=@{ParameterName="ShadowColor"}}
) $buttressCode @("UV","Time","ArchSpan","ArchRise","Thickness","TraceryDetail","StoneColor","ShadowColor")

# ============================================================
# 5.5: M_SDF_GildedAltar
# ============================================================
$altarCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.5,-3.0);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time;
float aw=max(AltarWidth,0.5);
float t=0.0;float hit=-1.0;float goldMask=0.0;float candleMask=0.0;
float flameGlow=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float px=p.x/aw;float py=p.y;float pz=p.z;
  float baseTop=max(-py,0.0);
  float baseSide=abs(px)-1.0;
  float base=max(baseSide,baseTop-0.3);
  float baseFront=abs(pz)-0.4;
  base=max(base,baseFront);
  float d=base;
  if(py>-0.3&&py<0.0){goldMask=1.0;}
  float panelD=abs(pz-0.35)-0.03;
  float panelH=max(py-0.0,-(py-1.0));
  float panel=max(panelD,panelH);
  float panelSide=abs(px)-0.8;
  panel=max(panel,panelSide);
  if(panel<d){d=panel;goldMask=1.0;}
  float crossV=max(abs(px)-0.03,abs(py-0.7)-0.25);
  float crossH=max(abs(px)-0.15,abs(py-0.8)-0.03);
  float crossPiece=min(crossV,crossH);
  float crossFront=abs(pz-0.38)-0.01;
  float cross=max(crossPiece,crossFront);
  if(cross<d){d=cross;goldMask=1.0;}
  float NC=max(floor(CandleCount),1.0);
  for(int ci=0;ci<8;ci++){
    if(float(ci)>=NC)break;
    float cx=lerp(-0.6,0.6,(float(ci)+0.5)/NC);
    float candle=length(float2(px-cx,pz))-0.02;
    float candleH=max(py-0.0,-(py-0.35));
    float candleD=max(candle,candleH);
    if(candleD<d){d=candleD;candleMask=1.0;goldMask=0.0;}
    float flameH=py-0.35;
    float flicker=sin(tt*8.0+float(ci)*2.7)*0.01+sin(tt*13.0+float(ci)*4.1)*0.005;
    float flameR=0.015+flicker;
    float flame=length(float2(px-cx+flicker,pz))-flameR;
    float flameTop=max(flameH-0.08,0.0);
    float flameBottom=max(-flameH,0.0);
    float flameShape=max(flame,max(flameTop,flameBottom));
    if(flameShape<0.01){
      flameGlow+=FlameIntensity*0.3/(1.0+length(float3(px-cx,flameH,pz))*10.0);
    }
  }
  if(d<0.002){hit=t;break;}
  t+=d*0.7;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg+float3(1.0,0.7,0.3)*flameGlow*0.1;
float3 hp=ro+rd*hit;
float eps=0.003;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=CandleColor;
col=lerp(col,GoldColor,goldMask*GildingDetail);
float3 L=normalize(float3(0.0,1.0,-0.3));
col*=(0.25+0.75*saturate(dot(nrm,L)));
col+=FlameColor*flameGlow;
col+=CandleColor*candleMask*0.3;
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=GoldColor*fres*0.2*goldMask;
return saturate(col);
"@
Build-SDF "M_SDF_GildedAltar" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="AW";class="ScalarParameter";props=@{ParameterName="AltarWidth"}},
    @{id="CC";class="ScalarParameter";props=@{ParameterName="CandleCount"}},
    @{id="FI";class="ScalarParameter";props=@{ParameterName="FlameIntensity"}},
    @{id="GD";class="ScalarParameter";props=@{ParameterName="GildingDetail"}},
    @{id="GC";class="VectorParameter";props=@{ParameterName="GoldColor"}},
    @{id="CDC";class="VectorParameter";props=@{ParameterName="CandleColor"}},
    @{id="FC";class="VectorParameter";props=@{ParameterName="FlameColor"}}
) $altarCode @("UV","Time","AltarWidth","CandleCount","FlameIntensity","GildingDetail","GoldColor","CandleColor","FlameColor")

Write-Host "`n=== BATCH 2/2 COMPLETE ===" -ForegroundColor Green
