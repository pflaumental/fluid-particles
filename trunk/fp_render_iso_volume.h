#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

#define FP_DEFAULT_ISOVOLUME_VOXELSIZE 0.2f
#define FP_INITIAL_ISOVOLUME_SIDELENGTH 200
#define FP_INITIAL_ISOVOLUME_CAPACITY FP_INITIAL_ISOVOLUME_SIDELENGTH \
        * FP_INITIAL_ISOVOLUME_SIDELENGTH * FP_INITIAL_ISOVOLUME_SIDELENGTH


//--------------------------------------------------------------------------------------
// Fluid particles render technique: Iso volume via marching cubes
//--------------------------------------------------------------------------------------

class fp_IsoVolume {
public:
    std::vector<float> m_IsoValues;
    int m_NumValuesX;
    int m_NumValuesY;
    int m_NumValuesZ;
    int m_NumValues;
    float m_VoxelSize;
    float m_SmoothingLength;
    float m_PaticleMass;
    
    fp_IsoVolume(
            fp_FluidParticle* Particles,
            int NumParticles,
            float VoxelSize = FP_DEFAULT_ISOVOLUME_VOXELSIZE);
    void ConstructFromParticles(
            float MinX, 
            float MaxX, 
            float MinY, 
            float MaxY, 
            float MinZ, 
            float MaxZ);

private:
    fp_FluidParticle* m_Particles;
    int m_NumParticles;
};

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