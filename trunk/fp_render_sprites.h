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

    //static const D3D10_INPUT_ELEMENT_DESC layout[] = {
    //    { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //    { "DIFFUSE",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, 
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //    { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, 
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //};
};

class fp_RenderSprites {
public:
    float m_SpriteSize;
    fp_FluidParticle* m_Particles;

    fp_RenderSprites(int NumParticles, fp_FluidParticle* Particles);
    ~fp_RenderSprites();

    // DX9 specific
    HRESULT OnD3D9CreateDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    HRESULT OnD3D9ResetDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    void    OnD3D9FrameRender(
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
            float LightScale);
    void    OnD3D9LostDevice( void* UserContext );
    void    OnD3D9DestroyDevice( void* UserContext );

    // DX10 specific
    HRESULT OnD3D10CreateDevice(
            ID3D10Device* d3dDevice,
            const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    HRESULT OnD3D10ResizedSwapChain(
            ID3D10Device* d3dDevice,
            IDXGISwapChain *SwapChain,
            const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    void    OnD3D10ReleasingSwapChain( void* UserContext );
    void    OnD3D10DestroyDevice( void* UserContext );
    void    OnD3D10FrameRender(
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
            float LightScale); 

private:
    int m_NumParticles;    
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer9;
    LPDIRECT3DTEXTURE9 m_Texture9;
    ID3D10Texture2D * m_Texture10;
};

#endif
