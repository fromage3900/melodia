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

function Get-Expressions-Retry($path, $maxRetries = 4) {
    for ($attempt = 1; $attempt -le $maxRetries; $attempt++) {
        Start-Sleep -Seconds 4
        $r = Call-Mono "get_all_expressions" @{ asset_path = $path }
        try {
            $parsed = $r | ConvertFrom-Json
            if ($parsed.expressions) { return $parsed.expressions }
        } catch { Write-Host "    get_all_expressions attempt $attempt failed..." -ForegroundColor Yellow }
    }
    return $null
}

function Build-SDF($name, $paramDefs, $hlslCode, $inputNames) {
    $fullPath = "$sdfPath/$name"
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    $delR = Call-Mono "delete_asset" @{ asset_path = $fullPath }
    Start-Sleep -Seconds 2
    Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    $nodes = @(); $posY = -200
    foreach ($p in $paramDefs) { $n = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }; if ($p.props) { $n.props = $p.props }; $nodes += $n; $posY += 60 }
    Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } } | Out-Null
    $inputObjs = @(); foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
    $r = Call-Mono "create_custom_hlsl_node" @{ asset_path = $fullPath; code = $hlslCode; description = $name; output_type = "CMOT_Float3"; inputs = $inputObjs }
    if (-not $r) { Write-Host "  FATAL: create_custom_hlsl_node returned null" -ForegroundColor Red; return }
    $cn = ($r | ConvertFrom-Json).expression_name
    Write-Host "  Custom=$cn inputs=$(($r | ConvertFrom-Json).input_count)"
    $exprs = Get-Expressions-Retry $fullPath
    if (-not $exprs) { Write-Host "  FATAL: Could not get expressions" -ForegroundColor Red; return }
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
    Start-Sleep -Seconds 6
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
            else {
                Write-Host "  COMPILE FAIL" -ForegroundColor Red
                $e = @($s.compile_errors)
                if ($e.Count -gt 0) {
                    foreach ($err in $e[0..([Math]::Min(2,$e.Count-1))]) {
                        $clean = ($err -replace '\\n',"`n" -replace '\\r','')
                        Write-Host "  $clean" -ForegroundColor Red
                    }
                }
            }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow }
    Call-Mono "save_material" @{ asset_path = $fullPath } | Out-Null
}

# ============================================================
# M4: M_SDF_GrandStaff_CrossSection - Grand piano staff with measures
# ============================================================
$grandCode = @'
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.2);
float3 rd=normalize(float3(uvc*0.8,1.5));
float tt=Time*0.05;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;
float noteHit=0.0;float staffHit=0.0;float braceHit=0.0;float ledgerHit=0.0;
[loop]
for(int step=0;step<85;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  float paper=abs(p.z)-0.015;
  float paperEdge=max(abs(p.x)-1.5,abs(p.y)-1.1);
  paper=max(paper,paperEdge);
  if(paper<d){d=paper;noteHit=0.0;staffHit=0.0;braceHit=0.0;ledgerHit=0.0;}
  for(int st=0;st<2;st++){
    float staffY=lerp(0.35,-0.35,float(st));
    [unroll]
    for(int s=0;s<5;s++){
      float ly=staffY+lerp(-0.12,0.12,float(s)/4.0);
      float ln=abs(p.y-ly)-0.003;
      float lnX=max(-p.x-1.3,p.x-1.3);
      ln=max(ln,lnX);
      if(ln<d){d=ln;staffHit=1.0;noteHit=0.0;braceHit=0.0;ledgerHit=0.0;}
    }
    for(int bi=0;bi<4;bi++){
      float barX=lerp(-1.2,1.2,float(bi)/3.0);
      float br=abs(p.x-barX)-0.004;
      float brY=max(-(p.y-staffY+0.12),p.y-staffY-0.12);
      br=max(br,brY);
      if(br<d){d=br;staffHit=1.0;noteHit=0.0;braceHit=0.0;ledgerHit=0.0;}
    }
  }
  float braceX=abs(p.x+1.28)-0.015;
  float braceY=max(-p.y-0.45,p.y-0.45);
  float brace=max(braceX,braceY);
  float braceCurve=length(float2(p.x+1.25,p.y))-0.08;
  braceCurve=max(braceCurve,abs(-p.x-1.28)-0.1);
  braceCurve=max(braceCurve,abs(p.y)-0.35);
  brace=min(brace,braceCurve);
  brace=max(brace,abs(p.z)-0.01);
  if(brace<d){d=brace;braceHit=1.0;noteHit=0.0;staffHit=0.0;ledgerHit=0.0;}
  for(int ni=0;ni<14;ni++){
    float fi=float(ni);
    int whichStaff=int(floor(fmod(fi,2.0)));
    float staffY=whichStaff==0?0.35:-0.35;
    int noteInStaff=ni/2;
    float nx=lerp(-1.0,1.1,float(noteInStaff)/6.0);
    float ny=staffY+sin(fi*2.1+0.5)*0.1;
    float2 nc=float2(p.x-nx,p.y-ny);
    float ang=0.25;
    float2 nr=float2(cos(ang)*nc.x+sin(ang)*nc.y,-sin(ang)*nc.x+cos(ang)*nc.y);
    float head=length(nr/float2(0.05,0.035))-1.0;
    float headZ=abs(p.z)-0.008;
    float nt=max(head,headZ);
    float stem=abs(p.x-nx-0.04)-0.003;
    float stemY=max(-(p.y-ny),p.y-ny-0.2);
    stem=max(stem,stemY);
    stem=max(stem,abs(p.z)-0.004);
    nt=min(nt,stem);
    if(ni%4==1){
      float flagT=saturate((p.y-ny-0.15)/0.07);
      float flagX=abs(p.x-nx-0.04-0.035*sin(flagT*3.14159))-0.003;
      float flagY=max(-(p.y-ny-0.15),p.y-ny-0.22);
      float flag=max(flagX,flagY);
      flag=max(flag,abs(p.z)-0.003);
      nt=min(nt,flag);
    }
    if(nt<d){d=nt;noteHit=1.0;staffHit=0.0;braceHit=0.0;ledgerHit=0.0;}
  }
  for(int li=0;li<3;li++){
    float ly1=0.35+0.12+float(li)*0.06;
    float ly2=-0.35-0.12-float(li)*0.06;
    float ledge1=abs(p.y-ly1)-0.003;
    float ledge1X=max(-p.x+0.2,p.x-0.5);
    ledge1=max(ledge1,ledge1X);
    float ledge2=abs(p.y-ly2)-0.003;
    float ledge2X=max(-p.x+0.0,p.x-0.3);
    ledge2=max(ledge2,ledge2X);
    float ledge=min(ledge1,ledge2);
    if(ledge<d){d=ledge;ledgerHit=1.0;noteHit=0.0;staffHit=0.0;braceHit=0.0;}
  }
  float crescX=max(-p.x+0.6,p.x-1.0);
  float crescW=0.003+0.015*saturate((p.x-0.6)/0.4);
  float crescTop=abs(p.y+0.5-crescW)-0.002;
  float crescBot=abs(p.y+0.5+crescW)-0.002;
  float cresc=min(crescTop,crescBot);
  cresc=max(cresc,crescX);
  cresc=max(cresc,abs(p.z)-0.003);
  if(cresc<d){d=cresc;staffHit=1.0;noteHit=0.0;braceHit=0.0;ledgerHit=0.0;}
  if(d<0.002){hit=t;break;}
  t+=d*0.65;if(t>20.0)break;
}
float3 bg=lerp(float3(0.05,0.03,0.1),float3(0.015,0.008,0.04),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 col=PaperColor;
col=lerp(col,InkColor,staffHit);
col=lerp(col,NoteColor,noteHit);
col=lerp(col,AccentColor,braceHit);
col=lerp(col,InkColor*0.7,ledgerHit);
float3 L=normalize(float3(0.3,0.6,-0.7));
col*=(0.3+0.7*saturate(dot(nrm,L)));
float glow=noteHit*GlowIntensity*pow(saturate(1.0-abs(hp.z)*5.0),2.0);
col+=NoteColor*glow;
return saturate(col);
'@
Build-SDF "M_SDF_GrandStaff_CrossSection" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="PC";class="VectorParameter";props=@{ParameterName="PaperColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}},
    @{id="NC";class="VectorParameter";props=@{ParameterName="NoteColor"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}}
) $grandCode @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")

# ============================================================
# M5: M_SDF_VinylRecord - Detailed vinyl record with grooves and tonearm
# ============================================================
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
  float grooveCount=floor(grooveR*180.0);
  float groovePhase=frac(grooveR*180.0)-0.5;
  float grooveDepth=abs(groovePhase)-0.15;
  float grooveBump=grooveDepth*0.003;
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
Build-SDF "M_SDF_VinylRecord" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="LC";class="VectorParameter";props=@{ParameterName="LabelColor"}},
    @{id="AR";class="VectorParameter";props=@{ParameterName="ArmColor"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}}
) $vinylCode @("UV","Time","LabelColor","ArmColor","GlowIntensity")

Write-Host "`n=== M4+M5 COMPLETE ===" -ForegroundColor Yellow
