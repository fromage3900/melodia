. "g:\Melodia\Scripts\create_sdf_material.ps1"

# === Task 4.1: M_SDF_Mandelbulb_Master (FIXED - struct wrapper for HLSL functions) ===
$mandelbulbHlsl = @'
struct FSDF {
    float Power; float MaxIterations; float SliceOffset; float Scale; float AOStrength;
    float RotationSpeed; float Time;
    float3 BaseColor; float3 GlowColor; float3 InkColor;
    float2x2 rotM;
    float2 mandelbulbDE(float3 pos) {
        float3 z = pos; float dr = 1.0; float r = 0.0; float trap = 1e10;
        for(int i = 0; i < (int)MaxIterations; i++) {
            r = length(z); if(r > 2.0) break;
            float theta = acos(clamp(z.z / max(r, 1e-8), -1.0, 1.0));
            float phi = atan2(z.y, z.x);
            dr = pow(r, Power - 1.0) * Power * dr + 1.0;
            float zr = pow(r, Power); theta *= Power; phi *= Power;
            z = zr * float3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)) + pos;
            trap = min(trap, length(z.xy));
        }
        return float2(0.5 * log(max(r,1e-8)) * r / max(dr,1e-8), trap);
    }
    float2 sceneSDF(float3 p) {
        p.xz = mul(rotM, p.xz);
        p /= Scale;
        float2 md = mandelbulbDE(p);
        float d = md.x * Scale;
        d = max(d, -(p.z * Scale - SliceOffset));
        return float2(d, md.y);
    }
    float3 calcNormal(float3 p) {
        float2 e = float2(0.0005, 0.0);
        return normalize(float3(sceneSDF(p+e.xyy).x-sceneSDF(p-e.xyy).x, sceneSDF(p+e.yxy).x-sceneSDF(p-e.yxy).x, sceneSDF(p+e.yyx).x-sceneSDF(p-e.yyx).x));
    }
    float calcAO(float3 p, float3 n) {
        float ao = 0.0; float sc = 1.0;
        for(int i = 0; i < 5; i++) { float h = 0.01+0.05*(float)i; ao += (h-sceneSDF(p+n*h).x)*sc; sc *= 0.5; }
        return saturate(1.0 - 2.0*ao*AOStrength);
    }
    float3 render(float2 uv) {
        float2 uvc = (uv - 0.5) * 2.0;
        float3 ro = float3(0.0, 0.0, 3.0);
        float3 rd = normalize(float3(uvc * 0.65, -1.8));
        float ang = Time * RotationSpeed;
        rotM = float2x2(cos(ang), -sin(ang), sin(ang), cos(ang));
        float t = 0.0; float trap = 0.0; bool hit = false;
        for(int i = 0; i < 100; i++) {
            float3 p = ro + rd * t; float2 d = sceneSDF(p);
            if(d.x < 0.0005) { trap = d.y; hit = true; break; }
            if(t > 20.0) break; t += d.x * 0.7;
        }
        float3 col = InkColor;
        if(hit) {
            float3 p = ro + rd * t; float3 n = calcNormal(p); float ao = calcAO(p, n);
            float3 L = normalize(float3(0.5, 0.8, -0.6));
            float diff = saturate(dot(n, L)); float spec = pow(saturate(dot(reflect(rd, n), L)), 32.0);
            float fres = pow(1.0-saturate(dot(-rd, n)), 3.0);
            float3 matCol = lerp(BaseColor, GlowColor, saturate(trap*2.0));
            matCol = lerp(InkColor, matCol, 1.0-fres);
            col = matCol * (0.3+0.7*diff) * ao;
            col += spec * GlowColor * 0.5; col += fres * GlowColor * 0.15;
        }
        return col;
    }
};
FSDF S; S.Power=Power; S.MaxIterations=MaxIterations; S.SliceOffset=SliceOffset;
S.Scale=Scale; S.AOStrength=AOStrength; S.RotationSpeed=RotationSpeed; S.Time=Time;
S.BaseColor=BaseColor; S.GlowColor=GlowColor; S.InkColor=InkColor;
return S.render(UV);
'@

New-SDFMaterial -AssetPath "/Game/_PROJECT/04_Materials/SDF/M_SDF_Mandelbulb_Master" `
    -Description "Mandelbulb fractal raymarcher with orbit trap coloring" `
    -HlslCode $mandelbulbHlsl `
    -Params @(
        @{ Name = "UV"; Type = "texcoord"; Default = 0 },
        @{ Name = "Time"; Type = "time"; Default = 0 },
        @{ Name = "Power"; Type = "scalar"; Default = 8.0 },
        @{ Name = "MaxIterations"; Type = "scalar"; Default = 12.0 },
        @{ Name = "SliceOffset"; Type = "scalar"; Default = 0.0 },
        @{ Name = "RotationSpeed"; Type = "scalar"; Default = 0.1 },
        @{ Name = "Scale"; Type = "scalar"; Default = 1.5 },
        @{ Name = "AOStrength"; Type = "scalar"; Default = 0.6 },
        @{ Name = "BaseColor"; Type = "vector"; Default = 0 },
        @{ Name = "GlowColor"; Type = "vector"; Default = 0 },
        @{ Name = "InkColor"; Type = "vector"; Default = 0 }
    ) `
    -VectorDefaults @(
        @{ Name = "BaseColor"; Value = @(0.4, 0.2, 0.6, 1.0) },
        @{ Name = "GlowColor"; Value = @(1.0, 0.8, 0.3, 1.0) },
        @{ Name = "InkColor"; Value = @(0.05, 0.02, 0.1, 1.0) }
    )
