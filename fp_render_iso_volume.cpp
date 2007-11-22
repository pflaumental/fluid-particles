#include "DXUT.h"
//#include "SDKmisc.h"
#include "fp_render_iso_volume.h"
//#include "fp_util.h"

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
