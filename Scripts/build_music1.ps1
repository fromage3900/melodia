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
    # Delete if exists
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
# M1: M_SDF_SheetMusic_Score - Sheet music page with staff, notes, clef
# ============================================================
$sheetCode = @'
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.8);
float3 rd=normalize(float3(uvc*0.8,1.5));
float tt=Time*0.06;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;
float noteHit=0.0;float staffHit=0.0;float clefHit=0.0;float beamHit=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  float paper=abs(p.z)-0.02;
  float paperEdge=max(abs(p.x)-1.4,abs(p.y)-0.9);
  paper=max(paper,paperEdge);
  if(paper<d){d=paper;noteHit=0.0;staffHit=0.0;clefHit=0.0;beamHit=0.0;}
  [unroll]
  for(int s=0;s<5;s++){
    float ly=lerp(-0.3,0.3,float(s)/4.0);
    float ln=abs(p.y-ly)-0.004;
    float lnX=max(-p.x-1.2,p.x-1.2);
    ln=max(ln,lnX);
    if(ln<d){d=ln;staffHit=1.0;noteHit=0.0;clefHit=0.0;beamHit=0.0;}
  }
  float bar1=abs(p.x-0.5)-0.005;float bar2=abs(p.x+0.5)-0.005;
  float bar3=abs(p.x-1.15)-0.006;
  float br=min(min(bar1,bar2),bar3);
  float brY=max(-p.y-0.3,p.y-0.3);
  br=max(br,brY);
  if(br<d){d=br;staffHit=1.0;noteHit=0.0;clefHit=0.0;beamHit=0.0;}
  for(int ni=0;ni<10;ni++){
    float frac=float(ni)/9.0;
    float nx=lerp(-0.9,0.95,frac);
    float nh=sin(float(ni)*1.7+0.3)*0.25;
    float2 nc=float2(p.x-nx,p.y-nh);
    float ang=0.3;
    float2 nr=float2(cos(ang)*nc.x+sin(ang)*nc.y,-sin(ang)*nc.x+cos(ang)*nc.y);
    float head=length(nr/float2(0.06,0.04))-1.0;
    float headZ=abs(p.z)-0.01;
    float nt=max(head,headZ);
    float stem=abs(p.x-nx-0.05)-0.003;
    float stemY=max(-(p.y-nh),p.y-nh-0.25);
    stem=max(stem,stemY);
    stem=max(stem,abs(p.z)-0.005);
    nt=min(nt,stem);
    if(ni%3==1){
      float flagT=saturate((p.y-nh-0.2)/0.08);
      float flagX=abs(p.x-nx-0.05-0.04*sin(flagT*3.14159))-0.003;
      float flagY=max(-(p.y-nh-0.2),p.y-nh-0.28);
      float flag=max(flagX,flagY);
      flag=max(flag,abs(p.z)-0.004);
      nt=min(nt,flag);
    }
    if(nt<d){d=nt;noteHit=1.0;staffHit=0.0;clefHit=0.0;beamHit=0.0;}
  }
  float cx=-1.1;float clY=p.y;
  float spiralR=0.08+0.04*sin(clY*15.0+1.0);
  float clefD=length(float2(p.x-cx,clY))-spiralR;
  float clefStem=abs(p.x-cx+0.01)-0.005;
  float clefStemY=max(-clY-0.35,clY-0.4);
  clefStem=max(clefStem,clefStemY);
  float clefTop=length(float2(p.x-cx+0.02,clY-0.38))-0.02;
  float clefBot=length(float2(p.x-cx,clY+0.32))-0.015;
  float clef=min(min(clefD,clefStem),min(clefTop,clefBot));
  clef=max(clef,abs(p.z)-0.008);
  if(clef<d){d=clef;clefHit=1.0;noteHit=0.0;staffHit=0.0;beamHit=0.0;}
  if(d<0.002){hit=t;break;}
  t+=d*0.7;if(t>20.0)break;
}
float3 bg=lerp(float3(0.06,0.04,0.12),float3(0.02,0.01,0.06),saturate(uvc.y*0.5+0.5));
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
col=lerp(col,AccentColor,clefHit);
float3 L=normalize(float3(0.3,0.6,-0.7));
col*=(0.3+0.7*saturate(dot(nrm,L)));
float shadow=1.0-saturate(abs(hp.z)*3.0)*0.2;
col*=shadow;
float glow=noteHit*GlowIntensity*pow(saturate(1.0-abs(hp.z)*5.0),2.0);
col+=NoteColor*glow;
return saturate(col);
'@
Build-SDF "M_SDF_SheetMusic_Score" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="PC";class="VectorParameter";props=@{ParameterName="PaperColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}},
    @{id="NC";class="VectorParameter";props=@{ParameterName="NoteColor"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}}
) $sheetCode @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")

# ============================================================
# M2: M_SDF_TrebleClef_Ornament - Ornate 3D G-clef
# ============================================================
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

# ============================================================
# M3: M_SDF_FloatingNotes - Animated musical notes floating in 3D
# ============================================================
$floatNotesCode = @'
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-3.5);
float3 rd=normalize(float3(uvc*0.9,1.5));
float tt=Time*0.1;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;
float noteType=0.0;
[loop]
for(int step=0;step<85;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  for(int ni=0;ni<8;ni++){
    float fi=float(ni);
    float ang=fi*0.785+Time*0.08;
    float radius=0.6+0.15*sin(fi*2.3);
    float cx=cos(ang)*radius;
    float cz=sin(ang)*radius;
    float bobY=sin(Time*0.5+fi*1.1)*0.15;
    float cy_pos=sin(fi*1.9)*0.4+bobY;
    float3 np=float3(p.x-cx,p.y-cy_pos,p.z-cz);
    float tilt=sin(fi*0.7)*0.4;
    float2 hp2=float2(cos(tilt)*np.x+sin(tilt)*np.y,-sin(tilt)*np.x+cos(tilt)*np.y);
    float head=length(hp2/float2(0.08,0.055))-1.0;
    float headZ=abs(np.z)-0.012;
    float note=max(head,headZ);
    float stemDir=step(0.5,fract(fi*0.3+0.1))*2.0-1.0;
    float stemX=abs(np.x-0.07*stemDir)-0.004;
    float stemLen=0.35+0.05*sin(fi*3.1);
    float stemY=max(-np.y*stemDir,np.y*stemDir-stemLen);
    float stem=max(stemX,stemY);
    stem=max(stem,abs(np.z)-0.006);
    note=min(note,stem);
    int noteKind=int(floor(fmod(fi,3.0)));
    if(noteKind==1){
      float flagBase=np.y*stemDir-stemLen+0.05;
      float flagT=saturate((np.y*stemDir-flagBase)/0.12);
      float flagCurve=0.06*sin(flagT*3.14159);
      float flagX=abs(np.x-0.07*stemDir-flagCurve)-0.004;
      float flagY=max(-(np.y*stemDir-flagBase),np.y*stemDir-flagBase-0.12);
      float flag=max(flagX,flagY);
      flag=max(flag,abs(np.z)-0.005);
      note=min(note,flag);
    }
    if(noteKind==2){
      float hollow=length(hp2/float2(0.065,0.04))-1.0;
      float ring=abs(length(hp2/float2(0.08,0.055))-1.0)-0.015;
      float hollowNote=max(max(hollow,headZ),-ring);
      if(hollowNote<note){note=hollowNote;}
    }
    if(note<d){d=note;noteType=fi;}
  }
  if(d<0.003){hit=t;break;}
  t+=d*0.65;if(t>25.0)break;
}
float3 bg=lerp(float3(0.05,0.03,0.1),float3(0.015,0.008,0.04),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(float3(hp.x+eps,hp.y,hp.z))-length(float3(hp.x-eps,hp.y,hp.z)),
  length(float3(hp.x,hp.y+eps,hp.z))-length(float3(hp.x,hp.y-eps,hp.z)),
  length(float3(hp.x,hp.y,hp.z+eps))-length(float3(hp.x,hp.y,hp.z-eps))
));
nrm=normalize(nrm);
float hue=fmod(noteType/8.0+Time*0.02,1.0);
float3 noteCol=lerp(NoteColorA,NoteColorB,sin(hue*6.283)*0.5+0.5);
float3 L=normalize(float3(0.4,0.7,-0.6));
float diff=saturate(dot(nrm,L));
float3 H2=normalize(L+normalize(-rd));
float spec=pow(saturate(dot(nrm,H2)),40.0);
float fresnel=pow(1.0-saturate(dot(nrm,normalize(-rd))),3.0);
float3 col=noteCol*(0.3+0.7*diff)+spec*0.4;
col+=fresnel*AccentColor*GlowIntensity;
float ao=0.7+0.3*saturate(abs(hp.z)*4.0);
col*=ao;
return saturate(col);
'@
Build-SDF "M_SDF_FloatingNotes" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="NA";class="VectorParameter";props=@{ParameterName="NoteColorA"}},
    @{id="NB";class="VectorParameter";props=@{ParameterName="NoteColorB"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}}
) $floatNotesCode @("UV","Time","NoteColorA","NoteColorB","AccentColor","GlowIntensity")

Write-Host "`n=== ALL 3 MUSIC MATERIALS COMPLETE ===" -ForegroundColor Yellow
