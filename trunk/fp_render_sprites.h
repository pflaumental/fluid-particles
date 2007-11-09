#pragma once
#ifndef FP_RENDER_SPRITES_H
#define FP_RENDER_SPRITES_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

//--------------------------------------------------------------------------------------
// Fluid particles render technique: Point Sprites
//--------------------------------------------------------------------------------------

struct fp_SpriteVertex
{
    D3DXVECTOR3 m_Position;
    D3DCOLOR    m_cColor;

	enum FVF
	{
		FVF_Flags = D3DFVF_XYZ|D3DFVF_DIFFUSE
	};
};

class fp_RenderSprites {
public:
    fp_RenderSprites(int NumParticles, fp_FluidParticle* pParticles);
    ~fp_RenderSprites();

    HRESULT OnCreateDevice(
            IDirect3DDevice9* pd3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc );

    HRESULT OnResetDevice(
            IDirect3DDevice9* pd3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc );

    void OnFrameRender(
            IDirect3DDevice9* pd3dDevice,
            double fTime,
            float fElapsedTime );

    void OnDetroyDevice();

    void OnLostDevice();

private:
    int m_NumParticles;
    fp_FluidParticle* m_Particles;
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;
    LPDIRECT3DTEXTURE9 m_Texture;    
};

#endif
