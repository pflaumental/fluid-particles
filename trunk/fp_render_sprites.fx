//--------------------------------------------------------------------------------------
// File: fp_render_sprites.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct RenderSpritesVSIn {
    float3 Pos            : POSITION;
};

struct RenderSpritesGSIn {
    float3 Pos            : POSITION;
};

struct RenderSpritesPSIn {
    float4 Pos			  : SV_POSITION;
    float2 Tex			  : TEXCOORD0;
};

struct RenderSpritesPSOut {
    float4 Color		  : SV_TARGET;
};

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

cbuffer cbPerObject {
    matrix g_WorldView;
    matrix g_Proj;
};

cbuffer cbUser {
    float g_SpriteSize;
};

cbuffer cbImmutable {
    float3 g_ParticlePositions[4] = {
        float3( -1, 1, 0 ),
        float3( 1, 1, 0 ),
        float3( -1, -1, 0 ),
        float3( 1, -1, 0 ),
    };
    float2 g_ParticleTexcoords[4] = { 
        float2(0,0), 
        float2(1,0),
        float2(0,1),
        float2(1,1),
    };
};

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

Texture2D g_ParticleDiffuse;

//--------------------------------------------------------------------------------------
// Texture samplers
//--------------------------------------------------------------------------------------

SamplerState g_LinearClamp {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// BlendStates
//--------------------------------------------------------------------------------------

BlendState AlphaBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

BlendState NoBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};

//--------------------------------------------------------------------------------------
// DepthStencilStates
//--------------------------------------------------------------------------------------

DepthStencilState EnableDepth {
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
};

DepthStencilState DisableDepth {
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

DepthStencilState DisableDepthWrite {
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
};

DepthStencilState DisableDepthTest {
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = ALWAYS;
};

//--------------------------------------------------------------------------------------
// Vertex shader for particles:
// Transforms positions into view-space
//--------------------------------------------------------------------------------------
RenderSpritesGSIn RenderSpritesVS(RenderSpritesVSIn Input) {
    RenderSpritesGSIn output;
    output.Pos = mul(float4(Input.Pos,1), g_WorldView);
    return output;    
}

//--------------------------------------------------------------------------------------
// Geometry shader for particles:
// Outputs 2 triangles with texture coordinates for each particle
//--------------------------------------------------------------------------------------
[maxvertexcount(4)]
void RenderSpritesGS(
		point RenderSpritesGSIn Input[1], 
		inout TriangleStream<RenderSpritesPSIn> SpriteStream) {
	RenderSpritesPSIn output;
	[unroll] for(int i=0; i<4; i++) {
		float3 outPosView = Input[0].Pos + g_ParticlePositions[i] * g_SpriteSize;
		output.Pos = mul(float4(outPosView,1), g_Proj);
		output.Tex = g_ParticleTexcoords[i];
		SpriteStream.Append(output);
	}
	SpriteStream.RestartStrip();
}	

//--------------------------------------------------------------------------------------
// Pixel shader for particles:
// Lookups the diffuse color in particle texture
//--------------------------------------------------------------------------------------
RenderSpritesPSOut RenderSpritesPS(RenderSpritesPSIn Input)  { 
    RenderSpritesPSOut output;
	output.Color = g_ParticleDiffuse.Sample(g_LinearClamp, Input.Tex);
    return output;
}


//--------------------------------------------------------------------------------------
// Renders scene to render target using D3D10 Techniques
//--------------------------------------------------------------------------------------
technique10 RenderSprites {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, RenderSpritesVS()));
        SetGeometryShader(CompileShader(gs_4_0, RenderSpritesGS()));
        SetPixelShader(CompileShader(ps_4_0, RenderSpritesPS()));

        SetBlendState(AlphaBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepthWrite, 0 );
    }
}
