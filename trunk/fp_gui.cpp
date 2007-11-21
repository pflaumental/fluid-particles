#include "DXUT.h"
#include "fp_gui.h"

extern void CALLBACK FP_OnGUIEvent(
        UINT nEvent,
        int nControlID,
        CDXUTControl* pControl,
        void* pUserContext );

//--------------------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------------------
fp_GUI::fp_GUI()
        : m_ShowHelp(false), m_Font10(NULL), m_Font9(NULL), m_Sprite10(NULL),
        m_Sprite9(NULL), m_TxtHelper(NULL) {
    
    for( int i=0; i<FP_MAX_LIGHTS; i++ )
        m_LightControl[i].SetLightDirection( D3DXVECTOR3(
                sinf(D3DX_PI*2*i/FP_MAX_LIGHTS-D3DX_PI/6),
                0,
                -cosf(D3DX_PI*2*i/FP_MAX_LIGHTS-D3DX_PI/6) ) );

    m_D3DSettingsDlg.Init( &m_DialogResourceManager );
    m_HUD.Init( &m_DialogResourceManager );
    m_SampleUI.Init( &m_DialogResourceManager );

    m_HUD.SetCallback( FP_OnGUIEvent ); int iY = 10; 
    m_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    m_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    m_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22,
            VK_F2 );

    m_SampleUI.SetCallback( FP_OnGUIEvent ); iY = 10; 

    WCHAR sz[100];
    iY += 24;
    m_SampleUI.AddButton( IDC_RESET_SIM, L"Reset simulation (R)", 35, iY += 24,
            125, 22, 'R' );

    iY += 24;
    StringCchPrintf( sz, 100, L"Particle size: %0.2f", 1.0f ); 
    m_SampleUI.AddStatic( IDC_PARTICLE_SCALE_STATIC, sz, 35, iY += 24, 125, 22 );
    m_SampleUI.AddSlider( IDC_PARTICLE_SCALE, 50, iY += 24, 100, 22, 1, 100,
            (int) (1.0f * 50.0f) );

    iY += 24;
    StringCchPrintf( sz, 100, L"# Lights: %d", 1 ); 
    m_SampleUI.AddStatic( IDC_NUM_LIGHTS_STATIC, sz, 35, iY += 24, 125, 22 );
    m_SampleUI.AddSlider( IDC_NUM_LIGHTS, 50, iY += 24, 100, 22, 1, FP_MAX_LIGHTS, 1 );

    iY += 24;
    StringCchPrintf( sz, 100, L"Light scale: %0.2f", 1.0f ); 
    m_SampleUI.AddStatic( IDC_LIGHT_SCALE_STATIC, sz, 35, iY += 24, 125, 22 );
    m_SampleUI.AddSlider( IDC_LIGHT_SCALE, 50, iY += 24, 100, 22, 0, 20,
            (int) (1.0f * 10.0f) );

    iY += 24;
    m_SampleUI.AddButton( IDC_ACTIVE_LIGHT, L"Change active light (K)", 35, iY += 24,
            125, 22, 'K' );
}

bool fp_GUI::RenderSettingsDialog(float ElapsedTime) {
    if( !m_D3DSettingsDlg.IsActive() )
        return false;
    m_D3DSettingsDlg.OnRender( ElapsedTime );
    return true;   
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
bool fp_GUI::MsgProc(
        HWND Wnd,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam,
        bool* NoFurtherProcessing,
        int ActiveLight) {
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *NoFurtherProcessing = m_DialogResourceManager.MsgProc( Wnd, Msg, wParam,
            lParam );
    if( *NoFurtherProcessing )
        return true;

    // Pass messages to settings dialog if its active
    if( m_D3DSettingsDlg.IsActive() ) {
        m_D3DSettingsDlg.MsgProc( Wnd, Msg, wParam, lParam );
        return true;
    }

    // Give the dialogs a chance to handle the message first
    *NoFurtherProcessing = m_HUD.MsgProc( Wnd, Msg, wParam, lParam );
    if( *NoFurtherProcessing )
        return true;
    *NoFurtherProcessing = m_SampleUI.MsgProc( Wnd, Msg, wParam, lParam );
    if( *NoFurtherProcessing )
        return true;

    m_LightControl[ActiveLight].HandleMessages( Wnd, Msg, wParam, lParam );

    return false;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void fp_GUI::OnKeyboard( UINT Char, bool KeyDown, bool AltDown, void* UserContext ){
    if( KeyDown ) {
        switch( Char ) {
            case VK_F1: m_ShowHelp = !m_ShowHelp; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void fp_GUI::OnGUIEvent(
        UINT Event,
        int ControlID,
        CDXUTControl* Control,
        int& ActiveLight,
        int& NumActiveLights,
        float& LightScale,
        float& ParticleScale,
        bool& ResetSim) {   
    WCHAR sz[100];
    ResetSim = false;
    switch( ControlID ) {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     m_D3DSettingsDlg.SetActive(
                !m_D3DSettingsDlg.IsActive() ); break;

        case IDC_RESET_SIM:
            ResetSim = true;
            break;

        case IDC_PARTICLE_SCALE: 
            ParticleScale = (float) (m_SampleUI.GetSlider(
                    IDC_PARTICLE_SCALE )->GetValue() * 0.02f);

            StringCchPrintf( sz, 100, L"Particle scale: %0.2f", ParticleScale ); 
            m_SampleUI.GetStatic( IDC_PARTICLE_SCALE_STATIC )->SetText( sz );
            break;

        case IDC_ACTIVE_LIGHT:
            if( !m_LightControl[ActiveLight].IsBeingDragged() ) {
                ActiveLight++;
                ActiveLight %= NumActiveLights;
            }
            break;

        case IDC_NUM_LIGHTS:
            if( !m_LightControl[ActiveLight].IsBeingDragged() ) {
                StringCchPrintf( sz, 100, L"# Lights: %d", m_SampleUI.GetSlider(
                        IDC_NUM_LIGHTS )->GetValue() ); 
                m_SampleUI.GetStatic( IDC_NUM_LIGHTS_STATIC )->SetText( sz );

                NumActiveLights = m_SampleUI.GetSlider(
                        IDC_NUM_LIGHTS )->GetValue();
                ActiveLight %= NumActiveLights;
            }
            break;

        case IDC_LIGHT_SCALE: 
            LightScale = (float) (m_SampleUI.GetSlider(
                    IDC_LIGHT_SCALE )->GetValue() * 0.10f);

            StringCchPrintf( sz, 100, L"Light scale: %0.2f", LightScale ); 
            m_SampleUI.GetStatic( IDC_LIGHT_SCALE_STATIC )->SetText( sz );
            break;
    }
    
}



//--------------------------------------------------------------------------------------
// DX9 specific
//--------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------
// Create any D3D10 GUI resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT fp_GUI::OnD3D10CreateDevice(
        ID3D10Device* d3dDevice,
        const DXGI_SURFACE_DESC *BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;

    V_RETURN( m_DialogResourceManager.OnD3D10CreateDevice( d3dDevice ) );
    V_RETURN( m_D3DSettingsDlg.OnD3D10CreateDevice( d3dDevice ) );
    V_RETURN( D3DX10CreateFont( d3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial",
            &m_Font10 ) );
    V_RETURN( D3DX10CreateSprite( d3dDevice, 512, &m_Sprite10 ) );
    m_TxtHelper = new CDXUTTextHelper( m_Font9, m_Sprite9, m_Font10, m_Sprite10,
            15 );

    m_SampleUI.GetStatic( IDC_NUM_LIGHTS_STATIC )->SetVisible( false );
    m_SampleUI.GetSlider( IDC_NUM_LIGHTS )->SetVisible( false );
    m_SampleUI.GetButton( IDC_ACTIVE_LIGHT )->SetVisible( false );
    
    V_RETURN( CDXUTDirectionWidget::StaticOnD3D10CreateDevice( d3dDevice ) );
    for( int i=0; i<FP_MAX_LIGHTS; i++ )
        m_LightControl[i].SetRadius( FP_OBJECT_RADIUS );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 GUI resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT fp_GUI::OnD3D10ResizedSwapChain(
        ID3D10Device* d3dDevice,
        IDXGISwapChain *SwapChain,
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;

    V_RETURN( m_DialogResourceManager.OnD3D10ResizedSwapChain( d3dDevice,
            BackBufferSurfaceDesc ) );
    V_RETURN( m_D3DSettingsDlg.OnD3D10ResizedSwapChain( d3dDevice,
            BackBufferSurfaceDesc ) );

    m_HUD.SetLocation( BackBufferSurfaceDesc->Width-170, 0 );
    m_HUD.SetSize( 170, 170 );
    m_SampleUI.SetLocation( BackBufferSurfaceDesc->Width-170,
            BackBufferSurfaceDesc->Height-360 );
    m_SampleUI.SetSize( 170, 360 );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the GUI using the D3D10 device
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D10FrameRender(
        ID3D10Device* d3dDevice,
        double fTime,
        float ElapsedTime,
        const D3DXVECTOR3* EyePt,
        const D3DXMATRIX*  WorldViewProjection,
        const D3DXMATRIX*  World,
        const D3DXMATRIX*  View,
        const D3DXMATRIX*  Proj,
        int NumActiveLights,
        int ActiveLight,
        float LightScale) {
    HRESULT hr;
   
    // Render the light arrow so the user can visually see the light dir
    for( int i=0; i<NumActiveLights; i++ ) {
        D3DXCOLOR arrowColor = ( i == ActiveLight )
                ? D3DXVECTOR4(1,1,0,1)
                : D3DXVECTOR4(1,1,1,1);
        V( m_LightControl[i].OnRender10( arrowColor, View, Proj, EyePt ) );
        m_LightDir[i] = m_LightControl[i].GetLightDirection();
        m_LightDiffuse[i] = LightScale * D3DXVECTOR4(1,1,1,1);
    }

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    m_HUD.OnRender( ElapsedTime ); 
    m_SampleUI.OnRender( ElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D10 GUI resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D10ReleasingSwapChain( void* UserContext )
{
    m_DialogResourceManager.OnD3D10ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D10 GUI resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D10DestroyDevice( void* UserContext )
{
    m_DialogResourceManager.OnD3D10DestroyDevice();
    m_D3DSettingsDlg.OnD3D10DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D10DestroyDevice();
    SAFE_DELETE( m_TxtHelper );
    SAFE_RELEASE( m_Font10 );
    SAFE_RELEASE( m_Sprite10 );
}


//--------------------------------------------------------------------------------------
// DX9 specific
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Create any D3D9 GUI resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT fp_GUI::OnD3D9CreateDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;

    V_RETURN( m_DialogResourceManager.OnD3D9CreateDevice( d3dDevice ) );
    V_RETURN( m_D3DSettingsDlg.OnD3D9CreateDevice( d3dDevice ) );

    // Initialize the font
    V_RETURN( D3DXCreateFont( d3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Arial", &m_Font9 ) );

    m_SampleUI.GetStatic( IDC_NUM_LIGHTS_STATIC )->SetVisible( true );
    m_SampleUI.GetSlider( IDC_NUM_LIGHTS )->SetVisible( true );
    m_SampleUI.GetButton( IDC_ACTIVE_LIGHT )->SetVisible( true );

    V_RETURN( CDXUTDirectionWidget::StaticOnD3D9CreateDevice( d3dDevice ) );
    for( int i=0; i<FP_MAX_LIGHTS; i++ )
        m_LightControl[i].SetRadius( 10.0f );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 GUI resources that won't live through a device reset
// (D3DPOOL_DEFAULT) or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT fp_GUI::OnD3D9ResetDevice(
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;

    V_RETURN( m_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( m_D3DSettingsDlg.OnD3D9ResetDevice() );

    if( m_Font9 ) V_RETURN( m_Font9->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( d3dDevice, &m_Sprite9 ) );
    m_TxtHelper = new CDXUTTextHelper( m_Font9, m_Sprite9, NULL, NULL, 15 );

    for( int i=0; i<FP_MAX_LIGHTS; i++ )
        m_LightControl[i].OnD3D9ResetDevice( BackBufferSurfaceDesc  );

    m_HUD.SetLocation( BackBufferSurfaceDesc->Width-170, 0 );
    m_HUD.SetSize( 170, 170 );
    m_SampleUI.SetLocation( BackBufferSurfaceDesc->Width-170,
            BackBufferSurfaceDesc->Height-360 );
    m_SampleUI.SetSize( 170, 360 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the GUI using the D3D9 device
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D9FrameRender(
        IDirect3DDevice9* d3dDevice,
        double fTime,
        float fElapsedTime,
        const D3DXVECTOR3* vEyePt,
        const D3DXMATRIX*  mWorldViewProjection,
        const D3DXMATRIX*  mWorld,
        const D3DXMATRIX*  mView,
        const D3DXMATRIX*  mProj,
        int NumActiveLights,
        int nActiveLight,
        float fLightScale) {
    HRESULT hr;
   
    // Render the GUI

    // Render the light arrow so the user can visually see the light dir
    for( int i=0; i<NumActiveLights; i++ ) {
        D3DXCOLOR arrowColor = ( i == nActiveLight ) ? D3DXCOLOR(1,1,0,1) : D3DXCOLOR(1,1,1,1);
        V( m_LightControl[i].OnRender9( arrowColor, mView, mProj, vEyePt ) );
        m_LightDir[i] = m_LightControl[i].GetLightDirection();
        m_LightDiffuseColor[i] = fLightScale * D3DXCOLOR(1,1,1,1);
    }

    m_HUD.OnRender( fElapsedTime );
    m_SampleUI.OnRender( fElapsedTime );

    RenderText();
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D9LostDevice( void* UserContext )
{
    m_DialogResourceManager.OnD3D9LostDevice();
    m_D3DSettingsDlg.OnD3D9LostDevice();
    CDXUTDirectionWidget::StaticOnD3D9LostDevice();
    if( m_Font9 ) m_Font9->OnLostDevice();
    SAFE_RELEASE(m_Sprite9);
    SAFE_DELETE(m_TxtHelper);
    
}


//--------------------------------------------------------------------------------------
// Release D3D9 GUI resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void fp_GUI::OnD3D9DestroyDevice( void* UserContext )
{
    m_DialogResourceManager.OnD3D9DestroyDevice();
    m_D3DSettingsDlg.OnD3D9DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D9DestroyDevice();    
    SAFE_RELEASE(m_Font9);
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void fp_GUI::RenderText() {
    m_TxtHelper->Begin();
    m_TxtHelper->SetInsertionPos( 2, 0 );
    m_TxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    m_TxtHelper->DrawTextLine( DXUTGetFrameStats( true/*DXUTIsVsyncEnabled()*/ ) );  
    m_TxtHelper->DrawTextLine( DXUTGetDeviceStats() );    
  
    // Draw help
    if( m_ShowHelp ) {
        UINT nBackBufferHeight = ( DXUTIsAppRenderingWithD3D9() )
                ? DXUTGetD3D9BackBufferSurfaceDesc()->Height
                : DXUTGetDXGIBackBufferSurfaceDesc()->Height;
        m_TxtHelper->SetInsertionPos( 2, nBackBufferHeight-15*6 );
        m_TxtHelper->SetForegroundColor( D3DXCOLOR(1.0f, 0.75f, 0.0f, 1.0f ) );
        m_TxtHelper->DrawTextLine( L"Controls:" );
        m_TxtHelper->DrawFormattedTextLine( L"Time: %f", DXUTGetTime() );
        DXUTGetGlobalTimer()->LimitThreadAffinityToCurrentProc();
/*
        m_TxtHelper->SetInsertionPos( 20, nBackBufferHeight-15*5 );
        m_TxtHelper->DrawTextLine( L"Rotate model: Left mouse button\n"
                                L"Rotate light: Right mouse button\n"
                                L"Rotate camera: Middle mouse button\n"
                                L"Zoom camera: Mouse wheel scroll\n" );

        m_TxtHelper->SetInsertionPos( 250, nBackBufferHeight-15*5 );
        m_TxtHelper->DrawTextLine( L"Hide help: F1\n" 
                                L"Quit: ESC\n" );*/
    } else {
        m_TxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        //m_TxtHelper->DrawTextLine( L"Press F1 for help" );
        m_TxtHelper->DrawFormattedTextLine( L"Time: %f", DXUTGetTime() );
    }

    m_TxtHelper->End();
}
