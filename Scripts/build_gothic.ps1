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
    Start-Sleep -Seconds 2
    $exprResult = Call-Mono "get_all_expressions" @{ asset_path = $fullPath }
    try {
        $exprs = ($exprResult | ConvertFrom-Json).expressions
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
    } catch { Write-Host "  Wiring error: $_" -ForegroundColor Red }
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 5
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        $s = $r | ConvertFrom-Json
        if ($s.is_compiled) { Write-Host "  OK: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
        else { Write-Host "  FAIL" -ForegroundColor Red; $e = @($s.compile_errors); if ($e.Count -gt 0) { $err = $e[0] -replace '\\n',' ' -replace '\\r',''; Write-Host "  $($err.Substring(0,[Math]::Min(300,$err.Length)))" -ForegroundColor Red } }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow }
}

$bg = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'

# ============================================================
# 5.1: M_SDF_GothicRoseWindow
# ============================================================
$roseCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float t=0.0;float hit=-1.0;float glassMask=0.0;float stoneMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float2 pc=p.xy;
  float r=length(pc);
  float a=atan2(pc.y,pc.x);
  float N=max(PetalCount,3.0);
  float seg=3.14159*2.0/N;
  float sa=a-floor(a/seg)*seg-seg*0.5;
  float2 sp=float2(cos(sa),sin(sa))*r;
  float outerRing=abs(r-0.9)-TraceryWidth;
  float innerRing=abs(r-0.5)-TraceryWidth*0.7;
  float hub=abs(r-0.15)-TraceryWidth*0.5;
  float spoke=abs(sp.y)-TraceryWidth*0.4;
  float archTop=length(sp-float2(0.35,0.0))-0.12;
  float trefoil1=length(sp-float2(0.25,0.1))-0.06;
  float trefoil2=length(sp-float2(0.25,-0.1))-0.06;
  float trefoil=min(min(trefoil1,trefoil2),archTop);
  float tracery=min(min(min(outerRing,innerRing),hub),min(spoke,trefoil));
  float glassShell=max(tracery,-(r-0.92));
  float glassInner=min(r-0.16,0.92-r);
  float glass=max(glassShell,-glassInner);
  float d=min(tracery,glass);
  if(d<0.001){
    hit=t;
    glassMask=(glass<tracery)?1.0:0.0;
    stoneMask=(tracery<=glass)?1.0:0.0;
    break;
  }
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
float3 col=lerp(StoneColor,GlassColor,glassMask);
float3 L=normalize(float3(0.3,0.5,-0.8));
float diff=0.2+0.8*saturate(dot(nrm,L));
col*=diff;
float backlight=pow(saturate(dot(-rd,L)),2.0);
col+=GlassColor*backlight*GlowIntensity*glassMask;
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=StoneColor*fres*0.2;
return saturate(col);
"@
Build-SDF "M_SDF_GothicRoseWindow" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="PC";class="ScalarParameter";props=@{ParameterName="PetalCount"}},
    @{id="TW";class="ScalarParameter";props=@{ParameterName="TraceryWidth"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="GC";class="VectorParameter";props=@{ParameterName="GlassColor"}},
    @{id="SC";class="VectorParameter";props=@{ParameterName="StoneColor"}}
) $roseCode @("UV","Time","PetalCount","TraceryWidth","GlowIntensity","GlassColor","StoneColor")

# ============================================================
# 5.2: M_SDF_CathedralVault
# ============================================================
$vaultCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,-1.5,-3.0);
float3 rd=normalize(float3(uvc*0.8,1.2));
float tt=Time*0.05;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float vw=max(VaultWidth,0.5);float vh=max(VaultHeight,0.5);
float t=0.0;float hit=-1.0;float ribMask=0.0;float bossMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float px=p.x/vw;float py=p.y/vh;float pz=p.z;
  float d=1e10;
  float N=max(RibCount,2.0);
  float seg=3.14159*2.0/N;
  for(int i=0;i<int(N);i++){
    float ang=float(i)*seg;
    float2 dir=float2(cos(ang),sin(ang));
    float along=dot(p.xz,dir);
    float perp=abs(p.xz.x*dir.y-p.xz.y*dir.x);
    float archH=vh*(1.0-pow(abs(along)/3.0,1.5));
    float vault=py-archH;
    float ribD=abs(perp)-RibThickness;
    float rib=max(ribD,abs(vault)-0.05);
    d=min(d,max(rib,-(abs(along)-2.5)));
    ribMask=(rib<d+0.01)?1.0:ribMask;
  }
  float bossD=length(p.xz)-KeystoneSize;
  float bossY=abs(py-vh*0.95)-KeystoneSize*0.5;
  float boss=max(bossD,bossY);
  if(boss<d){d=boss;bossMask=1.0;ribMask=0.0;}
  float ceiling=abs(py-vh*0.9)-0.03;
  if(ceiling<d&&py>0.0){d=ceiling;ribMask=0.0;bossMask=0.0;}
  float wall=min(abs(px-1.0),abs(px+1.0))-0.1;
  if(wall<d){d=wall;ribMask=0.0;bossMask=0.0;}
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
col=lerp(col,RibColor,ribMask);
col=lerp(col,GoldColor,bossMask);
float3 L=normalize(float3(0.0,1.0,-0.3));
col*=(0.15+0.85*saturate(dot(nrm,L)));
float ao=1.0-saturate(length(hp.xz)*0.3);
col*=(0.5+0.5*ao);
return saturate(col);
"@
Build-SDF "M_SDF_CathedralVault" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="VW";class="ScalarParameter";props=@{ParameterName="VaultWidth"}},
    @{id="VH";class="ScalarParameter";props=@{ParameterName="VaultHeight"}},
    @{id="RC";class="ScalarParameter";props=@{ParameterName="RibCount"}},
    @{id="RT";class="ScalarParameter";props=@{ParameterName="RibThickness"}},
    @{id="KS";class="ScalarParameter";props=@{ParameterName="KeystoneSize"}},
    @{id="SC";class="VectorParameter";props=@{ParameterName="StoneColor"}},
    @{id="RBC";class="VectorParameter";props=@{ParameterName="RibColor"}},
    @{id="GC";class="VectorParameter";props=@{ParameterName="GoldColor"}}
) $vaultCode @("UV","Time","VaultWidth","VaultHeight","RibCount","RibThickness","KeystoneSize","StoneColor","RibColor","GoldColor")

Write-Host "`n=== BATCH 1/2 COMPLETE ===" -ForegroundColor Yellow
