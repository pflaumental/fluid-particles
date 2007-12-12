#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_cpu_sph.h"

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

struct fp_VolumeIndex {
    int x;
    int y;
    int z;

    fp_VolumeIndex() : x(0), y(0), z(0) {}
    fp_VolumeIndex(int X, int Y, int Z) : x(X), y(Y), z(Z) {}
    fp_VolumeIndex(const fp_VolumeIndex& Other) : x(Other.x), y(Other.y), z(Other.z) {}
};

fp_VolumeIndex operator+(const fp_VolumeIndex& A, const fp_VolumeIndex& B);

class fp_IsoVolume {
public:
    fp_Fluid* m_Fluid;
    std::vector<float> m_IsoValues;
    std::vector<D3DXVECTOR3> m_GradientIsoValues;
    float m_LastMinX;
    float m_LastMinY;
    float m_LastMinZ;
    float m_LastMaxX;
    float m_LastMaxY;
    float m_LastMaxZ;
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
    inline void DistributeParticle(
        D3DXVECTOR3 ParticlePosition, 
        float ParticleMassDensityQuotient,
        float MinX,
        float MinY,
        float MinZ);
    inline void DistributeParticleWithStamp(
            D3DXVECTOR3 ParticlePosition, 
            float ParticleMassDensityQuotient,
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

    void ConstructMesh();

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
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;
    LPDIRECT3DINDEXBUFFER9 m_IndexBuffer;
    D3DMATERIAL9 m_Material;
    //LPDIRECT3DTEXTURE9 m_Texture;
    static int s_EdgeTable[256];
    static int s_TriTable[256][16];

    inline D3DXVECTOR3 CalcNormal(
        const D3DXVECTOR3* gradient1, 
        const D3DXVECTOR3* gradient2, 
        float s);
};

#endif
