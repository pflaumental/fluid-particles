#pragma once
#ifndef FP_RENDER_RAYCAST_H
#define FP_RENDER_RAYCAST_H

#include "DXUT.h"

#include "fp_global.h"
#include "fp_util.h"
#include "fp_cpu_sph.h"

//--------------------------------------------------------------------------------------
// Fluid particles render technique: Iso volume via GPU raycast
//--------------------------------------------------------------------------------------

struct fp_SplatParticleVertex {
    D3DXVECTOR4 m_PositionAndDensity;
    static const D3D10_INPUT_ELEMENT_DESC Layout[];
};

class fp_RenderRaycast {
public:  
    float m_IsoLevel;
    fp_Fluid* m_Fluid;
    fp_FluidParticle* m_Particles;

    fp_RenderRaycast(
            fp_Fluid* Fluid,
            float VoxelSize,
            float IsoLevel = FP_RAYCAST_DEFAULT_ISO_LEVEL,
            const fp_VolumeIndex& VolumeDimensions
                    = FP_RAYCAST_VOLUME_TEXTURE_DIMENSIONS);
    ~fp_RenderRaycast();

    void SetFluid(fp_Fluid* Fluid);
    void SetVoxelSize(float VoxelSize);
    D3DXVECTOR3 GetVolumeSize();
    fp_VolumeIndex GetVolumeTextureSize();
    
    // Defines the lower, left, front corner of the volume
    void SetVolumeStartPos(D3DXVECTOR3* VolumeStartPos);
    
    HRESULT CreateVolumeTexture(ID3D10Device* pd3dDevice);
    void FillVolumeTexture(ID3D10Device* d3dDevice); 
    void DestroyVolumeTexture();

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
            ID3D10Device* d3dDevice); 

private:
    ID3D10Texture3D *m_VolumeTexture;
    ID3D10RenderTargetView *m_VolumeRTV;
    ID3D10ShaderResourceView *m_VolumeSRV;
    ID3D10Texture1D *m_WValsMulParticleMassTexture;
    ID3D10ShaderResourceView *m_WValsMulParticleMassSRV;
    int m_WValsMulParticleMassLength;

    ID3D10InputLayout* m_VertexLayout;
    ID3D10Buffer* m_VertexBuffer;    

    ID3D10Effect* m_Effect;
    ID3D10EffectTechnique* m_TechRenderRaycast;
    ID3D10EffectVectorVariable* m_EffectVarCornersPos;
    ID3D10EffectScalarVariable* m_EffectVarHalfParticleVoxelDiameter;
    ID3D10EffectScalarVariable* m_EffectVarParticleVoxelRadius;
    ID3D10EffectVectorVariable* m_EffectVarVolumeDimensions;
    ID3D10EffectMatrixVariable* m_EffectVarWorldToNDS;
    ID3D10EffectShaderResourceVariable* m_EffectVarWValsMulParticleMass;

    fp_VolumeIndex m_VolumeDimensions;
    
    int m_NumParticles;
    float m_VoxelSize;
    int m_ParticleVoxelDiameter;
    int m_ParticleVoxelRadius;
    D3DXVECTOR3 m_VolumeStartPos;
};

#endif
