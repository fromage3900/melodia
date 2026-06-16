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
            if ($s.is_compiled) { Write-Host "  OK: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
            else { Write-Host "  FAIL" -ForegroundColor Red; $e = @($s.compile_errors); if ($e.Count -gt 0) { $err = $e[0] -replace '\\n',' ' -replace '\\r',''; Write-Host "  $($err.Substring(0,[Math]::Min(300,$err.Length)))" -ForegroundColor Red } }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow }
}

$bg = 'float3 bg=lerp(float3(0.06,0.04,0.12),float3(0.02,0.01,0.06),saturate(uvc.y*0.5+0.5));'

# ============================================================
# M1: M_SDF_SheetMusic_Score
# ============================================================
$sheetCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.8);
float3 rd=normalize(float3(uvc*0.8,1.5));
float tt=Time*0.06;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float t=0.0;float hit=-1.0;
float noteMask=0.0;float staffMask=0.0;float clefMask=0.0;float beamMask=0.0;
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float d=1e10;
  float paper=abs(p.z)-0.02;
  float paperEdge=max(abs(p.x)-1.4,abs(p.y)-0.9);
  paper=max(paper,paperEdge);
  if(paper<d){d=paper;noteMask=0.0;staffMask=0.0;clefMask=0.0;beamMask=0.0;}
  for(int s=0;s<5;s++){
    float ly=lerp(-0.3,0.3,float(s)/4.0);
    float line=abs(p.y-ly)-0.004;
    float lineX=max(-p.x-1.2,p.x-1.2);
    line=max(line,lineX);
    if(line<d){d=line;staffMask=1.0;noteMask=0.0;clefMask=0.0;beamMask=0.0;}
  }
  float barX1=abs(p.x-0.5)-0.005;float barX2=abs(p.x+0.5)-0.005;
  float barX3=abs(p.x-1.15)-0.006;
  float bar=min(min(barX1,barX2),barX3);
  float barY=max(-p.y-0.3,p.y-0.3);
  bar=max(bar,barY);
  if(bar<d){d=bar;staffMask=1.0;noteMask=0.0;clefMask=0.0;beamMask=0.0;}
  float notePositions[12]={-0.9,-0.65,-0.35,-0.1,0.15,0.4,0.65,0.85,-0.8,-0.45,0.0,0.55};
  float noteHeights[12]={0.15,0.0,-0.15,-0.3,0.0,0.15,-0.15,0.3,0.075,-0.225,0.225,-0.075};
  for(int ni=0;ni<12;ni++){
    float nx=notePositions[ni];float ny=noteHeights[ni];
    float2 nc=float2(p.x-nx,p.y-ny);
    float ang=0.3;
    float2 nr=float2(cos(ang)*nc.x+sin(ang)*nc.y,-sin(ang)*nc.x+cos(ang)*nc.y);
    float head=length(nr/float2(0.06,0.04))-1.0;
    float headZ=abs(p.z)-0.01;
    float note=max(head,headZ);
    float stem=abs(p.x-nx-0.05)-0.003;
    float stemY=max(-(p.y-ny),p.y-ny-0.25);
    stem=max(stem,stemY);
    stem=max(stem,abs(p.z)-0.005);
    note=min(note,stem);
    if(ni%3==1){
      float flagT=saturate((p.y-ny-0.2)/0.08);
      float flagX=abs(p.x-nx-0.05-0.04*sin(flagT*3.14159))-0.003;
      float flagY=max(-(p.y-ny-0.2),p.y-ny-0.28);
      float flag=max(flagX,flagY);
      flag=max(flag,abs(p.z)-0.004);
      note=min(note,flag);
    }
    if(note<d){d=note;noteMask=1.0;staffMask=0.0;clefMask=0.0;beamMask=0.0;}
  }
  for(int bi=0;bi<3;bi++){
    float bx1=notePositions[bi*4+1];float bx2=notePositions[bi*4+2];
    float by1=noteHeights[bi*4+1]+0.25;float by2=noteHeights[bi*4+2]+0.25;
    float bdir=sign(p.x-(bx1+bx2)*0.5);
    float beamDist=abs(p.y-lerp(by1,by2,saturate((p.x-bx1)/(bx2-bx1+0.001))))-0.008;
    float beamX=max(bx1-p.x,p.x-bx2);
    float beam=max(beamDist,beamX);
    beam=max(beam,abs(p.z)-0.004);
    if(beam<d){d=beam;beamMask=1.0;noteMask=0.0;staffMask=0.0;clefMask=0.0;}
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
  if(clef<d){d=clef;clefMask=1.0;noteMask=0.0;staffMask=0.0;beamMask=0.0;}
  if(d<0.002){hit=t;break;}
  t+=d*0.7;if(t>20.0)break;
}
$bg
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float3 nrm=normalize(float3(
  length(abs(hp+float3(eps,0,0))-0.5)-length(abs(hp-float3(eps,0,0))-0.5),
  length(abs(hp+float3(0,eps,0))-0.5)-length(abs(hp-float3(0,eps,0))-0.5),
  length(abs(hp+float3(0,0,eps))-0.5)-length(abs(hp-float3(0,0,eps))-0.5)));
float3 paperCol=PaperColor;
float3 inkCol=InkColor;
float3 noteCol=NoteColor;
float3 col=paperCol;
col=lerp(col,inkCol,staffMask);
col=lerp(col,noteCol,noteMask);
col=lerp(col,inkCol,beamMask);
col=lerp(col,AccentColor,clefMask);
float3 L=normalize(float3(0.3,0.6,-0.7));
col*=(0.3+0.7*saturate(dot(nrm,L)));
float shadow=1.0-saturate(abs(hp.z)*3.0)*0.2;
col*=shadow;
float glow=noteMask*GlowIntensity*pow(saturate(1.0-abs(hp.z)*5.0),2.0);
col+=noteCol*glow;
return saturate(col);
"@
Build-SDF "M_SDF_SheetMusic_Score" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="PC";class="VectorParameter";props=@{ParameterName="PaperColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}},
    @{id="NC";class="VectorParameter";props=@{ParameterName="NoteColor"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}}
) $sheetCode @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")

Write-Host "`n=== M1 COMPLETE ===" -ForegroundColor Yellow
