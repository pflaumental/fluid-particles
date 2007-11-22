#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

//--------------------------------------------------------------------------------------
// Fluid particles render technique: Iso volume via marching cubes
//--------------------------------------------------------------------------------------

typedef int fp_IsoVolume;

class fp_RenderIsoVolume {
public:
    fp_IsoVolume* m_IsoVolume;

    fp_RenderIsoVolume(fp_IsoVolume* IsoVolume);
    ~fp_RenderIsoVolume();

    HRESULT OnCreateDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc );

    HRESULT OnResetDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc );

    void OnFrameRender(
            IDirect3DDevice9* d3dDevice,
            double Time,
            float ElapsedTime );

    void OnDetroyDevice();

    void OnLostDevice();

private:
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;
    //LPDIRECT3DTEXTURE9 m_Texture;
};

#endif
