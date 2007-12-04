#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

#define FP_DEFAULT_ISOVOLUME_VOXELSIZE 1.0f
#define FP_DEFAULT_ISO_VOLUME_BORDER 5.0f
#define FP_DEFAULT_MC_ISO_LEVEL 0.001f
#define FP_INITIAL_ISOVOLUME_SIDELENGTH 200
#define FP_MC_MAX_TRIANGLES 1000000
#define FP_MC_MAX_VETICES FP_MC_MAX_TRIANGLES * 3
#define FP_INITIAL_ISOVOLUME_CAPACITY FP_INITIAL_ISOVOLUME_SIDELENGTH \
        * FP_INITIAL_ISOVOLUME_SIDELENGTH * FP_INITIAL_ISOVOLUME_SIDELENGTH


//--------------------------------------------------------------------------------------
// Fluid particles render technique: Iso volume via marching cubes
//--------------------------------------------------------------------------------------

struct fp_MCVertex
{
    D3DXVECTOR3 m_Position;
    D3DXVECTOR3 m_Normal;

	enum FVF
	{
		FVF_Flags = D3DFVF_XYZ|D3DFVF_NORMAL
	};
    
    fp_MCVertex(D3DXVECTOR3 Position, D3DXVECTOR3 Normal) :
            m_Position(Position),
            m_Normal(Normal) {}
};

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
    std::vector<D3DXVECTOR3> m_GradientIsoValues;
    int m_NumValuesX;
    int m_NumValuesY;
    int m_NumValuesZ;
    int m_NumValuesYZ;
    int m_NumValues;
    float m_VoxelSize;
    float m_HalfVoxelSize;
    float m_IsoVolumeBorder;
    D3DXVECTOR3 m_VolumeStart;
    D3DXVECTOR3 m_VolumeCellOffset;
    
    fp_IsoVolume(
            fp_Fluid* Fluid, 
            float VoxelSize = FP_DEFAULT_ISOVOLUME_VOXELSIZE, 
            float IsoVolumeBorder = FP_DEFAULT_ISO_VOLUME_BORDER);
    void UpdateSmoothingLength();
    void SetVoxelSize(float VoxelSize);
    void ConstructFromFluid();
    inline void DistributeParticleWithStamp(
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
    D3DXVECTOR3* m_GradientStampValues;

    void CreateStamp();
    void DestroyStamp();
};

class fp_RenderIsoVolume {
public:
    fp_IsoVolume* m_IsoVolume;    
    float m_IsoLevel;
    int m_NumVertices;
    int m_NumTriangles;
    int m_NumActiveLights;
    D3DLIGHT9* m_Lights;

    fp_RenderIsoVolume(
            fp_IsoVolume* IsoVolume,
            int NumLights,
            float IsoLevel = FP_DEFAULT_MC_ISO_LEVEL);
    ~fp_RenderIsoVolume();

    HRESULT OnCreateDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc );

    HRESULT OnResetDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc );

    void ConstructMesh();

    void OnFrameRender(
            IDirect3DDevice9* d3dDevice,
            double Time,
            float ElapsedTime );
    

    void OnDetroyDevice();

    void OnLostDevice();

private:
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;
    LPDIRECT3DINDEXBUFFER9 m_IndexBuffer;
    D3DMATERIAL9 m_Material;
    //LPDIRECT3DTEXTURE9 m_Texture;
    static int s_EdgeTable[256];
    static int s_TriTable[256][16];

    inline D3DXVECTOR3 CalcNormal(
        D3DXVECTOR3* gradient1, 
        D3DXVECTOR3* gradient2, 
        float s);
};

#endif
