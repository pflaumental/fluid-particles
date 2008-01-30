#include "DXUT.h"
//#include "SDKmisc.h"
#include "fp_render_raycast.h"
#include "fp_util.h"

#define FP_RENDER_RAYCAST_EFFECT_FILE L"fp_render_raycast.fx" 

const D3D10_INPUT_ELEMENT_DESC fp_SplatParticleVertex::Layout[] = {
        {"POSITION_DENSITY",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
                D3D10_INPUT_PER_VERTEX_DATA, 0 }};

fp_RenderRaycast::fp_RenderRaycast(
        fp_Fluid* Fluid,
        float VoxelSize,
        float IsoLevel,
        const fp_VolumeIndex& VolumeDimensions) :
    m_VoxelSize(VoxelSize),
    m_IsoLevel(IsoLevel),
    m_VolumeDimensions(VolumeDimensions),
    m_VolumeTexture(NULL),
    m_VolumeRTV(NULL),
    m_VolumeSRV(NULL),
    m_ExitPoint(NULL),
    m_WValsMulParticleMassTexture(NULL),
    m_WValsMulParticleMassSRV(NULL),
    m_SplatParticleVertexLayout(NULL),
    m_SplatParticleVertexBuffer(NULL), 
    m_Effect(NULL),
    m_TechRenderRaycast(NULL),
    m_EffectVarCornersPos(NULL),
    m_EffectVarHalfParticleVoxelDiameter(NULL),
    m_EffectVarParticleVoxelRadius(NULL),
    m_EffectVarVolumeDimensions(NULL),
    m_EffectVarWorldToNDS(NULL),
    m_EffectVarWValsMulParticleMass(NULL) {
    SetFluid(Fluid);
}

fp_RenderRaycast::~fp_RenderRaycast() {
}

HRESULT fp_RenderRaycast::OnD3D9CreateDevice(
        IDirect3DDevice9* D3DDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    //HRESULT hr;
    return S_OK;
}

HRESULT fp_RenderRaycast::OnD3D9ResetDevice(
        IDirect3DDevice9* D3DDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    return S_OK;
}

void fp_RenderRaycast::OnD3D9FrameRender(IDirect3DDevice9* D3DDevice) {  
}

void fp_RenderRaycast::OnD3D9DestroyDevice(void* UserContext) {

}

void fp_RenderRaycast::OnD3D9LostDevice(void* UserContext) {    
}


HRESULT fp_RenderRaycast::OnD3D10CreateDevice(
        ID3D10Device* D3DDevice,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    V_RETURN(CreateVolumeTexture(D3DDevice));

    // CreateWValsMulParticleMass texture
    D3D10_TEXTURE1D_DESC texDesc;
    texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE; 
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.MiscFlags = 0;
    texDesc.Usage = D3D10_USAGE_DYNAMIC;
    texDesc.Width = m_WValsMulParticleMassLength = 32;
    V_RETURN(D3DDevice->CreateTexture1D(&texDesc, NULL, &m_WValsMulParticleMassTexture));

    // Create CreateWValsMulParticleMass shader sesource view
    D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof(srvDesc) );
    srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE1D;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;
    V_RETURN(D3DDevice->CreateShaderResourceView(m_WValsMulParticleMassTexture,
            &srvDesc, &m_WValsMulParticleMassSRV));

    // Read the D3DX effect file
    m_Effect = fp_Util::LoadEffect(D3DDevice, FP_RENDER_RAYCAST_EFFECT_FILE);

    // Obtain technique objects
    m_TechRenderRaycast = m_Effect->GetTechniqueByName("RenderRaycast");

    // Obtain effect variables
    m_EffectVarCornersPos = m_Effect->GetVariableByName("g_CornersPos")
            ->AsVector();
    m_EffectVarHalfParticleVoxelDiameter = m_Effect->GetVariableByName(
            "g_HalfParticleVoxelDiameter")->AsScalar();
    m_EffectVarParticleVoxelRadius = m_Effect->GetVariableByName(
            "g_ParticleVoxelRadius")->AsScalar();
    m_EffectVarVolumeDimensions = m_Effect->GetVariableByName("g_VolumeDimensions")
            ->AsVector();
    m_EffectVarWorldToNDS = m_Effect->GetVariableByName("g_WorldToNDS")->AsMatrix();    
    m_EffectVarWValsMulParticleMass = m_Effect->GetVariableByName(
            "g_WValsMulParticleMass")->AsShaderResource();
    m_EffectVarBBoxStart = m_Effect->GetConstantBufferByName(
            "g_BBoxStart")->AsVector();
    m_EffectVarBBoxSize = m_Effect->GetConstantBufferByName(
            "g_BBoxSize")->AsVector();
    m_EffectVarWorldViewProjection = m_Effect->GetVariableByName(
            "g_WorldViewProjection")->AsMatrix();
    bool allValid = m_EffectVarCornersPos->IsValid();
    allValid = allValid || m_EffectVarHalfParticleVoxelDiameter->IsValid();
    allValid = allValid || m_EffectVarParticleVoxelRadius->IsValid();
    allValid = allValid || m_EffectVarVolumeDimensions->IsValid();
    allValid = allValid || m_EffectVarWorldToNDS->IsValid();    
    allValid = allValid || m_EffectVarWValsMulParticleMass->IsValid();
    allValid = allValid || m_EffectVarBBoxStart->IsValid();
    allValid = allValid || m_EffectVarBBoxSize->IsValid();
    allValid = allValid || m_EffectVarWorldViewProjection->IsValid();
    if(!allValid) return E_FAIL;

    // Set effect variables as needed
    V_RETURN(m_EffectVarVolumeDimensions->SetIntVector((int*)&m_VolumeDimensions));
    SetVoxelSize(m_VoxelSize);
    SetVolumeStartPos(&m_VolumeStartPos);

    // Create vertex buffer
    D3D10_PASS_DESC passDesc;
    V_RETURN( m_TechRenderRaycast->GetPassByIndex(0)->GetDesc(&passDesc));
    int numElements = sizeof(fp_SplatParticleVertex::Layout)
            / sizeof(fp_SplatParticleVertex::Layout[0]);
    V_RETURN(D3DDevice->CreateInputLayout(fp_SplatParticleVertex::Layout, numElements,
            passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_SplatParticleVertexLayout));

    D3D10_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_NumParticles * sizeof(fp_SplatParticleVertex);
    bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    V_RETURN(D3DDevice->CreateBuffer(&bufferDesc, NULL, &m_SplatParticleVertexBuffer));

    return S_OK;
}

HRESULT fp_RenderRaycast::OnD3D10ResizedSwapChain(
        ID3D10Device* D3DDevice,
        IDXGISwapChain *SwapChain,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    // Create exit point render target
    m_ExitPoint = new fp_RenderTarget2D(D3DDevice, BackBufferSurfaceDesc->Width,
            BackBufferSurfaceDesc->Height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    return S_OK;
}

void fp_RenderRaycast::OnD3D10FrameRender(
        ID3D10Device* D3DDevice,
        const D3DXMATRIX*  WorldViewProjection) {  
    //HRESULT hr;
    FillVolumeTexture(D3DDevice);
    RenderVolume(D3DDevice, WorldViewProjection);
}

void fp_RenderRaycast::OnD3D10DestroyDevice( void* UserContext ) {
    DestroyVolumeTexture();   
    SAFE_RELEASE(m_WValsMulParticleMassTexture);
    SAFE_RELEASE(m_WValsMulParticleMassSRV);
    SAFE_RELEASE(m_SplatParticleVertexLayout);
    SAFE_RELEASE(m_SplatParticleVertexBuffer);
    SAFE_RELEASE(m_Effect);
}

void fp_RenderRaycast::OnD3D10ReleasingSwapChain( void* UserContext ) {
    SAFE_DELETE(m_ExitPoint);
}

void fp_RenderRaycast::SetFluid(fp_Fluid* Fluid) {
    m_Fluid = Fluid;
    m_Particles = Fluid->m_Particles;
    m_NumParticles = Fluid->m_NumParticles;
    SetVoxelSize(m_VoxelSize);
}

void fp_RenderRaycast::SetVoxelSize(float VoxelSize) {
    HRESULT hr;
    m_VoxelSize = VoxelSize;
    m_ParticleVoxelRadius = ceil(m_Fluid->m_SmoothingLength / m_VoxelSize);
    m_ParticleVoxelDiameter = m_ParticleVoxelRadius * 2 + 1;
    if(m_Effect != NULL) {
        V(m_EffectVarParticleVoxelRadius->SetInt((int)m_ParticleVoxelRadius));
        V(m_EffectVarHalfParticleVoxelDiameter->SetFloat(0.5f
                * m_ParticleVoxelDiameter));
        float offsetX = (float)m_ParticleVoxelRadius / (float)m_VolumeDimensions.x;
        float offsetY = (float)m_ParticleVoxelRadius / (float)m_VolumeDimensions.y;
        D3DXVECTOR4 cornersPos[4] = {
                D3DXVECTOR4(-offsetX, offsetY, 0, 0),
                D3DXVECTOR4(offsetX, offsetY, 0, 0),
                D3DXVECTOR4(-offsetX, -offsetY, 0, 0),
                D3DXVECTOR4(offsetX, -offsetY, 0, 0) };
        V(m_EffectVarCornersPos->SetFloatVectorArray((float*)cornersPos, 0, 4));

        // Fill the WValsMulParticleMass texture
        float* mappedTexture;
        V(m_WValsMulParticleMassTexture->Map(0, D3D10_MAP_WRITE_DISCARD, 0,
                (void**)&mappedTexture));
        for(int i=0; i < m_WValsMulParticleMassLength; i++) {
            float r = (float)i / m_WValsMulParticleMassLength * m_Fluid
                    ->m_SmoothingLength;
            float hSq_LenRSq = m_Fluid->m_SmoothingLengthSq - r*r;
            mappedTexture[i] = m_Fluid->WPoly6(hSq_LenRSq) * m_Fluid->m_ParticleMass;
        }
        m_WValsMulParticleMassTexture->Unmap(0);

        // Set bounding box size
        D3DXVECTOR3 bBoxSize = GetVolumeSize();
        V(m_EffectVarBBoxSize->SetFloatVector((float*)&bBoxSize));
    }
}

D3DXVECTOR3 fp_RenderRaycast::GetVolumeSize() {
    D3DXVECTOR3 result;
    result.x = m_VolumeDimensions.x * m_VoxelSize;
    result.y = m_VolumeDimensions.y * m_VoxelSize;
    result.z = m_VolumeDimensions.z * m_VoxelSize;
    return result;
}

fp_VolumeIndex fp_RenderRaycast::GetVolumeTextureSize() {
    return m_VolumeDimensions;
}

void fp_RenderRaycast::SetVolumeStartPos(D3DXVECTOR3* VolumeStartPos) {
    HRESULT hr;

    m_VolumeStartPos = *VolumeStartPos;

    if(m_Effect != NULL) {
        // Set bounding box start pos
        V(m_EffectVarBBoxStart->SetFloatVector((float*)m_VolumeStartPos));

        // Set WorldToNDS matrix

        D3DXMATRIX worldToNDS, tmpMatrix;    
        D3DXVECTOR3 volumeSize = GetVolumeSize();
        D3DXVECTOR3 volumeCenter = m_VolumeStartPos + 0.5f * volumeSize;
        // Volume center becomes new origin
        D3DXMatrixTranslation(&worldToNDS, -volumeCenter.x, -volumeCenter.y,
                -volumeCenter.z);
        // Scale all coords inside volume to [-1,1] range    
        D3DXMatrixScaling(&tmpMatrix, 2.0f / volumeSize.x, 2.0f / volumeSize.y, 2.0f
                / volumeSize.z);
        D3DXMatrixMultiply(&worldToNDS, &worldToNDS, &tmpMatrix);
        V(m_EffectVarWorldToNDS->SetMatrix((float*)&worldToNDS));
    }
}

HRESULT fp_RenderRaycast::CreateVolumeTexture(ID3D10Device* D3DDevice) {
    HRESULT hr;

    // Create the texture
    D3D10_TEXTURE3D_DESC texDesc;
    texDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0; 
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R16_FLOAT;
    texDesc.MiscFlags = 0;
    texDesc.Usage = D3D10_USAGE_DEFAULT;
    texDesc.Width =  m_VolumeDimensions.x;
    texDesc.Height = m_VolumeDimensions.y;
    texDesc.Depth =  m_VolumeDimensions.z;
    V_RETURN(D3DDevice->CreateTexture3D(&texDesc,NULL,&m_VolumeTexture));

    // Create the render target view
    D3D10_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension =  D3D10_RTV_DIMENSION_TEXTURE3D;
    rtvDesc.Texture3D.FirstWSlice = 0;
    rtvDesc.Texture3D.MipSlice = 0;
    rtvDesc.Texture3D.WSize = texDesc.Depth;
    V_RETURN(D3DDevice->CreateRenderTargetView(m_VolumeTexture, &rtvDesc,
            &m_VolumeRTV));

    // Create Shader Resource View
    D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof(srvDesc) );
    srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;
    V_RETURN(D3DDevice->CreateShaderResourceView(m_VolumeTexture, &srvDesc,
            &m_VolumeSRV));

    return S_OK;
}

void fp_RenderRaycast::FillVolumeTexture(ID3D10Device* D3DDevice) {    
    fp_SplatParticleVertex *splatVertices;
    float* densities = m_Fluid->GetDensities();
    m_SplatParticleVertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&splatVertices);
    for( int i = 0; i < m_NumParticles; i++ ) {
        D3DXVECTOR3 pos = m_Particles[i].m_Position;
        splatVertices->m_PositionAndDensity = D3DXVECTOR4(pos.x, pos.y, pos.z,
                densities[i]);
        splatVertices++;
    }
    m_SplatParticleVertexBuffer->Unmap();  

    ID3D10RenderTargetView *oldRTV;
    ID3D10DepthStencilView *oldDSV;
    D3DDevice->OMGetRenderTargets(1, &oldRTV, &oldDSV);
    
    D3D10_VIEWPORT rtViewport, oldViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;
    rtViewport.Width =  m_VolumeDimensions.x;
    rtViewport.Height = m_VolumeDimensions.y;

    UINT numViewports = 1;
    D3DDevice->RSGetViewports(&numViewports, &oldViewport);
    D3DDevice->RSSetViewports(1, &rtViewport);

    // Set vertex Layout
    D3DDevice->IASetInputLayout(m_SplatParticleVertexLayout);

    // Set IA parameters
    UINT strides[1] = {sizeof(fp_SplatParticleVertex)};
    UINT offsets[1] = {0};
    D3DDevice->IASetVertexBuffers(0, 1, &m_SplatParticleVertexBuffer, strides, offsets);
    D3DDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_EffectVarWValsMulParticleMass->SetResource(m_WValsMulParticleMassSRV);

    D3DDevice->OMSetRenderTargets(1, &m_VolumeRTV , NULL); 

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    D3DDevice->ClearRenderTargetView(m_VolumeRTV, clearColor);

    m_TechRenderRaycast->GetPassByIndex(0)->Apply(0);
    
    D3DDevice->DrawInstanced(m_NumParticles, m_ParticleVoxelDiameter, 0, 0);

    D3DDevice->RSSetViewports(1, &oldViewport);

    D3DDevice->OMSetRenderTargets(1, &oldRTV, oldDSV);
    SAFE_RELEASE(oldDSV);
    SAFE_RELEASE(oldRTV);
}

void fp_RenderRaycast::DestroyVolumeTexture() {
    SAFE_RELEASE(m_VolumeTexture);
    SAFE_RELEASE(m_VolumeRTV);
    SAFE_RELEASE(m_VolumeSRV);
}

void fp_RenderRaycast::RenderVolume(
        ID3D10Device* D3DDevice,
        const D3DXMATRIX*  WorldViewProjection) {
    m_EffectVarWorldViewProjection->SetMatrix((float*)WorldViewProjection);
    m_ExitPoint->Bind(true, false);
    m_ExitPoint->Unbind();

}
