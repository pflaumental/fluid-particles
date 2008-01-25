//--------------------------------------------------------------------------------------
// File: fp_render_raycast.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct SplatParticleVSIn {
    float4 ParticlePosDensity       : POSITION_DENSITY;
    uint ParticleSlice              : SV_InstanceID;    
};

struct SplatParticleGSIn {
    float4 ParticlePosTexZDensity    : POSITION_TEXZ_DENSITY;
    int VolumeSlice                  : VOLUMESLICE;
};

struct SplatParticlePSIn {
    float4 VoxelPos                 : SV_Position;
    float ParticleDensity           : DENSITY;
    uint VolumeSlice                : SV_RenderTargetArrayIndex;
    float3 VoxelTex                 : TEXCOORDS;     
};

struct SplatParticlePSOut {
    half IsoValue                   : SV_TARGET;
};

struct RaycastTransformVSOut {
    float4 VolumePosDepth           : VOLUMEPOS_DEPTH;
    float4 Pos                      : SV_POSITION;
};

struct RaycastTraceIsoPSOut {
    float4 Position                 : SV_Target0;
    float4 Normal                   : SV_Target1;
};

struct RaycastComposePSOut {
    float4 Color                    : SV_Target0;
    float Depth                     : SV_Depth;
};

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

cbuffer cbImmutable {
    float2 g_CornersTex[4] = { 
        float2(-1, 1), 
        float2( 1, 1),
        float2(-1,-1),
        float2( 1,-1),
    };
};

cbuffer cbOnce {
    int3 g_VolumeDimensions; // volume dimension in number of voxels
};

cbuffer cbSometimes {
    int g_ParticleVoxelRadius;
    float g_HalfParticleVoxelDiameter;
    float4 g_CornersPos[4]; // normalized device space corner offsets    
};

cbuffer cbOften {
    float4x4 g_WorldToNDS;
};

//*cbuffer cbEveryFrame {
//*};

//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------

SamplerState LinearBorder {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    BorderColor = float4(0, 0, 0, 0);
};

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

Texture1D g_WValsMulParticleMass;

//--------------------------------------------------------------------------------------
// RasterizerStates
//--------------------------------------------------------------------------------------

RasterizerState CullFront {
    CullMode = Front;
};

RasterizerState CullBack {
    CullMode = Back;
};

RasterizerState CullNone {
    CullMode = None;
};

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------

BlendState AlphaBlending {
    AlphaToCoverageEnable = FALSE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    
    BlendEnable[0] = TRUE;
    BlendEnable[1] = TRUE;
    BlendEnable[2] = TRUE;
    BlendEnable[3] = TRUE;
    BlendEnable[4] = TRUE;
    BlendEnable[5] = TRUE;
    BlendEnable[6] = TRUE;
    BlendEnable[7] = TRUE;
    
    RenderTargetWriteMask[0] = 0x0F;
    RenderTargetWriteMask[1] = 0x0F;
    RenderTargetWriteMask[2] = 0x0F;
    RenderTargetWriteMask[3] = 0x0F;
    RenderTargetWriteMask[4] = 0x0F;
    RenderTargetWriteMask[5] = 0x0F;
    RenderTargetWriteMask[6] = 0x0F;
    RenderTargetWriteMask[7] = 0x0F;   
};

BlendState AdditiveBlending
{
    AlphaToCoverageEnable = FALSE;    
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = ADD;
    
    BlendEnable[0] = TRUE;
    BlendEnable[1] = TRUE;
    BlendEnable[2] = TRUE;
    BlendEnable[3] = TRUE;
    BlendEnable[4] = TRUE;
    BlendEnable[5] = TRUE;
    BlendEnable[6] = TRUE;
    BlendEnable[7] = TRUE;
    
    RenderTargetWriteMask[0] = 0x0F;
    RenderTargetWriteMask[1] = 0x0F;
    RenderTargetWriteMask[2] = 0x0F;
    RenderTargetWriteMask[3] = 0x0F;
    RenderTargetWriteMask[4] = 0x0F;
    RenderTargetWriteMask[5] = 0x0F;
    RenderTargetWriteMask[6] = 0x0F;
    RenderTargetWriteMask[7] = 0x0F;    
}; 

BlendState BlendOver {
    AlphaToCoverageEnable = FALSE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = ADD;

    BlendEnable[0]			 = TRUE;
    BlendEnable[1]			 = TRUE;
    BlendEnable[2]			 = TRUE;
    BlendEnable[3]			 = TRUE;
    BlendEnable[4]			 = TRUE;
    BlendEnable[5]			 = TRUE;
    BlendEnable[6]			 = TRUE;
    BlendEnable[7]			 = TRUE;

    RenderTargetWriteMask[0] = 0x0F;
	RenderTargetWriteMask[1] = 0x0F;
    RenderTargetWriteMask[2] = 0x0F;
    RenderTargetWriteMask[3] = 0x0F;
    RenderTargetWriteMask[4] = 0x0F;
    RenderTargetWriteMask[5] = 0x0F;
    RenderTargetWriteMask[6] = 0x0F;
    RenderTargetWriteMask[7] = 0x0F;
};

BlendState NoBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
    BlendEnable[1] = FALSE;
    BlendEnable[2] = FALSE;
    BlendEnable[3] = FALSE;
    BlendEnable[4] = FALSE;
    BlendEnable[5] = FALSE;
    BlendEnable[6] = FALSE;
    BlendEnable[7] = FALSE;
};

//--------------------------------------------------------------------------------------
// DepthStencilStates
//--------------------------------------------------------------------------------------

DepthStencilState EnableDepth {
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState DisableDepth {
    DepthEnable = false;
    DepthWriteMask = ZERO;
    DepthFunc = Less;

    //Stencil
    StencilEnable = false;
    StencilReadMask = 0xFF;
    StencilWriteMask = 0x00;
};

//--------------------------------------------------------------------------------------
// Vertex shader for particle splatting
// Input:  world space particle position, slice index inside particle, ParticleDensity
// Output: normalized device space particle position, slice index inside volume,
//         ParticleDensity
// Transforms particle position to n. d. space, calculates slice index inside volume
//--------------------------------------------------------------------------------------
SplatParticleGSIn SplatParticleVS(in SplatParticleVSIn Input) {
    SplatParticleGSIn result;
    result.ParticlePosTexZDensity = mul(float4(Input.ParticlePosDensity.xyz, 1),
            g_WorldToNDS);
    result.VolumeSlice = int((0.5 * result.ParticlePosTexZDensity.z + 0.5)
            * float(g_VolumeDimensions.z)) - g_ParticleVoxelRadius 
            + int(Input.ParticleSlice);
    result.ParticlePosTexZDensity.z = 1 - float(Input.ParticleSlice)
            / g_HalfParticleVoxelDiameter;
    result.ParticlePosTexZDensity.w = Input.ParticlePosDensity.w;            
    return result;
}

//--------------------------------------------------------------------------------------
// Geometry shader for particle splatting
// Input:  vertex shader for particle splatting output
// Output: two n. d. space triangles (z=0), texture space corner positions, slice index
//         inside volume (render target index), ParticleDensity
// Computes n. d. space- (x,y) and texture space- (x,y,z) positions of corners.
// Volume slice index becomes render target index.
//--------------------------------------------------------------------------------------
[maxvertexcount (4)]
void SplatParticleGS(
        point SplatParticleGSIn Input[1], 
        inout TriangleStream<SplatParticlePSIn> SpriteStream) {        
	if (Input[0].VolumeSlice < 0 || Input[0].VolumeSlice >= g_VolumeDimensions.z)
	    return;
    SplatParticlePSIn output;
    output.VolumeSlice = Input[0].VolumeSlice;
    output.ParticleDensity = Input[0].ParticlePosTexZDensity.w;
	[unroll] for(int i=0; i<4; i++) {
	    output.VoxelPos = float4(g_CornersPos[i].xy
	            + Input[0].ParticlePosTexZDensity.xy, 0, 1);
		output.VoxelTex = float3(g_CornersTex[i],
		        Input[0].ParticlePosTexZDensity.z);
		SpriteStream.Append(output);
	}
	SpriteStream.RestartStrip();	
}

//--------------------------------------------------------------------------------------
// Pixel shader for particle splatting
// Input:  ParticleDensity, slice index inside volume (z), screen/volume space voxel
//         position (x,y) and texture space (x,y,z) voxel position
// Output: Isovalue
// Fetches the smoothing function value from a 1D texture and calculates the additive
// isovalue for this voxel and particle
//--------------------------------------------------------------------------------------
SplatParticlePSOut SplatParticlePS(SplatParticlePSIn Input) {
    SplatParticlePSOut result;
    float l = length(Input.VoxelTex);
    float wValMulParticleMass = g_WValsMulParticleMass.SampleLevel(LinearBorder, l, 0);
    result.IsoValue = wValMulParticleMass / Input.ParticleDensity;
    return result;
}


//--------------------------------------------------------------------------------------
// Vertex shader for transform
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
RaycastTransformVSOut RaycastTransformVS(in float3 Position : POSITION) {
    RaycastTransformVSOut result;
	//*// scale to [0,1]
    //*result.VolumePosDepth.xyz = (Position - g_BBoxMin) / g_BBoxSize;
    //*// re-scale to [1.5*texdelta,1-1.5*texdelta] for correct texture addressing (3 voxels overlap)
    //*result.VolumePosDepth.xyz *= 1.0 - 3.0 * g_TexDelta;
    //*result.VolumePosAndDepth.xyz += 1.5 * g_TexDelta;    
    //*Position *= g_Aspect;
    //*result.Pos = mul(float4(Position, 1), g_WorldViewProjection);
    //*result.VolumePosDepth.w = result.Pos.z;
    
    result.Pos = float4(0, 0, 0, 0);
    result.VolumePosDepth = float4(0, 0, 0, 0);
    return result;
}

//--------------------------------------------------------------------------------------
// Vertex shader that does nothing
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
float4 RaycastNullVS(in float3 Position : POSITION) : SV_POSITION {
    //*Position *= g_vAspect;
    //*return mul(float4(Position, 1), g_mWorldViewProjection);
    return float4(0, 0, 0, 0);
}

//--------------------------------------------------------------------------------------
// Pixel shader for finding the rayexit
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
float4 RaycastExitPS(RaycastTransformVSOut Input) : SV_Target {
    return Input.VolumePosDepth;
}

//--------------------------------------------------------------------------------------
// Pixel shader for tracing the iso volume
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
RaycastTraceIsoPSOut RaycastTraceIsoPS(RaycastTransformVSOut Input) {
    RaycastTraceIsoPSOut result;
    //*float4 vOldVal = g_txVolPos0.Load(int3(pos.xy,0));
    //*if(vOldVal.a != 0)
		//*discard;
    //*
    //*float4 vExitAndDepth = g_txExitPoint.Load(int3(pos.xy,0));
//*
    //*// ray entry, exit and direction
    //*float3 volumePos      = volumePosAndDepth.xyz;
    //*float3 vExit          = vExitAndDepth.xyz;
    //*float3 vDir           = vExit - volumePos;
    //*float  fDirLen        = length(vDir);
    //*float  fLocalStepsize = bPerPixelStepsize ? GetLocalStepsize(vDir, fDirLen) : g_fStepsize;
    //*vDir                  = vDir / fDirLen * fLocalStepsize;
	//*
    //*position0 = 0;
    //*normal0 = 0;
    //*
    //*int numSteps = ceil(fDirLen / fLocalStepsize);
    //*while(--numSteps >= 0) {
        //*float voxel = GetScalar(volumePos, g_txVolume);
        //*
        //*if (voxel >= g_fIsoVal)
			//*break;
		//*
        //*volumePos += vDir;
    //*}
    //*if(numSteps < 0) {
        //*float voxel = GetScalar(vExit, g_txVolume);
        //*
        //*if (voxel >= g_fIsoVal) {
			//*volumePos = vExit;
			//*numSteps = 0;
		//*}
    //*}
    //*
	//*// refine isosurface
	//*if(numSteps >= 0) {
		//*position0 = RefineIsosurface(vDir, volumePos, g_txVolume, g_fIsoVal);
		//*
		//*// compute depth 		
		//*position0.a = volumePosAndDepth.a + length(volumePosAndDepth.xyz - position0.xyz)/fDirLen * (vExitAndDepth.a - volumePosAndDepth.a);
		//*
		//*// generate normal
		//*float3 grad;
		//*float3 vTexDeltaGlobal = g_vTexDelta * (g_vBBoxSize/(1-3*g_vTexDelta)) * g_vAspect;
		//*grad.x = (GetScalar(position0.xyz+float3(g_vTexDelta.x,0,0),g_txVolume)-
				  //*GetScalar(position0.xyz-float3(g_vTexDelta.x,0,0),g_txVolume))/(2*vTexDeltaGlobal.x);
		//*grad.y = (GetScalar(position0.xyz+float3(0,g_vTexDelta.y,0),g_txVolume)-
				  //*GetScalar(position0.xyz-float3(0,g_vTexDelta.y,0),g_txVolume))/(2*vTexDeltaGlobal.y);
		//*grad.z = (GetScalar(position0.xyz+float3(0,0,g_vTexDelta.z),g_txVolume)-
				  //*GetScalar(position0.xyz-float3(0,0,g_vTexDelta.z),g_txVolume))/(2*vTexDeltaGlobal.z);
		//*
		//*float3 normal = normalize(mul(normalize(grad), (float3x3)g_mWorldView));	
		//*normal0 = float4(normal, position0.a);
		//*
		//*// rescale position to global box dimensions
		//*position0.xyz -= 1.5 * g_vTexDelta;
		//*position0.xyz /= 1.0 - 3.0 * g_vTexDelta;
		//*position0.xyz = (position0.xyz * g_vBBoxSize) + g_vBBoxMin;
	//*}
    result.Position = float4(0, 0, 0, 0);
    result.Normal = float4(0, 0, 0, 0);
    return result;	
}

//--------------------------------------------------------------------------------------
// Pixel shader for illumination
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
float4 RaycastIlluminatePS(float4 Position : SV_POSITION) : SV_Target0 {
    float4 color = float4(0, 0, 0, 0);
    //*float4 normal = g_txNormal0.Load(int3(pos.xy,0));
    //*
    //*if (normal.a == 0) discard;
    //*
	//*color0 = float4(saturate(dot(g_vLightDir,normal.xyz)),0,0,normal.a);
//*
	//*if (g_fEdges > 0) {
		//*float curv = length(
					   //*abs(g_txNormal0.Load(int3(pos.x+1,pos.y,0))-normal.xyz) +
					   //*abs(g_txNormal0.Load(int3(pos.x-1,pos.y,0))-normal.xyz) +
					   //*abs(g_txNormal0.Load(int3(pos.x,pos.y+1,0))-normal.xyz) +
					   //*abs(g_txNormal0.Load(int3(pos.x,pos.y-1,0))-normal.xyz)
					 //*);
					 //*
		//*color.x -= g_fEdges*curv;
	//*}
//*
//*
	//*// TODO disable if no stippling
	//*// encode stipple or not in y
	//*float4 VolumePos = g_txVolPos0.Load(int3(pos.xy,0));
	//*float random = g_txStipple.Sample(smPointWrap, VolumePos*g_vStippleScale).x;
	//*color.y =  ( color0.x > random ) ? 1 : 0;
	return color;
}

//--------------------------------------------------------------------------------------
// Pixel shader for composition
// Input:  -
// Output: -
// -
//--------------------------------------------------------------------------------------
RaycastComposePSOut RaycastComposePS(float4 Position : SV_POSITION) {
    RaycastComposePSOut result;
    //*float4 last = g_txColor0.Load(int3(pos.xy,0));
    //*
    //*if (last.a == 0) discard;
	//*
	//*if (bStipple) {
		//*color0 = float4(last.yyy,1);
				//*
		//*if (last.y == 1) {
			//*float nCount = g_txColor0.Load(int3(pos.x+1,pos.y,0)).y +
						   //*g_txColor0.Load(int3(pos.x-1,pos.y,0)).y +
						   //*g_txColor0.Load(int3(pos.x,pos.y+1,0)).y +
						   //*g_txColor0.Load(int3(pos.x,pos.y-1,0)).y;
			//*
			//*if (nCount < 4) color0 = float4(( 1-(4-nCount)*0.1).xxx,1);
		//*}
		//*
		//*
	//*} else color0 = float4(g_vColor0 * last.x, 1);
    //*
	//*depth = (-g_vDepthParam.z/last.a + g_vDepthParam.y)/g_vDepthParam.w;
	result.Color = float4(0, 0, 0, 0);
	result.Depth = 0;
	return result;
}



//--------------------------------------------------------------------------------------
// D3D10 Techniques
//--------------------------------------------------------------------------------------

technique10 RenderRaycast {
    pass P0_SplatParticle {
        SetVertexShader(CompileShader(vs_4_0, SplatParticleVS()));
        SetGeometryShader(CompileShader(gs_4_0, SplatParticleGS()));
        SetPixelShader(CompileShader(ps_4_0, SplatParticlePS()));

        SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullNone);
    }
    
    pass P1_FindRayExit {
        SetVertexShader(CompileShader(vs_4_0, RaycastTransformVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastExitPS()));  
          
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullFront);
    }
    
    pass P2_IsoSurface {
        SetVertexShader(CompileShader( vs_4_0, RaycastTransformVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastTraceIsoPS()));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullBack);        
    }
    
    pass P3_Illuminate {
        SetVertexShader(CompileShader(vs_4_0, RaycastNullVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastIlluminatePS()));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullBack);        
    }
    
    pass P4_Compose {
        SetVertexShader(CompileShader(vs_4_0, RaycastNullVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastComposePS()));
        
        SetBlendState(BlendOver, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullBack);
    }    
}