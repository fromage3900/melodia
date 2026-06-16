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
    } catch { Write-Host "  NET: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

function Get-Expr($path) {
    for ($attempt = 1; $attempt -le 8; $attempt++) {
        Start-Sleep -Seconds 5
        $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
        if (-not $r) { continue }
        try {
            $parsed = $r | ConvertFrom-Json
            if ($parsed.expressions -and $parsed.expressions.Count -gt 0) { return $parsed.expressions }
        } catch {}
        $er = Call-Mono "export_material_graph" @{ asset_path = $path }
        if ($er) {
            try {
                $exp = $er | ConvertFrom-Json
                if ($exp.nodes -and $exp.nodes.Count -gt 0) {
                    $exprs = @()
                    foreach ($n in $exp.nodes) { $exprs += @{ name = $n.name; class = $n.class } }
                    foreach ($n in $exp.custom_hlsl_nodes) { $exprs += @{ name = $n.name; class = "MaterialExpressionCustom" } }
                    return $exprs
                }
            } catch {}
        }
        Write-Host "    expr attempt $attempt failed" -ForegroundColor Yellow
    }
    return $null
}

function Build-SDF($name, $paramDefs, $hlslCode, $inputNames) {
    $fullPath = "/Game/_PROJECT/04_Materials/SDF/$name"
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    Call-Mono "delete_asset" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 4
    Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    Start-Sleep -Seconds 3
    $nodes = @(); $posY = -200
    foreach ($p in $paramDefs) { $n = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }; if ($p.props) { $n.props = $p.props }; $nodes += $n; $posY += 60 }
    Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null
    Write-Host "  Graph built, waiting 12s..."
    Start-Sleep -Seconds 12
    $inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
    $r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $hlslCode; description = $name; output_type = "CMOT_Float3"; inputs = $inputObjs }
    if (-not $r) { Write-Host "  FATAL: null" -ForegroundColor Red; return $false }
    $cj = $r | ConvertFrom-Json
    $cn = $cj.expression_name
    Write-Host "  Custom=$cn inputs=$($cj.input_count)"
    Start-Sleep -Seconds 12
    $exprs = Get-Expr $fullPath
    if (-not $exprs) { Write-Host "  FATAL: no expressions" -ForegroundColor Red; return $false }
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" } | Select-Object -First 1).name
    if (-not $customNode) { $customNode = $cn }
    $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
    Write-Host "  Got $($paramExprs.Count) params, wiring..."
    for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
        $pn = $paramExprs[$i].name
        if (-not $pn) { $pn = "$($paramExprs[$i].class)_$i" }
        $r = Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $pn; to_expression = $customNode; to_pin = $inputNames[$i] }
        try { $cr = $r | ConvertFrom-Json; $st = if ($cr.connected) { "OK" } else { "FAIL" } } catch { $st = "ERR" }
        Write-Host "    [$i] $pn -> $($inputNames[$i]) = $st"
        Start-Sleep -Seconds 1
    }
    Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    Start-Sleep -Seconds 2
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Write-Host "  Compiling..."
    Start-Sleep -Seconds 12
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green; Call-Mono "save_material" @{ asset_path = $fullPath } | Out-Null; return $true }
            else {
                Write-Host "  COMPILE FAIL" -ForegroundColor Red
                $e = @($s.compile_errors)
                if ($e.Count -gt 0) { Write-Host "  $($e[0].Substring(0,[Math]::Min(400,$e[0].Length)))" -ForegroundColor Red }
                return $false
            }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow; return $false }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow; return $false }
}

# Recompile M3
Write-Host "=== Recompile M3 ===" -ForegroundColor Cyan
Call-Mono "recompile_material" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" } | Out-Null
Start-Sleep -Seconds 12
$r = Call-Mono "get_compilation_stats" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" }
if ($r) {
    try { $s = $r | ConvertFrom-Json; Write-Host "M3: compiled=$($s.is_compiled) PS=$($s.num_pixel_shader_instructions)" -ForegroundColor $(if($s.is_compiled){'Green'}else{'Red'}) } catch { Write-Host "M3 parse error" }
}
Call-Mono "save_material" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" } | Out-Null

# M5: VinylRecord
$vinylCode = @'
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.5,-2.5);
float3 rd=normalize(float3(uvc*0.85,1.2));
float rotAngle=Time*0.25;
float rc=cos(rotAngle),rs=sin(rotAngle);
float t=0.0;float hit=-1.0;
float grooveHit=0.0;float labelHit=0.0;float armHit=0.0;float spindleHit=0.0;
[loop]
for(int step=0;step<90;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  float2 rp=float2(p.x,p.z);
  float2 rotRp=float2(rc*rp.x+rs*rp.y,-rs*rp.x+rc*rp.y);
  float discH=abs(p.y)-0.015;
  float discR=length(rotRp)-1.0;
  float disc=max(discH,discR);
  if(disc<d){d=disc;grooveHit=0.0;labelHit=0.0;spindleHit=0.0;}
  float grooveR=length(rotRp);
  float grooveBump=abs(frac(grooveR*180.0)-0.5)-0.15;
  grooveBump=grooveBump*0.003;
  float grooveDisc=abs(p.y+0.015+grooveBump)-0.001;
  float grooveInner=max(grooveDisc,grooveR-0.95);
  grooveInner=max(grooveInner,0.15-grooveR);
  if(grooveInner<d){d=grooveInner;grooveHit=1.0;labelHit=0.0;spindleHit=0.0;}
  float labelH=abs(p.y)-0.016;
  float labelR=length(rotRp)-0.14;
  float label=max(labelH,labelR);
  if(label<d){d=label;labelHit=1.0;grooveHit=0.0;spindleHit=0.0;}
  float labelRing=abs(length(rotRp)-0.12)-0.003;
  labelRing=max(labelRing,abs(p.y)-0.017);
  if(labelRing<d){d=labelRing;labelHit=1.0;grooveHit=0.0;spindleHit=0.0;}
  float labelLine1=abs(rotRp.x)-0.001;
  float labelLine1Y=max(-rotRp.y+0.02,rotRp.y-0.08);
  labelLine1=max(labelLine1,labelLine1Y);
  labelLine1=max(labelLine1,abs(p.y)-0.017);
  float labelLine2=abs(rotRp.x+0.04)-0.001;
  float labelLine2Y=max(-rotRp.y+0.03,rotRp.y-0.07);
  labelLine2=max(labelLine2,labelLine2Y);
  labelLine2=max(labelLine2,abs(p.y)-0.017);
  float labelLines=min(labelLine1,labelLine2);
  if(labelLines<d){d=labelLines;labelHit=1.0;grooveHit=0.0;spindleHit=0.0;}
  float spindleH=abs(p.y)-0.03;
  float spindleR=length(float2(p.x,p.z))-0.008;
  float spindle=max(spindleH,spindleR);
  float spindleHole=length(float2(p.x,p.z))-0.005;
  float spindleHoleH=max(-p.y-0.01,p.y-0.04);
  spindleHole=max(spindleHole,spindleHoleH);
  spindle=min(spindle,-spindleHole);
  if(spindle<d){d=spindle;spindleHit=1.0;grooveHit=0.0;labelHit=0.0;}
  float armBaseX=1.1;float armBaseZ=-0.8;
  float armBaseH=abs(p.y-0.04)-0.015;
  float armBaseR=length(float2(p.x-armBaseX,p.z-armBaseZ))-0.04;
  float armBase=max(armBaseH,armBaseR);
  float armPivotY=0.055;
  float armAngle=0.35+sin(Time*0.1)*0.02;
  float2 armDir=float2(-cos(armAngle),sin(armAngle));
  float2 armPos=float2(p.x-armBaseX,p.z-armBaseZ);
  float armProj=dot(armPos,armDir);
  float armPerp=length(armPos-armDir*armProj);
  float armBar=armPerp-0.008;
  float armLen=max(-armProj,armProj-1.5);
  armBar=max(armBar,armLen);
  float armBarH=abs(p.y-armPivotY)-0.006;
  armBar=max(armBar,armBarH);
  float arm=min(armBase,armBar);
  float headShell=length(float2(armProj-1.45,armPerp))-0.02;
  headShell=max(headShell,abs(p.y-armPivotY)-0.01);
  arm=min(arm,headShell);
  float counterW=length(float2(armProj+0.12,armPerp))-0.025;
  counterW=max(counterW,abs(p.y-armPivotY)-0.012);
  arm=min(arm,counterW);
  if(arm<d){d=arm;armHit=1.0;grooveHit=0.0;labelHit=0.0;spindleHit=0.0;}
  if(d<0.002){hit=t;break;}
  t+=d*0.6;if(t>25.0)break;
}
float3 bg=lerp(float3(0.04,0.02,0.08),float3(0.01,0.005,0.03),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.001;
float3 nrm=normalize(float3(
  length(float3(hp.x+eps,hp.y,hp.z))-length(float3(hp.x-eps,hp.y,hp.z)),
  length(float3(hp.x,hp.y+eps,hp.z))-length(float3(hp.x,hp.y-eps,hp.z)),
  length(float3(hp.x,hp.y,hp.z+eps))-length(float3(hp.x,hp.y,hp.z-eps))
));
nrm=normalize(nrm);
float2 rp2=float2(hp.x,hp.z);
float grooveR2=length(rp2);
float microGroove=abs(sin(grooveR2*600.0+Time*2.0))*0.03;
nrm.x+=microGroove;
nrm=normalize(nrm);
float3 vinylBlack=float3(0.02,0.02,0.025);
float3 labelCol=LabelColor;
float3 armCol=ArmColor;
float3 chromeCol=float3(0.7,0.7,0.75);
float3 col=vinylBlack;
col=lerp(col,labelCol,labelHit);
col=lerp(col,armCol,armHit);
col=lerp(col,chromeCol,spindleHit);
float3 L=normalize(float3(0.3,0.8,-0.5));
float diff=saturate(dot(nrm,L));
float3 H=normalize(L+normalize(-rd));
float spec=pow(saturate(dot(nrm,H)),60.0);
float fresnel=pow(1.0-saturate(dot(nrm,normalize(-rd))),4.0);
float grooveSheen=grooveHit*pow(saturate(dot(reflect(-L,nrm),normalize(-rd))),8.0)*0.6;
float3 refDir=reflect(rd,nrm);
float envRef=0.5+0.5*sin(refDir.x*5.0+refDir.y*3.0+Time*0.5);
col=col*(0.2+0.8*diff)+spec*GlowIntensity*0.3;
col+=grooveSheen*float3(0.8,0.85,1.0)*GlowIntensity;
col+=fresnel*float3(0.15,0.12,0.2)*GlowIntensity*0.5;
col+=envRef*0.02*grooveHit;
return saturate(col);
'@
$ok5 = Build-SDF "M_SDF_VinylRecord" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="LC";class="VectorParameter";props=@{ParameterName="LabelColor"}},
    @{id="AR";class="VectorParameter";props=@{ParameterName="ArmColor"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}}
) $vinylCode @("UV","Time","LabelColor","ArmColor","GlowIntensity")
Write-Host "M5 result: $(if($ok5){'OK'}else{'FAIL'})" -ForegroundColor $(if($ok5){'Green'}else{'Red'})
