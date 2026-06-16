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
    $fullPath = "$sdfPath/$name"
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
    Write-Host "  Waiting 12s before expressions..."
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
    Start-Sleep -Seconds 10
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if ($r) {
        try {
            $s = $r | ConvertFrom-Json
            if ($s.is_compiled) { Write-Host "  COMPILED: $($s.num_pixel_shader_instructions) PS" -ForegroundColor Green; return $true }
            else {
                Write-Host "  COMPILE FAIL" -ForegroundColor Red
                $e = @($s.compile_errors)
                if ($e.Count -gt 0) { Write-Host "  $($e[0].Substring(0,[Math]::Min(400,$e[0].Length)))" -ForegroundColor Red }
                return $false
            }
        } catch { Write-Host "  Parse error" -ForegroundColor Yellow; return $false }
    } else { Write-Host "  STATS null" -ForegroundColor Yellow; return $false }
}

# M3: FloatingNotes (frac instead of fract)
$code = @'
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
    float stemDir=step(0.5,frac(fi*0.3+0.1))*2.0-1.0;
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
$ok = Build-SDF "M_SDF_FloatingNotes" @(
    @{id="TC";class="TextureCoordinate"},@{id="TM";class="Time"},
    @{id="NA";class="VectorParameter";props=@{ParameterName="NoteColorA"}},
    @{id="NB";class="VectorParameter";props=@{ParameterName="NoteColorB"}},
    @{id="AC";class="VectorParameter";props=@{ParameterName="AccentColor"}},
    @{id="GI";class="ScalarParameter";props=@{ParameterName="GlowIntensity"}}
) $code @("UV","Time","NoteColorA","NoteColorB","AccentColor","GlowIntensity")
Call-Mono "save_material" @{ asset_path = "$sdfPath/M_SDF_FloatingNotes" } | Out-Null
Write-Host "M3 result: $(if($ok){'OK'}else{'FAIL'})" -ForegroundColor $(if($ok){'Green'}else{'Red'})
