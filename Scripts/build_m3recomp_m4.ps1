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
    } else { Write-Host "  STATS null (server may be down)" -ForegroundColor Yellow; return $false }
}

# Recompile M3 (already wired)
Write-Host "=== Recompile M3 FloatingNotes ===" -ForegroundColor Cyan
Call-Mono "recompile_material" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" } | Out-Null
Start-Sleep -Seconds 12
$r = Call-Mono "get_compilation_stats" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" }
if ($r) {
    try {
        $s = $r | ConvertFrom-Json
        if ($s.is_compiled) { Write-Host "  M3 COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green }
        else {
            Write-Host "  M3 FAIL" -ForegroundColor Red
            $e = @($s.compile_errors)
            if ($e.Count -gt 0) { Write-Host "  $($e[0].Substring(0,[Math]::Min(400,$e[0].Length)))" -ForegroundColor Red }
        }
    } catch { Write-Host "  Parse error" -ForegroundColor Yellow }
}
Call-Mono "save_material" @{ asset_path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_FloatingNotes" } | Out-Null

# M4: GrandStaff
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
$ok4 = Build-SDF "M_SDF_GrandStaff_CrossSection" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}},
    @{id="PC";class="VectorParameter";props=@{ParameterName="PaperColor"}},
    @{id="IC";class="VectorParameter";props=@{ParameterName="InkColor"}},
    @{id="NC";class="VectorParameter";props=@{ParameterName="NoteColor"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}}
) $grandCode @("UV","Time","GlowIntensity","PaperColor","InkColor","NoteColor","AccentColor")
Write-Host "M4 result: $(if($ok4){'OK'}else{'FAIL'})" -ForegroundColor $(if($ok4){'Green'}else{'Red'})
