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

#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_NUM_LIGHTS          6
#define IDC_NUM_LIGHTS_STATIC   7
#define IDC_ACTIVE_LIGHT        8
#define IDC_LIGHT_SCALE         9
#define IDC_LIGHT_SCALE_STATIC  10

//--------------------------------------------------------------------------------------
// Fluid particles GUI
//--------------------------------------------------------------------------------------

class fp_GUI {
public:
    bool m_bShowHelp; // If true, it renders the UI control text
    D3DXVECTOR3 m_LightDir[FP_MAX_LIGHTS];
    D3DXVECTOR4 m_LightDiffuse[FP_MAX_LIGHTS];
    D3DXCOLOR m_cLightDiffuse[FP_MAX_LIGHTS];

    fp_GUI();

    bool    RenderSettingsDialog(float fElapsedTime);     
    bool MsgProc(   // Returns true if no further processing is required
            HWND hWnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam,
            bool* pbNoFurtherProcessing,
            int nActiveLight);
    void    OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
    void    OnGUIEvent(
            UINT nEvent,
            int nControlID,
            CDXUTControl* pControl,
            int& rnActiveLight,
            int& NumActiveLights,
            float& rfLightScale);

    // DX9 specific
    HRESULT OnD3D9CreateDevice(
            IDirect3DDevice9* pd3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
            void* pUserContext );
    HRESULT OnD3D9ResetDevice(
            IDirect3DDevice9* pd3dDevice,
            const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
            void* pUserContext );
    void    OnD3D9FrameRender(
            IDirect3DDevice9* pd3dDevice,
            double fTime,
            float fElapsedTime,
            const D3DXVECTOR3* vEyePt,
            const D3DXMATRIX*  mWorldViewProjection,
            const D3DXMATRIX*  mWorld,
            const D3DXMATRIX*  mView,
            const D3DXMATRIX*  mProj,
            int NumActiveLights,
            int nActiveLight,
            float fLightScale);
    void    OnD3D9LostDevice( void* pUserContext );
    void    OnD3D9DestroyDevice( void* pUserContext );

    // DX10 specific
    HRESULT OnD3D10CreateDevice(
            ID3D10Device* pd3dDevice,
            const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
            void* pUserContext );
    HRESULT OnD3D10ResizedSwapChain(
            ID3D10Device* pd3dDevice,
            IDXGISwapChain *pSwapChain,
            const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
            void* pUserContext );
    void    OnD3D10ReleasingSwapChain( void* pUserContext );
    void    OnD3D10DestroyDevice( void* pUserContext );
    void    OnD3D10FrameRender(
            ID3D10Device* pd3dDevice,
            double fTime,
            float fElapsedTime,
            const D3DXVECTOR3* vEyePt,
            const D3DXMATRIX*  mWorldViewProjection,
            const D3DXMATRIX*  mWorld,
            const D3DXMATRIX*  mView,
            const D3DXMATRIX*  mProj,
            int NumActiveLights,
            int nActiveLight,
            float fLightScale);    

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
