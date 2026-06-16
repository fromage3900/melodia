$baseUrl = "http://localhost:9316/mcp"
$ct = "application/json"
$path = "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master"

function Call-Mono($action, $extraArgs) {
    $arguments = [ordered]@{ action = $action }
    foreach ($key in $extraArgs.Keys) { $arguments[$key] = $extraArgs[$key] }
    $body = @{
        jsonrpc = "2.0"; id = 1; method = "tools/call"
        params = @{ name = "material_query"; arguments = $arguments }
    } | ConvertTo-Json -Depth 20 -Compress
    $r = Invoke-RestMethod -Uri $baseUrl -Method Post -Body $body -ContentType $ct -TimeoutSec 60
    $text = $r.result.content | Where-Object { $_.type -eq "text" } | Select-Object -First 1
    if ($text) { return $text.text }
    return ($r | ConvertTo-Json -Depth 3 -Compress)
}

# Step 1: Update the Custom HLSL node with proper inputs
$hlslCode = @"
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
"@

Write-Host "Updating custom HLSL node with inputs..."
$r = Call-Mono "update_custom_hlsl_node" @{
    asset_path = $path
    expression_name = "MaterialExpressionCustom_0"
    code = $hlslCode
    inputs = @(
        @{ name = "UV" }
        @{ name = "Time" }
        @{ name = "Power" }
        @{ name = "MaxIter" }
        @{ name = "SliceOff" }
        @{ name = "RotSpd" }
        @{ name = "ScaleParam" }
        @{ name = "AOStr" }
        @{ name = "BaseCol" }
        @{ name = "GlowCol" }
        @{ name = "InkCol" }
    )
}
Write-Host "Update result: $r" -ForegroundColor Cyan

# Step 2: Check if inputs were created
Write-Host "`nChecking expression details..."
$r = Call-Mono "get_expression_details" @{
    asset_path = $path
    expression_name = "MaterialExpressionCustom_0"
}
Write-Host "Details: $r" -ForegroundColor Yellow
