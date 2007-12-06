#include "DXUT.h"
#include "SDKmisc.h"
#include "fp_render_sprites.h"
#include "fp_util.h"

D3DCOLOR g_SpriteColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);

fp_RenderSprites::fp_RenderSprites(int NumParticles, fp_FluidParticle* Particles)
        :
        m_NumParticles(NumParticles),
        m_Particles(Particles),
        m_SpriteSize(FP_RENDER_DEFAULT_SPRITE_SIZE){
}

fp_RenderSprites::~fp_RenderSprites() {
	OnLostDevice();
	OnDetroyDevice();
}

HRESULT fp_RenderSprites::OnCreateDevice(                                 
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* pBackBufferSurfaceDesc ) {
    HRESULT hr;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Media/PointSprites/particle.bmp" ) );
    D3DXCreateTextureFromFile( d3dDevice, str, &m_Texture );
    
    return S_OK;
}

HRESULT fp_RenderSprites::OnResetDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc ) {
    d3dDevice->CreateVertexBuffer( m_NumParticles * sizeof(fp_SpriteVertex), 
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY | D3DUSAGE_POINTS, 
            fp_SpriteVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer, NULL );
    return S_OK;
}

void fp_RenderSprites::OnFrameRender(
        IDirect3DDevice9* d3dDevice,
        double Time,
        float ElapsedTime ) {
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

	m_VertexBuffer->Lock( 0, m_NumParticles * sizeof(fp_SpriteVertex),
		                   (void**)&pSpriteVertices, D3DLOCK_DISCARD );

	for( int i = 0; i < m_NumParticles; i++ )
    {
        pSpriteVertices->m_Position = m_Particles[i].m_Position;
        pSpriteVertices->m_cColor = D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f);
        pSpriteVertices++;
    }

    m_VertexBuffer->Unlock();
	
	//
	// Render point sprites...
	//

    d3dDevice->SetStreamSource( 0, m_VertexBuffer, 0, sizeof(fp_SpriteVertex) );
    d3dDevice->SetFVF( fp_SpriteVertex::FVF_Flags );
    d3dDevice->SetTexture(0, m_Texture);
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

void fp_RenderSprites::OnDetroyDevice() {
    SAFE_RELEASE(m_Texture);
}

void fp_RenderSprites::OnLostDevice() {   
    SAFE_RELEASE(m_VertexBuffer);
}
