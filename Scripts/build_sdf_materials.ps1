$ErrorActionPreference = "Continue"
$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"

function Invoke-Monolith($toolName, $arguments) {
    $body = @{
        jsonrpc = "2.0"
        id = 1
        method = "tools/call"
        params = @{
            name = $toolName
            arguments = $arguments
        }
    } | ConvertTo-Json -Depth 20 -Compress
    try {
        $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 60
        if ($r.result.content) {
            $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
            if ($text -and $text.text) { return $text.text }
            return ($r.result.content | ConvertTo-Json -Depth 5 -Compress)
        }
        if ($r.error) { return "MCP_ERROR: $($r.error.message)" }
        return ($r | ConvertTo-Json -Depth 5 -Compress)
    } catch {
        Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
        return $null
    }
}

function Build-SDFMaterial($path, $graphSpec) {
    Write-Host "`n=== Building $path ===" -ForegroundColor Cyan

    Write-Host "  Creating material..."
    $r = Invoke-Monolith "material_query" @{
        action = "create_material"
        asset_path = $path
        blend_mode = "Opaque"
        shading_model = "Unlit"
    }
    Write-Host "  Create: $r"

    Write-Host "  Setting two_sided..."
    $r = Invoke-Monolith "material_query" @{
        action = "set_material_property"
        asset_path = $path
        two_sided = $true
    }

    Write-Host "  Building graph..."
    $r = Invoke-Monolith "material_query" @{
        action = "build_material_graph"
        asset_path = $path
        clear_existing = $true
        graph_spec = $graphSpec
    }
    Write-Host "  Graph: $r"

    Write-Host "  Recompiling..."
    $r = Invoke-Monolith "material_query" @{
        action = "recompile_material"
        asset_path = $path
    }
    Write-Host "  Compile: $r"

    Write-Host "  Getting stats..."
    Start-Sleep -Seconds 3
    $r = Invoke-Monolith "material_query" @{
        action = "get_compilation_stats"
        asset_path = $path
    }
    Write-Host "  STATS: $r" -ForegroundColor Yellow
    return $r
}

# ============================================================
# MATERIAL 1: M_SDF_Mandelbulb_Master
# ============================================================
# Uses M_SDF_Baroque pattern: #define macros for SDF eval, [loop]/[unroll] attributes
$mandelbulbCode = @'
#define MB_DE(PP, OUT_D, OUT_TR) { float3 _z=(PP)*Scl; float _dr=1.0; float _r=0.0; float4 _tr=float4(1e10,1e10,1e10,1e10); for(int _i=0;_i<int(MI);_i++){_r=length(_z);if(_r>2.0)break;float _th=acos(clamp(_z.z/max(_r,1e-6),-1.0,1.0));float _ph=atan2(_z.y,_z.x);_dr=pow(_r,Pw-1.0)*Pw*_dr+1.0;float _zr=pow(_r,Pw);_th*=Pw;_ph*=Pw;_z=_zr*float3(sin(_th)*cos(_ph),sin(_th)*sin(_ph),cos(_th));_z=_z*Scl+(PP)*Scl;_tr=min(_tr,float4(abs(_z),_r));} OUT_D=0.5*log(max(_r,1e-10))*_r/max(_dr,1e-10); OUT_TR=_tr; }
float2 uvc=(UV-0.5)*2.0;
float3 ro=float3(0.0,0.0,-2.5);
float3 rd=normalize(float3(uvc*0.7,1.5));
float tt=Time*RotSpd;
float cy=cos(tt),sy=sin(tt);
rd=float3(cy*rd.x-sy*rd.z,rd.y,sy*rd.x+cy*rd.z);
float Pw=max(Power,2.0);
float Scl=max(ScaleParam,0.1);
float MI=max(MaxIter,2.0);
float t=0.0;float hit=-1.0;float4 trap=float4(1e10,1e10,1e10,1e10);
[loop]
for(int step=0;step<100;step++){
  float3 p=ro+rd*t;
  float d;float4 tr;
  MB_DE(p,d,tr)
  if(SliceOff>0.0&&p.z>SliceOff){
    float3 sp=ro+rd*((SliceOff-ro.z)/max(rd.z,0.001));
    float sd;float4 str2;
    MB_DE(sp,sd,str2)
    d=min(d,abs(sd));
  }
  if(d<0.001){hit=t;trap=tr;break;}
  t+=d*0.8;
  if(t>20.0)break;
}
float3 bg=lerp(float3(0.08,0.06,0.14),float3(0.03,0.02,0.07),saturate(uvc.y*0.5+0.5));
if(hit<0.0)return bg;
float3 hp=ro+rd*hit;
float eps=0.002;
float d1;float4 t1;MB_DE(hp+float3(eps,0,0),d1,t1)
float d2;float4 t2;MB_DE(hp-float3(eps,0,0),d2,t2)
float d3;float4 t3;MB_DE(hp+float3(0,eps,0),d3,t3)
float d4;float4 t4;MB_DE(hp-float3(0,eps,0),d4,t4)
float d5;float4 t5;MB_DE(hp+float3(0,0,eps),d5,t5)
float d6;float4 t6;MB_DE(hp-float3(0,0,eps),d6,t6)
float3 nrm=normalize(float3(d1-d2,d3-d4,d5-d6));
float ao=0.0;
[unroll]
for(int a=1;a<=4;a++){
  float hr=0.012*float(a);
  float ad;float4 at;
  MB_DE(hp+nrm*hr,ad,at)
  ao+=(hr-ad);
}
ao=saturate(1.0-ao*AOStr*3.0);
ao=0.15+0.85*ao;
float orbitMix=saturate(trap.w*2.0);
float3 col=lerp(GlowCol,BaseCol,orbitMix);
col*=ao;
float3 L=normalize(float3(0.6,0.4,-0.7));
float diff=saturate(dot(nrm,L));
col*=(0.25+0.75*diff);
float spec=pow(saturate(dot(reflect(-L,nrm),-rd)),24.0);
col+=GlowCol*spec*0.4*ao;
float fres=pow(1.0-saturate(dot(nrm,-rd)),3.0);
col+=GlowCol*fres*0.4;
col=lerp(InkCol,col,ao);
return saturate(col);
'@

$mandelbulbGraph = @{
    nodes = @(
        @{ id = "TexCoord"; class = "TextureCoordinate"; pos = @(-1200, -200) }
        @{ id = "TimeNode"; class = "Time"; pos = @(-1200, -100) }
        @{ id = "Power"; class = "ScalarParameter"; props = @{ ParameterName = "Power" }; pos = @(-1200, 0) }
        @{ id = "MaxIter"; class = "ScalarParameter"; props = @{ ParameterName = "MaxIterations" }; pos = @(-1200, 60) }
        @{ id = "SliceOff"; class = "ScalarParameter"; props = @{ ParameterName = "SliceOffset" }; pos = @(-1200, 120) }
        @{ id = "RotSpd"; class = "ScalarParameter"; props = @{ ParameterName = "RotationSpeed" }; pos = @(-1200, 180) }
        @{ id = "ScaleParam"; class = "ScalarParameter"; props = @{ ParameterName = "Scale" }; pos = @(-1200, 240) }
        @{ id = "AOStr"; class = "ScalarParameter"; props = @{ ParameterName = "AOStrength" }; pos = @(-1200, 300) }
        @{ id = "BaseCol"; class = "VectorParameter"; props = @{ ParameterName = "BaseColor" }; pos = @(-1200, 400) }
        @{ id = "GlowCol"; class = "VectorParameter"; props = @{ ParameterName = "GlowColor" }; pos = @(-1200, 480) }
        @{ id = "InkCol"; class = "VectorParameter"; props = @{ ParameterName = "InkColor" }; pos = @(-1200, 560) }
    )
    custom_hlsl_nodes = @(
        @{
            id = "SDFCore"
            code = $mandelbulbCode
            description = "Mandelbulb fractal raymarcher"
            output_type = "CMOT_Float3"
            inputs = @("TexCoord", "TimeNode", "Power", "MaxIter", "SliceOff", "RotSpd", "ScaleParam", "AOStr", "BaseCol", "GlowCol", "InkCol")

            pos = @(-400, 200)
        }
    )
    connections = @(
        @{ from = "TexCoord"; to = "SDFCore"; to_pin = "TexCoord" }
        @{ from = "TimeNode"; to = "SDFCore"; to_pin = "TimeNode" }
        @{ from = "Power"; to = "SDFCore"; to_pin = "Power" }
        @{ from = "MaxIter"; to = "SDFCore"; to_pin = "MaxIter" }
        @{ from = "SliceOff"; to = "SDFCore"; to_pin = "SliceOff" }
        @{ from = "RotSpd"; to = "SDFCore"; to_pin = "RotSpd" }
        @{ from = "ScaleParam"; to = "SDFCore"; to_pin = "ScaleParam" }
        @{ from = "AOStr"; to = "SDFCore"; to_pin = "AOStr" }
        @{ from = "BaseCol"; to = "SDFCore"; to_pin = "BaseCol" }
        @{ from = "GlowCol"; to = "SDFCore"; to_pin = "GlowCol" }
        @{ from = "InkCol"; to = "SDFCore"; to_pin = "InkCol" }
    )
    outputs = @(
        @{ from = "SDFCore"; to_property = "EmissiveColor" }
    )
}

Build-SDFMaterial "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master" $mandelbulbGraph

Write-Host "`n=== Material 1 DONE ===" -ForegroundColor Green
