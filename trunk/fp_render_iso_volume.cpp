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

fp_IsoVolume::fp_IsoVolume(fp_Fluid* Fluid, float VoxelSize) 
        :
        m_Fluid(Fluid),
        m_NumValues(0),
        m_NumValuesXY(0),
        m_NumValuesX(0),
        m_NumValuesY(0),
        m_NumValuesZ(0),        
        m_IsoValues(),
        m_StampRowLengths(NULL),
        m_StampRowStartOffsets(NULL),
        m_StampRowValueStarts(NULL),
        m_StampValues(NULL){
    m_VoxelSize = -1.0f; // Needed in SetSmoothingLength()
    UpdateSmoothingLength();
    SetVoxelSize(VoxelSize);
    m_IsoValues.reserve(FP_INITIAL_ISOVOLUME_CAPACITY);
}

void fp_IsoVolume::UpdateSmoothingLength() {
    if(m_VoxelSize > 0.0f) {
        if(m_StampValues != NULL)
            DestroyStamp();
        CreateStamp();
    }
}

void fp_IsoVolume::SetVoxelSize(float VoxelSize) {
    m_VoxelSize = VoxelSize;
    m_HalfVoxelSize = VoxelSize / 2.0f;
    if(m_StampValues != NULL)
        DestroyStamp();
    CreateStamp();
}

void fp_IsoVolume::ConstructFromFluid() {
    float MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
    m_Fluid->GetParticleMinsAndMaxs(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);
    m_NumValuesX = (int)((MaxX - MinX + m_VoxelSize) / m_VoxelSize);
    m_NumValuesY = (int)((MaxY - MinY + m_VoxelSize) / m_VoxelSize);
    m_NumValuesZ = (int)((MaxZ - MinZ + m_VoxelSize) / m_VoxelSize);
    m_NumValuesXY = m_NumValuesX * m_NumValuesY;
    m_NumValues = m_NumValuesXY * m_NumValuesZ;
    m_IsoValues.clear();
    m_IsoValues.resize(m_NumValues, 0.0f);
    int NumParticles = m_Fluid->m_NumParticles;
    fp_FluidParticle* Particles = m_Fluid->m_Particles;
    float* Densities = m_Fluid->GetDensities();
    for (int i = 0; i < NumParticles; i++) {
        D3DXVECTOR3 particlePosition = Particles[i].m_Position;
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
    particleVolumeIndex.X = (int)((ParticlePosition.x - MinX) / m_VoxelSize);
    particleVolumeIndex.Y = (int)((ParticlePosition.y - MinY) / m_VoxelSize);
    particleVolumeIndex.Z = (int)((ParticlePosition.z - MinZ) / m_VoxelSize);
    for (int stampRowIndex = 0; stampRowIndex < m_NumStampRows; stampRowIndex++) {
        fp_VolumeIndex destStart = particleVolumeIndex
                + m_StampRowStartOffsets[stampRowIndex];
        if(destStart.X < 0 || destStart.X >= m_NumValuesX
                || destStart.Y < 0 || destStart.Y >= m_NumValuesY)
            continue;

        int destIndexXY = m_NumValuesXY * destStart.X
            + m_NumValuesY * destStart.Y;
        int destIndex = destStart.Z < 0 ? destIndexXY : destIndexXY + destStart.Z;
        int zEnd = destStart.Z + m_StampRowLengths[stampRowIndex];
        if(zEnd > m_NumValuesZ)
            zEnd = m_NumValuesZ;
        int destEndIndex = destIndexXY + zEnd;

        int stampValueIndex = m_StampRowValueStarts[stampRowIndex];

        for(; destIndex < destEndIndex; destIndex++) {
            m_IsoValues[destIndex] += m_StampValues[stampValueIndex++] * ParticleDensity;
        }
    }
}

void fp_IsoVolume::CreateStamp() {
    int stampRadius = 2 * (int)((m_Fluid->m_SmoothingLength - m_HalfVoxelSize) 
            / m_VoxelSize);
    int stampSideLength = 1 + 2 * stampRadius;
    int stampSideLengthSq = stampSideLength * stampSideLength;    
    
    float halfLength = stampSideLength * m_HalfVoxelSize;
    D3DXVECTOR3 mid(halfLength, halfLength, halfLength);
    
    float* cubeStampDistancesSq = new float[stampSideLength * stampSideLength
            * stampSideLength];
    fp_VolumeIndex* cubeStampRowStarts = new fp_VolumeIndex[stampSideLengthSq];
    int* cubeStampRowLengths = new int[stampSideLengthSq];
    int m_NumStampValues = 0;
    int m_NumStampRows = 0;

    for (int stampX = 0; stampX < stampSideLength; stampX++) {
        for (int stampY = 0; stampY < stampSideLength; stampY++) {
            int stampIndexXY = stampX * stampSideLengthSq + stampY * stampSideLength;
            int cubeStampRowIndex = stampX * stampSideLength + stampY;
            cubeStampRowLengths[cubeStampRowIndex] = 0;
            for (int stampZ = 0; stampZ < stampSideLength; stampZ++) {
                D3DXVECTOR3 coord(
                        stampX * m_VoxelSize + m_HalfVoxelSize,
                        stampY * m_VoxelSize + m_HalfVoxelSize,
                        stampZ * m_VoxelSize + m_HalfVoxelSize);
                D3DXVECTOR3 toMid = mid - coord;
                float distSq = D3DXVec3LengthSq(&toMid);
                if(distSq < m_Fluid->m_SmoothingLengthSq) {
                    int stampIndex = stampIndexXY + stampZ;
                    cubeStampDistancesSq[stampIndex] = distSq;
                    if(cubeStampRowLengths[cubeStampRowIndex] == 0) {
                        m_NumStampRows++;
                        fp_VolumeIndex rowStart;
                        rowStart.X = stampX;
                        rowStart.Y = stampY;
                        rowStart.Z = stampZ;
                        cubeStampRowStarts[cubeStampRowIndex] = rowStart;
                    }
                    m_NumStampValues++;
                    cubeStampRowLengths[cubeStampRowIndex]++;
                } else {
                    cubeStampDistancesSq[stampIndexXY + stampZ] = -1.0f;
                }
            }
        }
    }
    m_StampRowLengths = new int[m_NumStampRows];
    m_StampRowStartOffsets = new fp_VolumeIndex[m_NumStampRows];
    m_StampRowValueStarts = new int[m_NumStampRows];
    m_StampValues = new float[m_NumStampValues];
    int stampRowIndex = 0;
    int stampValueIndex = 0;
    for (int cubeRow = 0; cubeRow < stampSideLengthSq; cubeRow++) {
        int rowLength = cubeStampRowLengths[cubeRow];
        if(rowLength == 0)
            continue;
        m_StampRowLengths[stampRowIndex] = rowLength;
        fp_VolumeIndex cubeStampVolumeIndex = cubeStampRowStarts[cubeRow];
        fp_VolumeIndex rowStartOffset;
        rowStartOffset.X = cubeStampVolumeIndex.X - stampRadius;
        rowStartOffset.Y = cubeStampVolumeIndex.Y - stampRadius;
        rowStartOffset.Z = cubeStampVolumeIndex.Z - stampRadius;
        m_StampRowStartOffsets[stampRowIndex] = rowStartOffset;
        m_StampRowValueStarts[stampRowIndex] = stampValueIndex;
        int cubeStampIndex = cubeStampVolumeIndex.X * stampSideLengthSq
                + cubeStampVolumeIndex.Y * stampSideLength
                + stampRadius - (rowLength - 1) / 2;
        int cubeStampEnd = cubeStampIndex + rowLength;
        for (; cubeStampIndex < cubeStampEnd; cubeStampIndex++) {            
            m_StampValues[stampValueIndex]
                    = m_Fluid->WPoly6(cubeStampDistancesSq[cubeStampIndex]);
            stampValueIndex++;
        }
        stampRowIndex++;
    }
}

void fp_IsoVolume::DestroyStamp() {
    delete[] m_StampRowLengths;
    delete[] m_StampRowStartOffsets;
    delete[] m_StampRowValueStarts;
    delete[] m_StampValues;
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
