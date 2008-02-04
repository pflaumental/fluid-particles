//--------------------------------------------------------------------------------------
// File: fp_render_raycast.fx
//--------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

cbuffer Immutable {
    float2 g_CornersTex[4] = { 
        float2(-1, 1), 
        float2( 1, 1),
        float2(-1,-1),
        float2( 1,-1),
    };
};

cbuffer Once {
    int3 g_VolumeDimensions; // volume dimension in number of voxels
    float3 g_VolumeSizeRatio; // contains ratio "size / sizeMax" for each dimension
    float3 g_TexDelta;
    float g_StepSize;
    int g_NumRefineSteps = 5;
    float3 g_MaterialColor = float3(0.75, 0.75, 1.0); // TODO remove color hack
};

cbuffer Sometimes {
    int g_ParticleVoxelRadius;
    float g_HalfParticleVoxelDiameter;
    float4 g_CornersPos[4]; // normalized device space corner offsets
    float3 g_BBoxSize;
    float g_IsoLevel;
};

cbuffer Often {
    float4x4 g_WorldToNDS;
    float4x4 g_World;
    float4x4 g_WorldView;
    float4x4 g_WorldViewProjection;
    float3 g_BBoxStart;
    float3 g_LightDir = float3(0,1,0); // TODO remove light hack    
};

//*cbuffer cbEveryFrame {
//*};





//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

Texture1D g_WValsMulParticleMass;
Texture2D g_ExitPoint;
Texture2D g_IntersectionPosition0;
Texture2D g_IntersectionPosition1;
Texture2D g_IntersectionNormal0;
Texture2D g_IntersectionNormal1;
Texture3D g_IsoVolume;
TextureCube g_Environment;





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
    float4 VolumePosClipDepth       : VOLUMEPOS_CLIPDEPTH;
    float4 Pos                      : SV_POSITION;
};

struct RaycastTraceIsoAndShadePSOut {
    float4 Color                    : SV_Target0;
    float Depth                     : SV_Depth;
};





//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------

SamplerState LinearBorder {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    BorderColor = float4(0, 0, 0, 0);
};

SamplerState LinearPointClamp
{
    Filter      = MIN_MAG_LINEAR_MIP_POINT;
    AddressU    = Clamp;
    AddressV    = Clamp;
    AddressW    = Clamp;
};

SamplerState LinearClamp
{
    Filter      = MIN_MAG_MIP_LINEAR;
    AddressU    = Clamp;
    AddressV    = Clamp;
    AddressW    = Clamp;
};

//--------------------------------------------------------------------------------------
// Shaders
//--------------------------------------------------------------------------------------

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
    result.ParticlePosTexZDensity.z = -(1 - float(Input.ParticleSlice)
            / g_HalfParticleVoxelDiameter);
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
// Vertex shader for transformations
// Input:  worldspace vertex position
// Output: volume space position, clip space depth, clip space position
// -
//--------------------------------------------------------------------------------------
RaycastTransformVSOut RaycastTransformVS(in float3 Position : POSITION) {
    RaycastTransformVSOut result;
	// scale to [0,1]
    result.VolumePosClipDepth.xyz = Position / g_BBoxSize;    
    //*Position *= g_Aspect;
    result.Pos = mul(float4(Position, 1), g_WorldViewProjection);
    result.VolumePosClipDepth.w = result.Pos.z;    
    return result;
}

//--------------------------------------------------------------------------------------
// Pixel shader for finding the rayexit
// Input:  volume space position, clip space depth, screen space position
// Output: volume space position, clip space depth
// -
//--------------------------------------------------------------------------------------
float4 RaycastExitPS(RaycastTransformVSOut Input) : SV_Target {
    return Input.VolumePosClipDepth;
}

// Helper function for calculating the local stepsize
float CalcLocalStepsize(float3 RayDir, float RayDirLen) {
	float3 scaledRayDir = RayDir * g_VolumeSizeRatio;
	return g_StepSize * RayDirLen / length(scaledRayDir);
}

// Function for refining the iso trace
float3 RefineIsosurface(float3 TextureOffset, float3 SampleTexturePos) {
	TextureOffset /= 2;
	SampleTexturePos -= TextureOffset;
	for (int i = 0; i < g_NumRefineSteps; i++) {
		TextureOffset /= 2;
		float isoVal = g_IsoVolume.SampleLevel(LinearPointClamp, SampleTexturePos, 0).r;
		if (isoVal >= g_IsoLevel)
			SampleTexturePos -= TextureOffset;
		else
			SampleTexturePos += TextureOffset;
	}	
	return float3(SampleTexturePos);
}

//--------------------------------------------------------------------------------------
// Function for tracing iso surface
// Input:  volume space position, clip space depth, screen space position
// Output: worldspace position and normal of the first intersection
// Traces the iso volume along the view ray to find the first intersection.
// If Intersection is found it get's refined.
//--------------------------------------------------------------------------------------
void RaycastTraceIso(
        in RaycastTransformVSOut Input,        
        out float4 IntersectionNormalDepth,
        out float3 RayDirection,
        bool PerPixelStepsize) {
    IntersectionNormalDepth = float4(0,0,0,-1);

    float4 entryVolumePosClipDepth = Input.VolumePosClipDepth;
    float4 exitVolumePosClipDepth = g_ExitPoint.Load(
            int3(Input.Pos.xy, 0));

    // ray entry, exit and direction           
    float3 exitVolumePos = exitVolumePosClipDepth.xyz;
    float3 textureOffset = exitVolumePos - entryVolumePosClipDepth.xyz;
    RayDirection = textureOffset;
    float volumeRayLen = length(textureOffset);
    float localStepsize = PerPixelStepsize
            ? CalcLocalStepsize(textureOffset, volumeRayLen)
            : g_StepSize;
            
    float3 sampleTexturePos = entryVolumePosClipDepth.xyz;
    textureOffset /= volumeRayLen;
    textureOffset *= localStepsize;
    sampleTexturePos.y = 1 - sampleTexturePos.y; // y in volume is 1-y in texture
    textureOffset.y *= -1; // offset.y must therefore get flipped
    
    int numSteps = ceil(volumeRayLen / localStepsize);
    float isoVal;    
    while(numSteps-- > 0) {
        float isoVal = g_IsoVolume.SampleLevel(LinearClamp, sampleTexturePos, 0).r;
        if (isoVal >= g_IsoLevel)
			break;
        sampleTexturePos += textureOffset;
    }
    if(numSteps <= 0) {
        float3 exitTexturePos = exitVolumePos;
        exitTexturePos.y = 1 - exitTexturePos.y;
        isoVal = g_IsoVolume.SampleLevel(LinearClamp, exitTexturePos, 0).r;
        if (isoVal >= g_IsoLevel) {
			sampleTexturePos = exitTexturePos;
			numSteps = 1;
		}
    }    
	// if intersection found
	if(numSteps > 0) {
	    // refine isosurface
		sampleTexturePos = RefineIsosurface(textureOffset, sampleTexturePos);
		float3 intersectionVolumePos = sampleTexturePos;
		intersectionVolumePos.y = 1 - intersectionVolumePos.y;
		// compute depth 		
		IntersectionNormalDepth.w = entryVolumePosClipDepth.w
		        + length(entryVolumePosClipDepth.xyz - intersectionVolumePos)
		        / volumeRayLen * (exitVolumePosClipDepth.w - Input.VolumePosClipDepth.w);
		// generate normal
		float3 grad;
		grad.x = g_IsoVolume.SampleLevel(LinearClamp,
		        sampleTexturePos + float3(g_TexDelta.x, 0, 0), 0)
		        -
                g_IsoVolume.SampleLevel(LinearClamp,
                sampleTexturePos - float3(g_TexDelta.x, 0, 0), 0);
		grad.y = g_IsoVolume.SampleLevel(LinearClamp,
		        sampleTexturePos - float3(0, g_TexDelta.y, 0), 0)
		        -
                g_IsoVolume.SampleLevel(LinearClamp,
                sampleTexturePos + float3(0, g_TexDelta.y, 0), 0);
		grad.z = g_IsoVolume.SampleLevel(LinearClamp,
		        sampleTexturePos + float3(0, 0, g_TexDelta.z), 0)
		        -
                g_IsoVolume.SampleLevel(LinearClamp,
                sampleTexturePos - float3(0, 0 ,g_TexDelta.z), 0);        
		IntersectionNormalDepth.xyz = -normalize(grad);
	}    
}

void ShadeIso(
        in float4 NormalDepth,
        in float3 RayDirection,
        out float4 Color,
        out float Depth) {
    float3 reflectDir = RayDirection - 2 * dot(NormalDepth.xyz, RayDirection) 
            * NormalDepth.xyz;
    float3 reflectionColor = g_Environment.Sample(LinearClamp, reflectDir);
    Color = float4(reflectionColor/* * g_MaterialColor*/, 1);
    Depth = NormalDepth.w;
}

//--------------------------------------------------------------------------------------
// Pixel shader for tracing and shading the iso surface
// Input:  volume space position, clip space depth, screen space position
// Output: worldspace position and normal of the first intersection
// comment TODO
//--------------------------------------------------------------------------------------
RaycastTraceIsoAndShadePSOut RaycastTraceIsoAndShadePS(
        RaycastTransformVSOut Input,
        uniform bool PerPixelStepSize) {
    RaycastTraceIsoAndShadePSOut result;   
    
    float4 intersectionNormalClipDepth;
    float3 rayDirection;
    RaycastTraceIso(Input, intersectionNormalClipDepth, rayDirection, PerPixelStepSize);
            
    if(intersectionNormalClipDepth.w == -1)
        discard;
                        
    ShadeIso(intersectionNormalClipDepth, rayDirection, result.Color, result.Depth);

    return result;	
}

//--------------------------------------------------------------------------------------
// Vertex shader for environment rendering
//--------------------------------------------------------------------------------------
void EnvironmentVS(
        in float3 PositionIn : POSITION,
        out float4 PositionOut : SV_POSITION,
        out float3 TexCoordsOut : TEXCOORD0) {
    PositionOut = mul(float4(PositionIn, 1), g_WorldViewProjection);
    TexCoordsOut = mul(float4(PositionIn, 1), g_World).xyz;
}

//--------------------------------------------------------------------------------------
// Pixel shader for environment rendering
//--------------------------------------------------------------------------------------
void EnvironmentPS(in float4 Position : SV_POSITION,
        in float3 TexCoords : TEXCOORD0,
        out float4 Color : SV_TARGET) {
    Color = g_Environment.Sample(LinearClamp, TexCoords);
}





//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Rasterizer states
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
// Blend states
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
// DepthStencil states
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
// Techniques
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

    pass P2_TraceIsoSurfaceAndShade {
        SetVertexShader(CompileShader( vs_4_0, RaycastTransformVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastTraceIsoAndShadePS(false)));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullBack);        
    }
    
    pass P3_TraceIsoSurfaceAndShadeWidthPerPixelStepsize {
        SetVertexShader(CompileShader( vs_4_0, RaycastTransformVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RaycastTraceIsoAndShadePS(true)));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullBack);        
    }
    
    pass P4_RenderEnvironment {
        SetVertexShader(CompileShader( vs_4_0, EnvironmentVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, EnvironmentPS()));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0);
        SetRasterizerState(CullNone);        
    }    
}