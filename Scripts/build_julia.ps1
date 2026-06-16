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
    } catch { Write-Host "  NET_ERROR: $($_.Exception.Message)" -ForegroundColor Red; return $null }
}

function Build-SDF($name, $paramDefs, $hlslCode, $inputNames) {
    $fullPath = "$sdfPath/$name"
    Write-Host "`n========== $name ==========" -ForegroundColor Cyan
    
    # 1. Create material
    Write-Host "  1. Creating material..."
    Call-Mono "create_material" @{ asset_path = $fullPath; blend_mode = "Opaque"; shading_model = "Unlit" } | Out-Null
    Call-Mono "set_material_property" @{ asset_path = $fullPath; two_sided = $true } | Out-Null
    
    # 2. Build parameter nodes only
    $nodes = @()
    $posY = -200
    foreach ($p in $paramDefs) {
        $node = @{ id = $p.id; class = $p.class; pos = @(-1200, $posY) }
        if ($p.props) { $node.props = $p.props }
        $nodes += $node
        $posY += 60
    }
    Write-Host "  2. Building $($nodes.Count) param nodes..."
    $r = Call-Mono "build_material_graph" @{ asset_path = $fullPath; clear_existing = $true; graph_spec = @{ nodes = $nodes; custom_hlsl_nodes = @(); connections = @(); outputs = @() } }
    Write-Host "    $r"
    
    # 3. Create Custom HLSL node with properly named input pins
    $inputObjs = @()
    foreach ($n in $inputNames) { $inputObjs += @{ name = $n } }
    Write-Host "  3. Creating HLSL node with $($inputObjs.Count) inputs..."
    $r = Call-Mono "create_custom_hlsl_node" @{
        asset_path = $fullPath; code = $hlslCode; description = $name; output_type = "CMOT_Float3"; inputs = $inputObjs
    }
    $createResult = $r | ConvertFrom-Json
    Write-Host "    created=$($createResult.expression_name) inputs=$($createResult.input_count)"
    
    # 4. Get expression names
    $exprResult = Call-Mono "get_all_expressions" @{ asset_path = $fullPath }
    $exprs = ($exprResult | ConvertFrom-Json).expressions
    $customNode = ($exprs | Where-Object { $_.class -eq "MaterialExpressionCustom" }).name
    $paramExprs = @($exprs | Where-Object { $_.class -ne "MaterialExpressionCustom" })
    Write-Host "  4. Expressions: $($paramExprs.Count) params + custom=$customNode"
    
    # 5. Wire each param to custom node (in order - connects to next available pin)
    Write-Host "  5. Wiring inputs..."
    for ($i = 0; $i -lt $paramExprs.Count -and $i -lt $inputNames.Count; $i++) {
        $r = Call-Mono "connect_expressions" @{
            asset_path = $fullPath; from_expression = $paramExprs[$i].name; to_expression = $customNode
        }
        $connResult = $r | ConvertFrom-Json
        $status = if ($connResult.connected) { "OK" } else { "FAIL" }
        Write-Host "    [$i] $($paramExprs[$i].name) -> $($inputNames[$i]) = $status"
    }
    
    # 6. Wire output to EmissiveColor
    Write-Host "  6. Wiring output..."
    Call-Mono "connect_expressions" @{ asset_path = $fullPath; from_expression = $customNode; to_property = "EmissiveColor" } | Out-Null
    
    # 7. Recompile + stats
    Write-Host "  7. Recompiling..."
    Call-Mono "recompile_material" @{ asset_path = $fullPath } | Out-Null
    Start-Sleep -Seconds 4
    $r = Call-Mono "get_compilation_stats" @{ asset_path = $fullPath }
    if (-not $r) { Write-Host "  STATS: null (server issue)" -ForegroundColor Red; return $null }
    $stats = $r | ConvertFrom-Json
    if ($stats.is_compiled) {
        Write-Host "  SUCCESS: $($stats.num_pixel_shader_instructions) PS, $($stats.num_vertex_shader_instructions) VS" -ForegroundColor Green
    } else {
        Write-Host "  FAILED" -ForegroundColor Red
        $errs = @($stats.compile_errors)
        if ($errs.Count -gt 0) {
            $e = $errs[0] -replace '\\n',' ' -replace '\\r',''
            Write-Host "    $($e.Substring(0, [Math]::Min(250, $e.Length)))" -ForegroundColor Red
        }
    }
    return $r
}

# ============================================================
# MATERIAL 2: M_SDF_JuliaSet_Quaternion
# Pin names match code variable names exactly
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
float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.003;
#define JDE(PP, OUT_V) { float4 _z=float4((PP).x*Scl,(PP).y*Scl,(PP).z*Scl,SliceW); for(int _i=0;_i<int(MI);_i++){float _n=dot(_z,_z);if(_n>4.0)break;_z=float4(_z.x*_z.x-_z.y*_z.y-_z.z*_z.z-_z.w*_z.w,2.0*_z.x*_z.y,2.0*_z.x*_z.z,2.0*_z.x*_z.w)+cc;} OUT_V=sqrt(dot(_z,_z)); }
float jn1,jn2,jn3,jn4,jn5,jn6;
JDE(hp+float3(eps,0,0),jn1) JDE(hp-float3(eps,0,0),jn2)
JDE(hp+float3(0,eps,0),jn3) JDE(hp-float3(0,eps,0),jn4)
JDE(hp+float3(0,0,eps),jn5) JDE(hp-float3(0,0,eps),jn6)
float3 nrm=normalize(float3(jn1-jn2,jn3-jn4,jn5-jn6));
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
# Pin names MUST match code variable names exactly
$juliaInputs = @("UV","Time","JuliaC_x","JuliaC_y","JuliaC_z","JuliaC_w","Power","MaxIterations","SliceW","Scale","BaseColor","HighlightColor","InkColor")

Build-SDF "M_SDF_JuliaSet_Quaternion" $juliaParams $juliaCode $juliaInputs

Write-Host "`n========== JULIA DONE ==========" -ForegroundColor Green
