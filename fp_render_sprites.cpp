#include "DXUT.h"
#include "SDKmisc.h"
#include "fp_render_sprites.h"
#include "fp_util.h"
#include <tools/DX10Tools.h>

const D3D10_INPUT_ELEMENT_DESC fp_SpriteVertex::Layout[] = { { "POSITION",  0,
        DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 } };

fp_RenderSprites::fp_RenderSprites(int NumParticles, fp_FluidParticle* Particles)
        :
        m_NumParticles(NumParticles),
        m_Particles(Particles),
        m_SpriteSize(FP_RENDER_DEFAULT_SPRITE_SIZE),
        m_SpriteColor(1.0f, 1.0f, 1.0f, 1.0f),
		m_Texture9(NULL),
        m_VertexBuffer10(NULL),
		m_Texture10RV(NULL),
        m_Effect10(NULL),
        m_TechRenderSprites(NULL),
        m_EffectSpriteSize(NULL),
        m_EffectSpriteColor(NULL),
        m_EffectTexture(NULL),
        m_EffectWorldViewProjection(NULL),
        m_EffectWorld(NULL),
        m_VertexLayout(NULL){
}

fp_RenderSprites::~fp_RenderSprites() {
	OnD3D9LostDevice(NULL);
	OnD3D9DestroyDevice(NULL);
    OnD3D10ReleasingSwapChain(NULL);
    OnD3D10DestroyDevice(NULL);
}

float fp_RenderSprites::GetSpriteSize() const {
    return m_SpriteSize;
}

void fp_RenderSprites::SetSpriteSize(float SpriteSize) {
    m_SpriteSize = SpriteSize;
    if(m_EffectSpriteSize != NULL)
        m_EffectSpriteSize->SetFloat(SpriteSize);
}

HRESULT fp_RenderSprites::OnD3D9CreateDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Media/PointSprites/particle.bmp" ) );
    D3DXCreateTextureFromFile( d3dDevice, str, &m_Texture9 );
    
    return S_OK;
}

HRESULT fp_RenderSprites::OnD3D9ResetDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    d3dDevice->CreateVertexBuffer( m_NumParticles * sizeof(fp_SpriteVertex), 
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY | D3DUSAGE_POINTS, 
            fp_SpriteVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer9, NULL );
    return S_OK;
}

void fp_RenderSprites::OnD3D9FrameRender(
        IDirect3DDevice9* d3dDevice,
        double Time,
        float ElapsedTime,
        const D3DXVECTOR3* EyePt,
        const D3DXMATRIX*  WorldViewProjection,
        const D3DXMATRIX*  World,
        const D3DXMATRIX*  View,
        const D3DXMATRIX*  Proj,
        int NumActiveLights,
        int ActiveLight,
        float LightScale) {
    d3dDevice->SetRenderState( D3DRS_LIGHTING,  FALSE );
    d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
    d3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
    d3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    d3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
    d3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
    d3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  TRUE );
    d3dDevice->SetRenderState( D3DRS_POINTSIZE,     fp_Util::FtoDW(m_SpriteSize) );
    d3dDevice->SetRenderState( D3DRS_POINTSIZE_MIN, fp_Util::FtoDW(1.0f) );
    d3dDevice->SetRenderState( D3DRS_POINTSCALE_A,  fp_Util::FtoDW(0.0f) );
    d3dDevice->SetRenderState( D3DRS_POINTSCALE_B,  fp_Util::FtoDW(0.0f) );
    d3dDevice->SetRenderState( D3DRS_POINTSCALE_C,  fp_Util::FtoDW(1.0f) );

	fp_SpriteVertex *pSpriteVertices;
    D3DCOLOR SpriteColor = D3DCOLOR_COLORVALUE(m_SpriteColor.r, m_SpriteColor.g,
            m_SpriteColor.b, m_SpriteColor.a);
	m_VertexBuffer9->Lock( 0, m_NumParticles * sizeof(fp_SpriteVertex),
            (void**)&pSpriteVertices, D3DLOCK_DISCARD );
	for( int i = 0; i < m_NumParticles; i++ ) {
        pSpriteVertices->m_Position = m_Particles[i].m_Position;
        pSpriteVertices->m_Color = SpriteColor;
        pSpriteVertices++;
    }
    m_VertexBuffer9->Unlock();
	
	//
	// Render point sprites...
	//

    d3dDevice->SetStreamSource( 0, m_VertexBuffer9, 0, sizeof(fp_SpriteVertex) );
    d3dDevice->SetFVF( fp_SpriteVertex::FVF_Flags );
    d3dDevice->SetTexture(0, m_Texture9);
	d3dDevice->DrawPrimitive( D3DPT_POINTLIST, 0, m_NumParticles );

	//
    // Reset render states...
	//

    d3dDevice->SetTexture(0, NULL);
	
    d3dDevice->SetRenderState( D3DRS_LIGHTING,  TRUE );

    d3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
    d3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  FALSE );

    d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
    d3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
    d3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

void fp_RenderSprites::OnD3D9DestroyDevice( void* UserContext ) {
    SAFE_RELEASE(m_Texture9);
}

void fp_RenderSprites::OnD3D9LostDevice(void* UserContext) {   
    SAFE_RELEASE(m_VertexBuffer9);
}

HRESULT fp_RenderSprites::OnD3D10CreateDevice(
        ID3D10Device* d3dDevice,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Media/PointSprites/particle.bmp" ) );
    D3DX10CreateShaderResourceViewFromFile(d3dDevice, str, NULL, NULL,
            &m_Texture10RV, NULL);



    // Read the D3DX effect file
    m_Effect10 = DX10Tools::EffectLoader(d3dDevice, L"fp_render_sprites.fx");
    //V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"fp_render_sprites.fx" ) );
    //V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", dwShaderFlags, 
    //        0, d3dDevice, NULL, NULL, &m_Effect10, NULL, NULL ) );

    // Obtain technique objects
    m_TechRenderSprites = m_Effect10->GetTechniqueByName(
            "RenderSprites" );

    // Obtain effect variables and set as needed
    m_EffectSpriteSize = m_Effect10->GetVariableByName("m_EffectSpriteSize")->AsScalar();
    m_EffectSpriteColor = m_Effect10->GetVariableByName("m_EffectSpriteColor")->AsVector();
    V_RETURN(m_EffectSpriteColor->SetFloatVector((float*)&m_SpriteColor));
    m_EffectTexture = m_Effect10->GetVariableByName( "m_EffectTexture" )->AsShaderResource();
    V_RETURN(m_EffectTexture->SetResource(m_Texture10RV));
    m_EffectWorldViewProjection = m_Effect10->GetVariableByName( "m_WorldViewProjection" )
        ->AsMatrix();
    m_EffectWorld = m_Effect10->GetVariableByName( "m_World" )->AsMatrix();

    D3D10_PASS_DESC passDesc;
    V_RETURN( m_TechRenderSprites->GetPassByIndex(0)->GetDesc(&passDesc));
    V_RETURN( d3dDevice->CreateInputLayout(fp_SpriteVertex::Layout, 1, passDesc.pIAInputSignature, 
            passDesc.IAInputSignatureSize, &m_VertexLayout ) );

    return S_OK;
}

HRESULT fp_RenderSprites::OnD3D10ResizedSwapChain(
        ID3D10Device* d3dDevice,
        IDXGISwapChain *SwapChain,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    D3D10_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_NumParticles * sizeof(fp_SpriteVertex);
    bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    V_RETURN(d3dDevice->CreateBuffer(&bufferDesc, NULL, &m_VertexBuffer10));

    // Set vertex buffer
    UINT stride = sizeof(fp_SpriteVertex);
    UINT offset = 0;
    d3dDevice->IASetVertexBuffers(0, 1, &m_VertexBuffer10, &stride, &offset);
    return S_OK;
}

void fp_RenderSprites::OnD3D10FrameRender(
        ID3D10Device* d3dDevice,
        double Time,
        float ElapsedTime,
        const D3DXVECTOR3* EyePt,
        const D3DXMATRIX*  WorldViewProjection,
        const D3DXMATRIX*  World,
        const D3DXMATRIX*  View,
        const D3DXMATRIX*  Proj,
        int NumActiveLights,
        int ActiveLight,
        float LightScale) {
     //d3dDevice->SetRenderState( D3DRS_LIGHTING,  FALSE );
     //d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
     //d3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
     //d3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
     //d3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
     //d3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
     //d3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  TRUE );
     //d3dDevice->SetRenderState( D3DRS_POINTSIZE,     fp_Util::FtoDW(m_SpriteSize) );
     //d3dDevice->SetRenderState( D3DRS_POINTSIZE_MIN, fp_Util::FtoDW(1.0f) );
     //d3dDevice->SetRenderState( D3DRS_POINTSCALE_A,  fp_Util::FtoDW(0.0f) );
     //d3dDevice->SetRenderState( D3DRS_POINTSCALE_B,  fp_Util::FtoDW(0.0f) );
     //d3dDevice->SetRenderState( D3DRS_POINTSCALE_C,  fp_Util::FtoDW(1.0f) );

     //fp_SpriteVertex *pSpriteVertices;

     //m_VertexBuffer->Lock( 0, m_NumParticles * sizeof(fp_SpriteVertex),
     //    (void**)&pSpriteVertices, D3DLOCK_DISCARD );

     //for( int i = 0; i < m_NumParticles; i++ )
     //{
     //    pSpriteVertices->m_Position = m_Particles[i].m_Position;
     //    pSpriteVertices->m_cColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);
     //    pSpriteVertices++;
     //}

     //m_VertexBuffer->Unlock();

     ////
     //// Render point sprites...
     ////

     //d3dDevice->SetStreamSource( 0, m_VertexBuffer, 0, sizeof(fp_SpriteVertex) );
     //d3dDevice->SetFVF( fp_SpriteVertex::FVF_Flags );
     //d3dDevice->SetTexture(0, m_Texture);
     //d3dDevice->DrawPrimitive( D3DPT_POINTLIST, 0, m_NumParticles );

     ////
     //// Reset render states...
     ////

     //d3dDevice->SetTexture(0, NULL);

     //d3dDevice->SetRenderState( D3DRS_LIGHTING,  TRUE );

     //d3dDevice->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
     //d3dDevice->SetRenderState( D3DRS_POINTSCALEENABLE,  FALSE );

     //d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
     //d3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
     //d3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

void fp_RenderSprites::OnD3D10DestroyDevice( void* UserContext ) {
    SAFE_RELEASE(m_Texture10RV);
    SAFE_RELEASE(m_Effect10);
    SAFE_RELEASE(m_VertexLayout);
}

void fp_RenderSprites::OnD3D10ReleasingSwapChain( void* UserContext ) {   
    SAFE_RELEASE(m_VertexBuffer10);
}