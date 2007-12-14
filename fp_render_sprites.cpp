#include "DXUT.h"
#include "SDKmisc.h"
#include "fp_render_sprites.h"
#include "fp_util.h"

D3DCOLOR g_SpriteColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);

fp_RenderSprites::fp_RenderSprites(int NumParticles, fp_FluidParticle* Particles)
        :
        m_NumParticles(NumParticles),
        m_Particles(Particles),
        m_SpriteSize(FP_RENDER_DEFAULT_SPRITE_SIZE),
		m_Texture9(NULL),
		m_Texture10(NULL){
}

fp_RenderSprites::~fp_RenderSprites() {
	OnD3D9LostDevice(NULL);
	OnD3D9DestroyDevice(NULL);
    OnD3D10ResizedSwapChain(NULL, NULL, NULL, NULL);
    OnD3D10DestroyDevice(NULL);
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

	m_VertexBuffer9->Lock( 0, m_NumParticles * sizeof(fp_SpriteVertex),
		                   (void**)&pSpriteVertices, D3DLOCK_DISCARD );

	for( int i = 0; i < m_NumParticles; i++ )
    {
        pSpriteVertices->m_Position = m_Particles[i].m_Position;
        pSpriteVertices->m_cColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);
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
    D3DX10CreateTextureFromFile(d3dDevice, str, NULL, NULL,
            (ID3D10Resource**)&m_Texture10, NULL);

    return S_OK;
}

HRESULT fp_RenderSprites::OnD3D10ResizedSwapChain(
        ID3D10Device* d3dDevice,
        IDXGISwapChain *SwapChain,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    //d3dDevice->CreateBuffer()
    //d3dDevice->CreateVertexBuffer( m_NumParticles * sizeof(fp_SpriteVertex), 
    //        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY | D3DUSAGE_POINTS, 
    //        fp_SpriteVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer, NULL );
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
    SAFE_RELEASE(m_Texture10);
}

void fp_RenderSprites::OnD3D10ReleasingSwapChain( void* UserContext ) {   
    //SAFE_RELEASE(m_VertexBuffer);
}