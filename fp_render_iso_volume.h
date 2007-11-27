#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

#define FP_DEFAULT_ISOVOLUME_VOXELSIZE 1.0f
#define FP_INITIAL_ISOVOLUME_SIDELENGTH 200
#define FP_INITIAL_ISOVOLUME_CAPACITY FP_INITIAL_ISOVOLUME_SIDELENGTH \
        * FP_INITIAL_ISOVOLUME_SIDELENGTH * FP_INITIAL_ISOVOLUME_SIDELENGTH


//--------------------------------------------------------------------------------------
// Fluid particles render technique: Iso volume via marching cubes
//--------------------------------------------------------------------------------------

typedef struct {
    int X;
    int Y;
    int Z;    
} fp_VolumeIndex;

fp_VolumeIndex operator+(const fp_VolumeIndex& A, const fp_VolumeIndex& B);

class fp_IsoVolume {
public:
    fp_Fluid* m_Fluid;
    std::vector<float> m_IsoValues;
    int m_NumValuesX;
    int m_NumValuesY;
    int m_NumValuesZ;
    int m_NumValuesYZ;
    int m_NumValues;
    float m_VoxelSize;
    float m_HalfVoxelSize;
    
    fp_IsoVolume(fp_Fluid* Fluid, float VoxelSize = FP_DEFAULT_ISOVOLUME_VOXELSIZE);
    void UpdateSmoothingLength();
    void SetVoxelSize(float VoxelSize);
    void ConstructFromFluid();
    inline void DistributeParticle(
            D3DXVECTOR3 ParticlePosition, 
            float ParticleDensity,
            float ParticleMass,
            float MinX,
            float MinY,
            float MinZ);

private:
    int m_NumStampRows;
    int* m_StampRowLengths;
    fp_VolumeIndex* m_StampRowStartOffsets;    
    int* m_StampRowValueStarts;

    int m_NumStampValues;
    float* m_StampValues;    

    void CreateStamp();
    void DestroyStamp();
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
