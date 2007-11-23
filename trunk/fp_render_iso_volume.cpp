#include "DXUT.h"
//#include "SDKmisc.h"
#include "fp_render_iso_volume.h"
//#include "fp_util.h"

fp_IsoVolume::fp_IsoVolume(fp_FluidParticle* Particles, int NumParticles, float VoxelSize) 
        :
        m_NumValues(0),
        m_NumValuesX(0),
        m_NumValuesY(0),
        m_NumValuesZ(0),
        m_VoxelSize(VoxelSize),
        m_IsoValues(),
        m_Particles(Particles),
        m_NumParticles(NumParticles) {
    m_IsoValues.reserve(FP_INITIAL_ISOVOLUME_CAPACITY);
}

void fp_IsoVolume::ConstructFromParticles(
        float MinX, 
        float MaxX, 
        float MinY, 
        float MaxY, 
        float MinZ, 
        float MaxZ) {
    m_NumValuesX = (int)((MaxX - MinX + m_VoxelSize) / m_VoxelSize);
    m_NumValuesY = (int)((MaxY - MinY + m_VoxelSize) / m_VoxelSize);
    m_NumValuesZ = (int)((MaxZ - MinZ + m_VoxelSize) / m_VoxelSize);
    m_NumValues = m_NumValuesX * m_NumValuesY * m_NumValuesZ;
    for (int i = 0; i < m_NumParticles; i++) {
        
    }
}

fp_RenderIsoVolume::fp_RenderIsoVolume(fp_IsoVolume* IsoVolume) {
}

fp_RenderIsoVolume::~fp_RenderIsoVolume() {
    OnLostDevice();
    OnDetroyDevice();
}

HRESULT fp_RenderIsoVolume::OnCreateDevice(                                 
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* pBackBufferSurfaceDesc ) {
    HRESULT hr;

    return S_OK;
}

HRESULT fp_RenderIsoVolume::OnResetDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc ) {

    return S_OK;
}

void fp_RenderIsoVolume::OnFrameRender(
        IDirect3DDevice9* d3dDevice,
        double Time,
        float ElapsedTime ) {

}

void fp_RenderIsoVolume::OnDetroyDevice() {

}

void fp_RenderIsoVolume::OnLostDevice() {

}
