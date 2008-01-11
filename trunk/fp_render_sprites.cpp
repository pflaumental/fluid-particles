#include "DXUT.h"
#include "SDKmisc.h"
#include "fp_render_sprites.h"
#include "fp_util.h"

#define FP_RENDER_SPRITES_TEXTURE_FILE L"Media/PointSprites/particle.dds" 
#define FP_RENDER_SPRITES_EFFECT_FILE L"fp_render_sprites.fx" 

const D3D10_INPUT_ELEMENT_DESC fp_SpriteVertex::Layout[] = { { "POSITION",  0,
        DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 } };

fp_RenderSprites::fp_RenderSprites(int NumParticles, fp_FluidParticle* Particles)
        :
        m_NumParticles(NumParticles),
        m_Particles(Particles),
        m_SpriteSize(FP_RENDER_DEFAULT_SPRITE_SIZE),
		m_Texture9(NULL),
        m_VertexBuffer10(NULL),
		m_Texture10RV(NULL),
        m_Effect10(NULL),
        m_TechRenderSprites(NULL),
        m_EffectTexture(NULL),
        m_EffectViewProjection(NULL),
        m_EffectSpriteCornersWorldS(NULL),
        m_VertexLayout(NULL){
}

fp_RenderSprites::~fp_RenderSprites() {
	if(DXUTIsAppRenderingWithD3D9()){
		OnD3D9LostDevice(NULL);
		OnD3D9DestroyDevice(NULL);
	}
	if(DXUTIsAppRenderingWithD3D10()) {
		OnD3D10ReleasingSwapChain(NULL);
		OnD3D10DestroyDevice(NULL);
	}
}

float fp_RenderSprites::GetSpriteSize() const {
    return m_SpriteSize;
}

void fp_RenderSprites::SetSpriteSize(float SpriteSize) {
    m_SpriteSize = SpriteSize;
}

HRESULT fp_RenderSprites::OnD3D9CreateDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, FP_RENDER_SPRITES_TEXTURE_FILE ) );
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

void fp_RenderSprites::OnD3D9FrameRender(IDirect3DDevice9* d3dDevice) {
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
	for( int i = 0; i < m_NumParticles; i++ ) {
        pSpriteVertices->m_Position = m_Particles[i].m_Position;
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
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, FP_RENDER_SPRITES_TEXTURE_FILE ) );
    D3DX10CreateShaderResourceViewFromFile(d3dDevice, str, NULL, NULL,
            &m_Texture10RV, NULL);

    // Read the D3DX effect file
	m_Effect10 = fp_Util::LoadEffect(d3dDevice, FP_RENDER_SPRITES_EFFECT_FILE);

    // Obtain technique objects
    m_TechRenderSprites = m_Effect10->GetTechniqueByName(
            "RenderSprites" );

    // Obtain effect variables and set as needed
    m_EffectTexture = m_Effect10->GetVariableByName("g_ParticleDiffuse")->AsShaderResource();
    V_RETURN(m_EffectTexture->SetResource(m_Texture10RV));
    m_EffectViewProjection = m_Effect10->GetVariableByName( "g_ViewProj" )->AsMatrix();
    m_EffectSpriteCornersWorldS = m_Effect10->GetVariableByName( "g_SpriteCornersWorldS" )->AsVector();

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
    return S_OK;
}

void fp_RenderSprites::OnD3D10FrameRender(
        ID3D10Device* d3dDevice,
        const D3DXMATRIX*  ViewProjection,
		const D3DXMATRIX*  InvView) {
    HRESULT hr;
     fp_SpriteVertex *pSpriteVertices;
     m_VertexBuffer10->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&pSpriteVertices);
     for( int i = 0; i < m_NumParticles; i++ ) {
         pSpriteVertices->m_Position = m_Particles[i].m_Position;
         pSpriteVertices++;
     }
     m_VertexBuffer10->Unmap();

	 d3dDevice->IASetInputLayout(m_VertexLayout);
	 UINT stride = sizeof(fp_SpriteVertex);
	 UINT offset = 0;
	 d3dDevice->IASetVertexBuffers(0, 1, &m_VertexBuffer10, &stride, &offset);
	 d3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );

	 V(m_EffectTexture->SetResource(m_Texture10RV));
     V(m_EffectViewProjection->SetMatrix((float*) ViewProjection));
     D3DXVECTOR4 spriteCornersWorldS[4] = {
         D3DXVECTOR4(-1,1,0,0),
         D3DXVECTOR4(1,1,0,0),
         D3DXVECTOR4(-1,-1,0,0),
         D3DXVECTOR4(1,-1,0,0)
     };
     for(int i=0; i<4; i++) {
         spriteCornersWorldS[i] *= m_SpriteSize;
     }
     D3DXVec3TransformNormalArray((D3DXVECTOR3*)spriteCornersWorldS, sizeof(D3DXVECTOR4),
            (D3DXVECTOR3*)spriteCornersWorldS, sizeof(D3DXVECTOR4), InvView, 4);
     V(m_EffectSpriteCornersWorldS->SetRawValue(spriteCornersWorldS, 0,
            sizeof(D3DXVECTOR4)*4));

	 D3D10_TECHNIQUE_DESC techDesc;
	 m_TechRenderSprites->GetDesc( &techDesc );
	 for( UINT iPass = 0; iPass < techDesc.Passes; ++iPass ) {
		 m_TechRenderSprites->GetPassByIndex( iPass )->Apply(0);
		 d3dDevice->Draw(m_NumParticles, 0);
	 }
}

void fp_RenderSprites::OnD3D10DestroyDevice( void* UserContext ) {
    SAFE_RELEASE(m_Texture10RV);
    SAFE_RELEASE(m_Effect10);
    SAFE_RELEASE(m_VertexLayout);
}

void fp_RenderSprites::OnD3D10ReleasingSwapChain( void* UserContext ) {   
    SAFE_RELEASE(m_VertexBuffer10);
}