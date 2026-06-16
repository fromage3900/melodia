$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$sdfPath = "/Game/_PROJECT/04_Materials/SDF"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    if ($extraArgs) { foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] } }
    $body = @{
        jsonrpc = "2.0"; id = 1; method = "tools/call"
        params = @{ name = "material_query"; arguments = $arguments }
    } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 60
        $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
        if ($text) { return $text.text }
        return ($r | ConvertTo-Json -Depth 3 -Compress)
    } catch { Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

function Build-SDF($name, $paramDefs, $hlslCode) {
    $fullPath = "$sdfPath/$name"
    Write-Host "`n========== $name ==========" -ForegroundColor Cyan
    
    # 1. Create material
    Write-Host "  Creating..."
    $r = Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" }
    Write-Host "    $r"
    
    # 2. Set two_sided
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    
    # 3. Build parameter nodes
    $nodes = @()
    $posY = -200
    foreach ($p in $paramDefs) {
        $node = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }
        if ($p.props) { $node.props = $p.props }
        $nodes += $node
        $posY += 60
    }
    $graphSpec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() }
    Write-Host "  Building params..."
    $r = Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = $graphSpec }
    $buildResult = $r | ConvertFrom-Json
    Write-Host "    nodes_created=$($buildResult.nodes_created) errors=$($buildResult.has_errors)"
    
    # 4. Create Custom HLSL node WITHOUT input pins (code accesses params as uniforms directly)
    Write-Host "  Creating HLSL node (no input pins)..."
    $r = Call-Mono "create_custom_hlsl_node" @{
        asset_path = $fullPath
        code = $hlslCode
        description = $name
        output_type = "CMOT_Float3"
    }
    Write-Host "    $r"
    
    # 5. Wire output to EmissiveColor
    Write-Host "  Wiring output to EmissiveColor..."
    $exprResult = Call-Mono "get_all_expressions" @{ asset_path = $fullPath }
    $exprs = ($exprResult | ConvertFrom-Json).expressions
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" }).name
    $r = Call-Mono "connect_expressions" @{
        asset_path = $fullPath
        from_expression = $customNode
        to_property = "EmissiveColor"
    }
    Write-Host "    $r"
    
    # 6. Recompile
    Write-Host "  Recompiling..."
    $r = Call-Mono "recompile_material" @{ asset_path = $fullPath }
    
    # 7. Stats
    Start-Sleep -Seconds 4
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    $stats = $r | ConvertFrom-Json
    if ($stats.is_compiled) {
        Write-Host "  SUCCESS: $($stats.num_pixel_shader_instructions) PS, $($stats.num_vertex_shader_instructions) VS" -ForegroundColor Green
    } else {
        Write-Host "  FAILED TO COMPILE" -ForegroundColor Red
        if ($stats.compile_errors) {
            foreach ($e in ($stats.compile_errors | Select-Object -First 2)) {
                $short = ($e -replace '\\n',' ' -replace '\\r','').Substring(0, [Math]::Min(300, $e.Length))
                Write-Host "  $short" -ForegroundColor Red
            }
        }
    }
    return $r
}

# ============================================================
# SHARED HLSL PATTERNS
# ============================================================
$bgCode = 'float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));'

# ============================================================
# MATERIAL 2: M_SDF_JuliaSet_Quaternion
# ============================================================
$juliaCode = @"
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*0.1;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float4 cc=float4(JuliaC_x+0.2*sin(Time*0.13),JuliaC_y+0.15*cos(Time*0.17),JuliaC_z+0.1*sin(Time*0.11),JuliaC_w+0.1*cos(Time*0.19));
float Pw=max(Power,2.0);
float MI=max(MaxIterations,2.0);
float Scl=max(Scale,0.1);
float t=0.0;float hit=-1.0;float4 trap=float4(1e10,1e10,1e10,1e10);
[loop]
for(int step=0;step<80;step++){
  float3 p=ro+rd*t;
  float4 z=float4(p.x*Scl,p.y*Scl,p.z*Scl,SliceW);
  float dz2=dot(z,z);
  float4 tr=float4(1e10,1e10,1e10,1e10);
  int escaped=0;
  [loop]
  for(int i=0;i<int(MI);i++){
    float nz2=dot(z,z);
    if(nz2>4.0){escaped=1;break;}
    float4 z2=float4(z.x*z.x-z.y*z.y-z.z*z.z-z.w*z.w,2.0*z.x*z.y,2.0*z.x*z.z,2.0*z.x*z.w);
    z=z2+cc;
    tr=min(tr,float4(abs(z),sqrt(nz2)));
  }
  float dist=0.5*sqrt(dot(z,z)/max(dz2,1e-10))*log(max(dot(z,z),1e-10));
  if(escaped==0)dist=0.0;
  if(dist<0.002&&escaped>0){hit=t;trap=tr;break;}
  t+=max(dist*0.7,0.001);
  if(t>20.0)break;
}
$bgCode
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.003;
#define JDE(PP, OUT_V) { float4 _z=float4((PP).x*Scl,(PP).y*Scl,(PP).z*Scl,SliceW); for(int _i=0;_i<int(MI);_i++){float _n=dot(_z,_z);if(_n>4.0)break;_z=float4(_z.x*_z.x-_z.y*_z.y-_z.z*_z.z-_z.w*_z.w,2.0*_z.x*_z.y,2.0*_z.x*_z.z,2.0*_z.x*_z.w)+cc;} OUT_V=sqrt(dot(_z,_z)); }
float n1,n2,n3,n4,n5,n6;
JDE(hp+float3(eps,0,0),n1) JDE(hp-float3(eps,0,0),n2)
JDE(hp+float3(0,eps,0),n3) JDE(hp-float3(0,eps,0),n4)
JDE(hp+float3(0,0,eps),n5) JDE(hp-float3(0,0,eps),n6)
float3 nrm=normalize(float3(n1-n2,n3-n4,n5-n6));
float orbitMix=saturate(trap.w*1.5);
float3 col=lerp(HighlightColor,BaseColor,orbitMix);
float3 L=normalize(float3(0.5,0.5,-0.7));
float diff=saturate(dot(nrm,L));
col*=(0.2+0.8*diff);
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=HighlightColor*fres*0.5;
col=lerp(InkColor,col,saturate(orbitMix*2.0+0.3));
return saturate(col);
"@

$juliaParams = @(
    @{ id = "TexCoord"; class = "TextureCoordinate" }
    @{ id = "TimeNode"; class = "Time" }
    @{ id = "JCx"; class = "ScalarParameter"; props = @{ ParameterName = "JuliaC_x" } }
    @{ id = "JCy"; class = "ScalarParameter"; props = @{ ParameterName = "JuliaC_y" } }
    @{ id = "JCz"; class = "ScalarParameter"; props = @{ ParameterName = "JuliaC_z" } }
    @{ id = "JCw"; class = "ScalarParameter"; props = @{ ParameterName = "JuliaC_w" } }
    @{ id = "Power"; class = "ScalarParameter"; props = @{ ParameterName = "Power" } }
    @{ id = "MaxIter"; class = "ScalarParameter"; props = @{ ParameterName = "MaxIterations" } }
    @{ id = "SliceW"; class = "ScalarParameter"; props = @{ ParameterName = "SliceW" } }
    @{ id = "ScaleP"; class = "ScalarParameter"; props = @{ ParameterName = "Scale" } }
    @{ id = "BaseCol"; class = "VectorParameter"; props = @{ ParameterName = "BaseColor" } }
    @{ id = "HighCol"; class = "VectorParameter"; props = @{ ParameterName = "HighlightColor" } }
    @{ id = "InkCol"; class = "VectorParameter"; props = @{ ParameterName = "InkColor" } }
)

Build-SDF "M_SDF_JuliaSet_Quaternion" $juliaParams $juliaCode

# ============================================================
# MATERIAL 3: M_SDF_SierpinskiTetrahedron
# ============================================================
$sierpinskiCode = @"
#define SMIN(A,B,K) (-log2(exp2(-(A)*(1.0/(K)))+exp2(-(B)*(1.0/(K))))*(K))
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
  float sc=1.0;float d=1e10;int depth=0;
  [loop]
  for(int i=0;i<int(IterationDepth);i++){
    p=abs(p)-float3(1.0,1.0,1.0)*sc;
    if(p.x<p.y)p.xy=p.yx;
    if(p.x<p.z)p.xz=p.zx;
    if(p.y<p.z)p.yz=p.zy;
    p-=float3(1.0,1.0,1.0)*sc*(Scl-1.0);
    sc*=Scl;
    depth=i;
    d=min(d,(length(max(abs(p)-float3(1.0,1.0,1.0)*sc,0.0))-0.1*sc)/sc);
  }
  d/=sc;
  if(d<0.001){hit=t;depthFrac=float(depth)/max(IterationDepth,1.0);break;}
  t+=d*0.8;
  if(t>20.0)break;
}
$bgCode
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
#define SDE(PP, OUT_D) { float3 _p=(PP)*Scl; float _sc=1.0; float _d=1e10; for(int _i=0;_i<int(IterationDepth);_i++){_p=abs(_p)-float3(1,1,1)*_sc;if(_p.x<_p.y)_p.xy=_p.yx;if(_p.x<_p.z)_p.xz=_p.zx;if(_p.y<_p.z)_p.yz=_p.zy;_p-=float3(1,1,1)*_sc*(Scl-1.0);_sc*=Scl;_d=min(_d,(length(max(abs(_p)-float3(1,1,1)*_sc,0.0))-0.1*_sc)/_sc);} OUT_D=_d/_sc; }
float n1,n2,n3,n4,n5,n6;
SDE(hp+float3(eps,0,0),n1) SDE(hp-float3(eps,0,0),n2)
SDE(hp+float3(0,eps,0),n3) SDE(hp-float3(0,eps,0),n4)
SDE(hp+float3(0,0,eps),n5) SDE(hp-float3(0,0,eps),n6)
float3 nrm=normalize(float3(n1-n2,n3-n4,n5-n6));
float3 col=lerp(EdgeColor,BaseColor,depthFrac);
col+=EdgeColor*GlowIntensity*(1.0-depthFrac);
float3 L=normalize(float3(0.5,0.5,-0.7));
float diff=saturate(dot(nrm,L));
col*=(0.15+0.85*diff);
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=EdgeColor*fres*0.4;
col=lerp(InkColor,col,saturate(depthFrac*2.0+0.2));
return saturate(col);
"@

$sierpinskiParams = @(
    @{ id = "TexCoord"; class = "TextureCoordinate" }
    @{ id = "TimeNode"; class = "Time" }
    @{ id = "IterDepth"; class = "ScalarParameter"; props = @{ ParameterName = "IterationDepth" } }
    @{ id = "ScaleP"; class = "ScalarParameter"; props = @{ ParameterName = "Scale" } }
    @{ id = "RotSpeed"; class = "ScalarParameter"; props = @{ ParameterName = "RotationSpeed" } }
    @{ id = "GlowInt"; class = "ScalarParameter"; props = @{ ParameterName = "GlowIntensity" } }
    @{ id = "BaseCol"; class = "VectorParameter"; props = @{ ParameterName = "BaseColor" } }
    @{ id = "EdgeCol"; class = "VectorParameter"; props = @{ ParameterName = "EdgeColor" } }
    @{ id = "InkCol"; class = "VectorParameter"; props = @{ ParameterName = "InkColor" } }
)

Build-SDF "M_SDF_SierpinskiTetrahedron" $sierpinskiParams $sierpinskiCode

Write-Host "`n========== BATCH 1 DONE ==========" -ForegroundColor Green
