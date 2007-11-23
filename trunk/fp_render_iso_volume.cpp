#include "DXUT.h"
//#include "SDKmisc.h"
#include "fp_render_iso_volume.h"
//#include "fp_util.h"

fp_VolumeIndex operator+(
        const fp_VolumeIndex& A,
        const fp_VolumeIndex& B) {
    fp_VolumeIndex result;
    result.X = A.X + B.X;
    result.Y = A.Y + B.Y;
    result.Z = A.Z + B.Z;
    return result;
}

fp_IsoVolume::fp_IsoVolume(
        fp_FluidParticle* Particles, 
        int NumParticles, 
        float VoxelSize,
        float SmoothingLength,
        float ParticleMass) 
        :
        m_NumValues(0),
        m_NumValuesX(0),
        m_NumValuesY(0),
        m_NumValuesZ(0),
        m_IsoValues(),
        m_Particles(Particles),
        m_NumParticles(NumParticles) {
    m_VoxelSize = -1.0f; // Needed in SetSmoothingLength()
    SetSmoothingLength(SmoothingLength);
    SetParticleMass(ParticleMass);
    SetVoxelSize(VoxelSize);
    m_IsoValues.reserve(FP_INITIAL_ISOVOLUME_CAPACITY);
}

void fp_IsoVolume::SetSmoothingLength(float SmoothingLength) {
    m_SmoothingLength = SmoothingLength;
    if(m_VoxelSize > 0.0f)
        CreateStamp();
}

void fp_IsoVolume::SetParticleMass(float ParticleMass) {
    m_ParticleMass = ParticleMass;
}

void fp_IsoVolume::SetVoxelSize(float VoxelSize) {
    m_VoxelSize = VoxelSize;
    m_HalfVoxelSize = VoxelSize / 2.0f;
    CreateStamp();
}


void fp_IsoVolume::ConstructFromParticles(
        float MinX, 
        float MaxX, 
        float MinY, 
        float MaxY, 
        float MinZ, 
        float MaxZ,
        float* Densities) {
    m_NumValuesX = (int)((MaxX - MinX + m_VoxelSize) / m_VoxelSize);
    m_NumValuesY = (int)((MaxY - MinY + m_VoxelSize) / m_VoxelSize);
    m_NumValuesZ = (int)((MaxZ - MinZ + m_VoxelSize) / m_VoxelSize);
    m_NumValues = m_NumValuesX * m_NumValuesY * m_NumValuesZ;
    m_IsoValues.clear();
    m_IsoValues.resize(m_NumValues, 0.0f);
    for (int i = 0; i < m_NumParticles; i++) {
        D3DXVECTOR3 particlePosition = m_Particles[i].m_Position;
        float particleDensity = Densities[i];
        DistributeParticle(particlePosition, particleDensity, MinX, MinY, MinZ);
    }
}

inline void fp_IsoVolume::DistributeParticle(
        D3DXVECTOR3 ParticlePosition,
        float ParticleDensity,
        float MinX,
        float MinY,
        float MinZ) {
    fp_VolumeIndex particleVolumeIndex;
    particleVolumeIndex.X = (ParticlePosition.x - MinX) / m_VoxelSize;
    particleVolumeIndex.Y = (ParticlePosition.y - MinY) / m_VoxelSize;
    particleVolumeIndex.Z = (ParticlePosition.z - MinZ) / m_VoxelSize;
    for (int i=0; i<m_NumStamp; i++) {
        fp_VolumeIndex destVolumeIndex = particleVolumeIndex + m_StampStarts[i];
        if(destVolumeIndex.X < 0 || destVolumeIndex.X >= m_NumValuesX
                || destVolumeIndex.Y < 0 || destVolumeIndex.Y >= m_NumValuesY)
            continue;
        int zEnd = destVolumeIndex.Z + m_StampZRowLengths[i];
        if(zEnd > m_NumValuesZ)
            zEnd = m_NumValuesZ;
        if(destVolumeIndex.Z < 0)
            destVolumeIndex.Z = 0;        
        for(; destVolumeIndex.Z < zEnd; destVolumeIndex.Z++) {
        }
    }
}

void fp_IsoVolume::CreateStamp() {
    
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
