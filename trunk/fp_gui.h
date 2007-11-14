#pragma once
#ifndef FP_GUI_H
#define FP_GUI_H

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"

#include "fp_global.h"

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------

#define IDC_TOGGLEFULLSCREEN         1
#define IDC_TOGGLEREF                3
#define IDC_CHANGEDEVICE             4
#define IDC_NUM_LIGHTS               6
#define IDC_NUM_LIGHTS_STATIC        7
#define IDC_ACTIVE_LIGHT             8
#define IDC_LIGHT_SCALE              9
#define IDC_LIGHT_SCALE_STATIC      10
#define IDC_PARTICLE_SCALE          11
#define IDC_PARTICLE_SCALE_STATIC   12

//--------------------------------------------------------------------------------------
// Fluid particles GUI
//--------------------------------------------------------------------------------------

class fp_GUI {
public:
    bool m_ShowHelp; // If true, it renders the UI control text
    D3DXVECTOR3 m_LightDir[FP_MAX_LIGHTS];
    D3DXVECTOR4 m_LightDiffuse[FP_MAX_LIGHTS];
    D3DXCOLOR m_LightDiffuseColor[FP_MAX_LIGHTS];

    fp_GUI();

    bool    RenderSettingsDialog(float ElapsedTime);     
    bool MsgProc(   // Returns true if no further processing is required
            HWND Wnd,
            UINT Msg,
            WPARAM wParam,
            LPARAM lParam,
            bool* NoFurtherProcessing,
            int ActiveLight);
    void    OnKeyboard( UINT Char, bool KeyDown, bool AltDown, void* UserContext );
    void    OnGUIEvent(
            UINT Event,
            int ControlID,
            CDXUTControl* Control,
            int& ActiveLight,
            int& NumActiveLights,
            float& LightScale,
            float& ParticeScale);

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
    CDXUTDialogResourceManager m_DialogResourceManager; // manager for shared resources of dialogs
    CDXUTDirectionWidget    m_LightControl[FP_MAX_LIGHTS];
    CD3DSettingsDlg         m_D3DSettingsDlg;       // Device settings dialog
    CDXUTDialog             m_HUD;                  // manages the 3D   
    CDXUTDialog             m_SampleUI;             // dialog for app specific controls    

    // Direct3D9 resources
    ID3DXFont*       m_Font9;         // Font for drawing text
    ID3DXSprite*     m_Sprite9;       // Sprite for batching draw text calls
    CDXUTTextHelper*        m_TxtHelper;

    // Direct3D10 resources
    ID3DX10Font*            m_Font10;       
    ID3DX10Sprite*          m_Sprite10;

    void    RenderText();
};

#endif
