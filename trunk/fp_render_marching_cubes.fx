//--------------------------------------------------------------------------------------
// File: fp_render_marching_cubes.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct RenderIsoVolumeVSIn {
    float3 Pos  	      : POSITION;
    float3 Normal         : NORMAL;    
};

struct RenderIsoVolumePSIn {
    float4 Pos  	      : SV_POSITION;
    float4 Diffuse        : COLOR0; // vertex diffuse color
};

struct RenderIsoVolumePSOut {
    float4 Color		  : SV_TARGET;
};

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

cbuffer cbOnce {
    float4 g_MaterialAmbientColor;      // Material's ambient color
    float4 g_MaterialDiffuseColor;      // Material's diffuse color        
};

cbuffer cbSometimes {
    float4 g_LightDir[3];               // Light's direction in world space    
    float4 g_LightDiffuse[3];           // Light's diffuse color
    float4 g_LightAmbient;              // Light's ambient color
};

cbuffer cbEveryFrame {
    float4x4 g_ViewProjection;    // View * Projection matrix
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
    DepthFunc = LESS_EQUAL;
};

//--------------------------------------------------------------------------------------
// Vertex shader for marching cubes
// Input:  worldspace position, normal
// Output: clip space position, vertex color
// Transforms to clipspace, calculates n dot l lighting
//--------------------------------------------------------------------------------------
RenderIsoVolumePSIn RenderIsoVolumeVS(
        in RenderIsoVolumeVSIn Input,
        uniform int NumLights) {
	RenderIsoVolumePSIn output;
	
	output.Pos = mul(float4(Input.Pos,1), g_ViewProjection);
	
	// Compute simple directional lighting equation
	float3 totalLightDiffuse = float3(0,0,0);
	for(int i=0; i<NumLights; i++ )
        totalLightDiffuse += g_LightDiffuse[i].rgb * max(0,dot(Input.Normal,
                g_LightDir[i].xyz));
                
    output.Diffuse.rgb = g_MaterialDiffuseColor * totalLightDiffuse + 
                         g_MaterialAmbientColor * g_LightAmbient;   
    output.Diffuse.a = 1.0f; 
	
	return output;
}	

//--------------------------------------------------------------------------------------
// Pixel shader for marching cubes
// Input:  screen space position (not used), pixel color (both p. c. interpolated)
// Output: pixel color
// Does nothing
//--------------------------------------------------------------------------------------
RenderIsoVolumePSOut RenderIsoVolumePS(RenderIsoVolumePSIn Input)  { 
    RenderIsoVolumePSOut output;
	output.Color = Input.Diffuse;
    return output;
}


//--------------------------------------------------------------------------------------
// Renders scene to render target using D3D10 Techniques
//--------------------------------------------------------------------------------------
technique10 RenderIsoVolume1Light {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, RenderIsoVolumeVS(1)));
        SetGeometryShader( NULL );
        SetPixelShader(CompileShader(ps_4_0, RenderIsoVolumePS()));
                
        SetDepthStencilState( EnableDepth, 0 );
    }
}

technique10 RenderIsoVolume2Lights {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, RenderIsoVolumeVS(2)));
        SetGeometryShader( NULL );
        SetPixelShader(CompileShader(ps_4_0, RenderIsoVolumePS()));
                
        SetDepthStencilState( EnableDepth, 0 );
    }
}

technique10 RenderIsoVolume3Lights {
    pass P0 {
        SetVertexShader(CompileShader(vs_4_0, RenderIsoVolumeVS(3)));
        SetGeometryShader( NULL );
        SetPixelShader(CompileShader(ps_4_0, RenderIsoVolumePS()));
                
        SetDepthStencilState( EnableDepth, 0 );
    }
}