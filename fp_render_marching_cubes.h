#pragma once
#ifndef FP_RENDER_ISO_VOLUME_H
#define FP_RENDER_ISO_VOLUME_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_util.h"
#include "fp_cpu_sph.h"

//--------------------------------------------------------------------------------------
// Fluid particles render technique: (CPU generated) iso volume via (CPU generated)
// marching cubes
//--------------------------------------------------------------------------------------

struct fp_MCVertex
{
    D3DXVECTOR3 m_Position;
    D3DXVECTOR3 m_Normal;

	enum FVF
	{
		FVF_Flags = D3DFVF_XYZ|D3DFVF_NORMAL
	};

    static const D3D10_INPUT_ELEMENT_DESC Layout[];
    
    fp_MCVertex(D3DXVECTOR3 Position, D3DXVECTOR3 Normal) :
            m_Position(Position),
            m_Normal(Normal) {}
};

class fp_CPUIsoVolume {
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
    
    fp_CPUIsoVolume(
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

class fp_RenderMarchingCubes {
public:
    fp_CPUIsoVolume* m_IsoVolume;    
    float m_IsoLevel;
    int m_NumVertices;
    int m_NumTriangles;
    int m_NumLights;
    int m_NumActiveLights;
    D3DLIGHT9* m_Lights9;

    fp_RenderMarchingCubes(
            fp_CPUIsoVolume* IsoVolume,
            int NumLights,
            float IsoLevel = FP_DEFAULT_MC_ISO_LEVEL);
    ~fp_RenderMarchingCubes();

    void ConstructMesh();

    // D3D9 specific
    HRESULT OnD3D9CreateDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    HRESULT OnD3D9ResetDevice(
            IDirect3DDevice9* d3dDevice,
            const D3DSURFACE_DESC* BackBufferSurfaceDesc,
            void* UserContext );
    void    OnD3D9FrameRender(IDirect3DDevice9* d3dDevice);
    void    OnD3D9LostDevice( void* UserContext );
    void    OnD3D9DestroyDevice( void* UserContext );

    // D3D10 specific
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
            const D3DXMATRIX*  WorldViewProjection); 

private:
    // D3D9 resources
    LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer9;
    LPDIRECT3DINDEXBUFFER9 m_IndexBuffer9;
    D3DMATERIAL9 m_Material9;
    //LPDIRECT3DTEXTURE9 m_Texture;

    // D3D10 resources
    ID3D10Buffer* m_VertexBuffer10;
    ID3D10Buffer* m_IndexBuffer10;
    ID3D10Effect* m_Effect10;
    ID3D10EffectTechnique* m_TechRenderIsoVolume1Light;
    ID3D10EffectTechnique* m_TechRenderIsoVolume2Lights;
    ID3D10EffectTechnique* m_TechRenderIsoVolume3Lights;
    ID3D10EffectVectorVariable* m_EffectVarLightDir;
    ID3D10EffectVectorVariable* m_EffectVarLightDiffuse;
    ID3D10EffectVectorVariable* m_EffectVarLightAmbient;
    ID3D10EffectMatrixVariable* m_EffectVarWorldViewProjection;
    ID3D10EffectVectorVariable* m_EffectVarMaterialDiffuseColor;
    ID3D10EffectVectorVariable* m_EffectVarMaterialAmbientColor;
    ID3D10InputLayout* m_VertexLayout;

    static int s_EdgeTable[256];
    static int s_TriTable[256][16];

    inline D3DXVECTOR3 CalcNormal(
        const D3DXVECTOR3* gradient1, 
        const D3DXVECTOR3* gradient2, 
        float s);
};

#endif
