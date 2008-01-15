#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "resource.h"

#include "fp_gui.h"
#include "fp_global.h"
#include "fp_util.h"
#include "fp_cpu_sph.h"
#include "fp_render_sprites.h"
#include "fp_render_iso_volume.h"
#include "fp_thread.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 



//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

CModelViewerCamera      g_Camera;               // A model viewing camera
fp_GUI                  g_GUI;

fp_Fluid*           g_Sim = NULL;
fp_RenderSprites*   g_RenderSprites = NULL;
fp_IsoVolume*       g_IsoVolume = NULL;
fp_RenderIsoVolume* g_RenderIsoVolume = NULL;

float                   g_LightScale;
int                     g_NumActiveLights;
int                     g_ActiveLight;
int                     g_RenderType;
bool                    g_MoveHorizontally;

// Direct3D10 resources
ID3D10EffectVectorVariable* g_LightDir = NULL;
ID3D10EffectVectorVariable* g_LightDiffuse = NULL;
ID3D10EffectMatrixVariable* g_WorldViewProjection = NULL;
ID3D10EffectMatrixVariable* g_World = NULL;
ID3D10EffectScalarVariable* g_Time = NULL;
ID3D10EffectVectorVariable* g_MaterialDiffuseColor = NULL;
ID3D10EffectVectorVariable* g_MaterialAmbientColor = NULL;
ID3D10EffectScalarVariable* g_NumLights = NULL;

fp_WorkerThreadManager g_WorkerThreadMgr;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK FP_ModifyDeviceSettings(
        DXUTDeviceSettings* DeviceSettings,
        void* UserContext );
void    CALLBACK FP_OnFrameMove( 
        double Time, 
        float ElapsedTime, 
        void* UserContext );
void    CALLBACK FP_OnFrameMoveInitial(
        double Time, 
        float ElapsedTime, 
        void* UserContext );
LRESULT CALLBACK FP_MsgProc( 
        HWND hWnd, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam, 
        bool* NoFurtherProcessing, 
        void* UserContext );
void    CALLBACK FP_OnKeyboard( 
        UINT Char, 
        bool KeyDown, 
        bool AltDown, 
        void* UserContext );
void    CALLBACK FP_OnGUIEvent( 
        UINT Event, 
        int ControlID, 
        CDXUTControl* Control, 
        void* UserContext );

extern bool    CALLBACK FP_IsD3D9DeviceAcceptable( 
        D3DCAPS9* Caps, 
        D3DFORMAT AdapterFormat, 
        D3DFORMAT BackBufferFormat, 
        bool Windowed, 
        void* UserContext );
extern HRESULT CALLBACK FP_OnD3D9CreateDevice( 
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext );
extern HRESULT CALLBACK FP_OnD3D9ResetDevice( 
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext );
extern void    CALLBACK FP_OnD3D9FrameRender( 
        IDirect3DDevice9* d3dDevice, 
        double Time, 
        float ElapsedTime, 
        void* UserContext );
extern void    CALLBACK FP_OnD3D9LostDevice( void* UserContext );
extern void    CALLBACK FP_OnD3D9DestroyDevice( void* UserContext );

bool    CALLBACK FP_IsD3D10DeviceAcceptable( 
        UINT Adapter, 
        UINT Output, 
        D3D10_DRIVER_TYPE DeviceType, 
        DXGI_FORMAT BackBufferFormat, 
        bool Windowed, 
        void* UserContext );
HRESULT CALLBACK FP_OnD3D10CreateDevice( 
        ID3D10Device* d3dDevice, 
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext );
HRESULT CALLBACK FP_OnD3D10ResizedSwapChain( 
        ID3D10Device* d3dDevice, 
        IDXGISwapChain *SwapChain, 
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext );
void    CALLBACK FP_OnD3D10ReleasingSwapChain( void* UserContext );
void    CALLBACK FP_OnD3D10DestroyDevice( void* UserContext );
void    CALLBACK FP_OnD3D10FrameRender( 
        ID3D10Device* d3dDevice, 
        double Time, 
        float ElapsedTime, 
        void* UserContext );

void    FP_InitApp();
void    FP_FinishApp();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(
        HINSTANCE Instance, 
        HINSTANCE PrevInstance, 
        LPWSTR CmdLine, 
        int CmdShow ) {
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( FP_ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( FP_MsgProc );
    DXUTSetCallbackKeyboard( FP_OnKeyboard );
    DXUTSetCallbackFrameMove( FP_OnFrameMoveInitial );

    DXUTSetCallbackD3D9DeviceAcceptable( FP_IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( FP_OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( FP_OnD3D9ResetDevice );
    DXUTSetCallbackD3D9FrameRender( FP_OnD3D9FrameRender );
    DXUTSetCallbackD3D9DeviceLost( FP_OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( FP_OnD3D9DestroyDevice );

    DXUTSetCallbackD3D10DeviceAcceptable( FP_IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( FP_OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( FP_OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10FrameRender( FP_OnD3D10FrameRender );
    DXUTSetCallbackD3D10SwapChainReleasing( FP_OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( FP_OnD3D10DestroyDevice );

    FP_InitApp();

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Fluid particles" );
    DXUTCreateDevice( true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop    

    FP_FinishApp();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void FP_InitApp() {
    g_ActiveLight = 0;
    g_NumActiveLights = 1;
    g_LightScale = FP_DEFAULT_LIGHT_SCALE;
    D3DXVECTOR3 center(0.0f, 0.0f, 0.0f);
    g_Sim = new fp_Fluid(&g_WorkerThreadMgr, FP_NUM_PARTICLES_X, FP_NUM_PARTICLES_Y,
            FP_NUM_PARTICLES_Z, FP_PARTICLE_SPACING_X, FP_PARTICLE_SPACING_Y,
            FP_PARTICLE_SPACING_Z, center, FP_GLASS_RADIUS, FP_GLASS_FLOOR);
    g_RenderSprites = new fp_RenderSprites(FP_NUM_PARTICLES, g_Sim->m_Particles);
    g_IsoVolume = new fp_IsoVolume(g_Sim);
    g_RenderIsoVolume = new fp_RenderIsoVolume(g_IsoVolume, 3);
    g_RenderIsoVolume->m_NumActiveLights = g_NumActiveLights;
    D3DCOLORVALUE diffuse  = { g_GUI.m_LightDiffuseColor->r,
            g_GUI.m_LightDiffuseColor->g, g_GUI.m_LightDiffuseColor->b, 1.0f };
    D3DCOLORVALUE ambient  = { 0.05f, 0.05f, 0.05f, 1.0f };
    D3DCOLORVALUE specular = { 0.8f, 0.8f, 0.8f, 1.0f };
    ZeroMemory( g_RenderIsoVolume->m_Lights, sizeof(D3DLIGHT9) * FP_MAX_LIGHTS );
    for (int i=0; i < FP_MAX_LIGHTS; i++) {
        g_RenderIsoVolume->m_Lights[i].Type = D3DLIGHT_DIRECTIONAL;
        g_RenderIsoVolume->m_Lights[i].Direction = g_GUI.m_LightDir[i];
        g_RenderIsoVolume->m_Lights[i].Diffuse = diffuse;
        g_RenderIsoVolume->m_Lights[i].Ambient = ambient;
        g_RenderIsoVolume->m_Lights[i].Specular = specular;
    }
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void FP_FinishApp() {
    delete g_Sim;
    g_Sim = NULL;
    delete g_RenderSprites;
    g_RenderSprites = NULL;
    delete g_IsoVolume;
    g_IsoVolume = NULL;
    delete g_RenderIsoVolume;
    g_RenderIsoVolume = NULL;
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the
// device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK FP_ModifyDeviceSettings( 
        DXUTDeviceSettings* DeviceSettings, 
        void* UserContext ) {
    if( DXUT_D3D9_DEVICE == DeviceSettings->ver ) {
        D3DCAPS9 Caps;
        IDirect3D9 *pD3D = DXUTGetD3D9Object();
        pD3D->GetDeviceCaps( DeviceSettings->d3d9.AdapterOrdinal,
                DeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION(1,1) ) {
            DeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
        #ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
            pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
        #endif
        #ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
        #endif
    }
    else {
        // Uncomment this to get debug information from D3D10
        //pDeviceSettings->d3d10.CreateFlags |= D3D10_CREATE_DEVICE_DEBUG;
    }

    // For the first device created if its a REF device, optionally display a warning
    // dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime ) {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == DeviceSettings->ver && DeviceSettings->d3d9.DeviceType
                == D3DDEVTYPE_REF) || (DXUT_D3D10_DEVICE == DeviceSettings->ver
                && DeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
                DXUTDisplaySwitchingToREFWarning( DeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnFrameMoveInitial(
        double Time, 
        float ElapsedTime, 
        void* UserContext ) {
    // Update the camera's position based on user input 
    g_Camera.FrameMove( ElapsedTime );
    if(Time > 0.1)
        DXUTSetCallbackFrameMove( FP_OnFrameMove );
}

void CALLBACK FP_OnFrameMove( double Time, float ElapsedTime, void* UserContext ) {
    // Update the camera's position based on user input 
    g_Camera.FrameMove( ElapsedTime );
    float mouseDragX, mouseDragY;
    g_Camera.GetMouseDrag(mouseDragX, mouseDragY);
    g_Sim->m_GlassPosition.x += 100.0f * mouseDragX;
    if(g_MoveHorizontally)
        g_Sim->m_GlassPosition.z -= 100.0f * mouseDragY;
    else
        g_Sim->m_GlassPosition.y -= 100.0f * mouseDragY;        
    g_Sim->Update(ElapsedTime);
    if(g_RenderType == FP_GUI_RENDER_TYPE_ISO_SURFACE) {
        g_IsoVolume->ConstructFromFluid();
        g_RenderIsoVolume->ConstructMesh();
    }
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK FP_MsgProc(
        HWND hWnd, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam, 
        bool* NoFurtherProcessing, 
        void* UserContext ) {
    // Pass messages to GUI instance
    if(g_GUI.MsgProc(hWnd, uMsg, wParam, lParam, NoFurtherProcessing, g_ActiveLight))
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnKeyboard( UINT Char, bool KeyDown, bool AltDown, void* UserContext ){
    if( KeyDown ) {
        switch( Char ) {
            case VK_F1: g_GUI.m_ShowHelp = !g_GUI.m_ShowHelp; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnGUIEvent(
        UINT Event, 
        int ControlID, 
        CDXUTControl* Control, 
        void* UserContext ) {   
    bool resetSim;
    float mcVoxelSize = -1.0f, mcIsoLevel = -1.0f;
    float oldSpriteSize = g_RenderSprites->GetSpriteSize();
    float newSpriteSize = oldSpriteSize;
    g_GUI.OnGUIEvent(Event, ControlID, Control, g_ActiveLight, g_NumActiveLights,
            mcVoxelSize, mcIsoLevel, g_LightScale, newSpriteSize, resetSim,
            g_MoveHorizontally, g_RenderType);
    if(oldSpriteSize != newSpriteSize)
        g_RenderSprites->SetSpriteSize(newSpriteSize);
    if(mcIsoLevel > 0.0f)
        g_RenderIsoVolume->m_IsoLevel = mcIsoLevel;
    if(mcVoxelSize > 0.0f && mcVoxelSize != g_IsoVolume->m_VoxelSize)
        g_IsoVolume->SetVoxelSize(mcVoxelSize);
    g_RenderIsoVolume->m_NumActiveLights = g_NumActiveLights;
    if(resetSim) {
		mcVoxelSize = g_IsoVolume->m_VoxelSize;
        delete g_Sim;
        delete g_IsoVolume;
        D3DXVECTOR3 center(0.0f, 0.0f, 0.0f);
        g_Sim = new fp_Fluid(&g_WorkerThreadMgr, FP_NUM_PARTICLES_X, FP_NUM_PARTICLES_Y,
            FP_NUM_PARTICLES_Z, FP_PARTICLE_SPACING_X, FP_PARTICLE_SPACING_Y,
            FP_PARTICLE_SPACING_Z, center, FP_GLASS_RADIUS, FP_GLASS_FLOOR);
        g_IsoVolume = new fp_IsoVolume(g_Sim, mcVoxelSize);
        g_RenderSprites->m_Particles = g_Sim->m_Particles;
        g_RenderIsoVolume->m_IsoVolume = g_IsoVolume;
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK FP_IsD3D10DeviceAcceptable(
        UINT Adapter,
        UINT Output, 
        D3D10_DRIVER_TYPE DeviceType,
        DXGI_FORMAT BackBufferFormat,
        bool Windowed,
        void* UserContext ) {
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependent on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D10CreateDevice(
        ID3D10Device* d3dDevice, 
        const DXGI_SURFACE_DESC *BackBufferSurfaceDesc, 
        void* UserContext ) {
    HRESULT hr;

    g_GUI.OnD3D10CreateDevice(d3dDevice, BackBufferSurfaceDesc, UserContext);

    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif


    g_RenderSprites->OnD3D10CreateDevice(d3dDevice, BackBufferSurfaceDesc,
            UserContext);
    g_RenderIsoVolume->OnD3D10CreateDevice(d3dDevice, BackBufferSurfaceDesc,
            UserContext);

    //D3DXVECTOR3 vCenter( 0.25767413f, -28.503521f, 111.00689f );

    //D3DXMatrixTranslation( &g_CenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );
    //D3DXMATRIXA16 m;
    //D3DXMatrixRotationY( &m, D3DX_PI );
    //g_CenterMesh *= m;
    //D3DXMatrixRotationX( &m, D3DX_PI / 2.0f );
    //g_CenterMesh *= m;

    //DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
    //#if defined( DEBUG ) || defined( _DEBUG )
    //// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    //// Setting this flag improves the shader debugging experience, but still allows 
    //// the shaders to be optimized and to run exactly the way they will run in 
    //// the release configuration of this program.
    //dwShaderFlags |= D3D10_SHADER_DEBUG;
    //#endif

    //// Read the D3DX effect file
    //WCHAR str[MAX_PATH];
    //V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"fp_effect10.fx" ) );
    //V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", dwShaderFlags, 
    //        0, d3dDevice, NULL, NULL, &g_Effect10, NULL, NULL ) );

    //// Obtain technique objects
    //g_TechRenderSceneWithTexture1Light = g_Effect10->GetTechniqueByName(
    //        "RenderSceneWithTexture1Light" );
    //g_TechRenderSceneWithTexture2Light = g_Effect10->GetTechniqueByName( 
    //        "RenderSceneWithTexture2Light" );
    //g_TechRenderSceneWithTexture3Light = g_Effect10->GetTechniqueByName( 
    //        "RenderSceneWithTexture3Light" );

    //// Obtain variables
    //g_ptxDiffuse = g_Effect10->GetVariableByName( "g_MeshTexture" )->AsShaderResource();
    //g_LightDir = g_Effect10->GetVariableByName( "g_LightDir" )->AsVector();
    //g_LightDiffuse = g_Effect10->GetVariableByName( "g_LightDiffuse" )->AsVector();
    //g_WorldViewProjection = g_Effect10->GetVariableByName( "g_mWorldViewProjection" )
    //        ->AsMatrix();
    //g_World = g_Effect10->GetVariableByName( "g_mWorld" )->AsMatrix();
    //g_Time = g_Effect10->GetVariableByName( "g_fTime" )->AsScalar();
    //g_MaterialAmbientColor = g_Effect10->GetVariableByName("g_MaterialAmbientColor")
    //        ->AsVector();
    //g_MaterialDiffuseColor = g_Effect10->GetVariableByName( "g_MaterialDiffuseColor" )
    //        ->AsVector();
    //g_NumLights = g_Effect10->GetVariableByName( "g_NumLights" )->AsScalar();

    //// Create our vertex input layout
    //const D3D10_INPUT_ELEMENT_DESC layout[] = {
    //    { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //    { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, 
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //    { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, 
    //            D3D10_INPUT_PER_VERTEX_DATA, 0 },
    //};

    //D3D10_PASS_DESC PassDesc;
    //V_RETURN( g_TechRenderSceneWithTexture1Light->GetPassByIndex( 0 )->GetDesc(
    //        &PassDesc ) );
    //V_RETURN( d3dDevice->CreateInputLayout( layout, 3, PassDesc.pIAInputSignature, 
    //        PassDesc.IAInputSignatureSize, &g_VertexLayout ) );

    //// Load the mesh
    //V_RETURN( g_Mesh10.Create( d3dDevice, L"tiny\\tiny.sdkmesh", true ) );

    //// Set effect variables as needed
    //D3DXCOLOR colorMtrlDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    //D3DXCOLOR colorMtrlAmbient(0.35f, 0.35f, 0.35f, 0);
    //V_RETURN( g_MaterialAmbientColor->SetFloatVector( (float*)&colorMtrlAmbient ) );
    //V_RETURN( g_MaterialDiffuseColor->SetFloatVector( (float*)&colorMtrlDiffuse ) );


    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -15.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( FP_OBJECT_RADIUS*3.0f, FP_OBJECT_RADIUS*0.5f, FP_OBJECT_RADIUS*100.0f );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D10ResizedSwapChain( 
        ID3D10Device* d3dDevice, 
        IDXGISwapChain *SwapChain, 
        const DXGI_SURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext ) {
    g_GUI.OnD3D10ResizedSwapChain(d3dDevice, SwapChain, BackBufferSurfaceDesc,
            UserContext);
    g_RenderSprites->OnD3D10ResizedSwapChain(d3dDevice, SwapChain, BackBufferSurfaceDesc,
            UserContext);
    g_RenderIsoVolume->OnD3D10ResizedSwapChain(d3dDevice, SwapChain, BackBufferSurfaceDesc,
            UserContext);

    // Setup the camera's projection parameters
    float fAspectRatio = BackBufferSurfaceDesc->Width / (float)BackBufferSurfaceDesc
            ->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 2.0f, 4000.0f );
    g_Camera.SetWindow( BackBufferSurfaceDesc->Width, BackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D10FrameRender( 
        ID3D10Device* d3dDevice, 
        double Time, 
        float ElapsedTime, 
        void* UserContext ) {
    HRESULT hr;    

    // If the settings dialog is being shown, then render it instead of rendering the
    // app's scene
    if( g_GUI.RenderSettingsDialog(ElapsedTime) )
        return;   

    // Clear the render target and depth stencil
    float ClearColor[4] = { FP_CLEAR_COLOR };
    ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
    d3dDevice->ClearRenderTargetView( pRTV, ClearColor );
    ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
    d3dDevice->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );
    
    D3DXMATRIX  world;
    D3DXMATRIX  view;
    D3DXMATRIX  projection;
	D3DXMATRIX  worldViewProjection;
	D3DXMATRIX  viewProjection;
	D3DXMATRIX  invView;

    // Get the projection & view matrix from the camera class
    world = *g_Camera.GetWorldMatrix();
    view = *g_Camera.GetViewMatrix();
    projection = *g_Camera.GetProjMatrix();
    viewProjection = view * projection;
	worldViewProjection = world * viewProjection;
	D3DXMatrixInverse(&invView, NULL, &view);

    for (int i=0; i < FP_MAX_LIGHTS; i++) {
        g_RenderIsoVolume->m_Lights[i].Direction = g_GUI.m_LightDir[i];
        D3DCOLORVALUE diffuse = { g_GUI.m_LightDiffuseColor->r,
            g_GUI.m_LightDiffuseColor->g, g_GUI.m_LightDiffuseColor->b, 1.0f };
        g_RenderIsoVolume->m_Lights[i].Diffuse = diffuse;
    }

    if(g_RenderType == FP_GUI_RENDER_TYPE_POINT_SPRITE)
        g_RenderSprites->OnD3D10FrameRender(d3dDevice, &viewProjection, &invView);
    else if(g_RenderType == FP_GUI_RENDER_TYPE_ISO_SURFACE)
        g_RenderIsoVolume->OnD3D10FrameRender(d3dDevice, Time, ElapsedTime,
                g_Camera.GetEyePt(), &worldViewProjection, &world, &view, &projection,
                g_NumActiveLights, g_ActiveLight, g_LightScale);

    // Render GUI
    g_GUI.OnD3D10FrameRender(d3dDevice, ElapsedTime, g_Camera.GetEyePt(),
            &view, &projection, g_NumActiveLights,
            g_ActiveLight, g_LightScale);
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D10ReleasingSwapChain( void* UserContext ) {
    g_GUI.OnD3D10ReleasingSwapChain(UserContext);
    if(g_RenderSprites) g_RenderSprites->OnD3D10ReleasingSwapChain(UserContext);
    if(g_RenderIsoVolume) g_RenderIsoVolume->OnD3D10ReleasingSwapChain(UserContext);
    //if(g_Effect10) g_Effect10->OnD3D10ReleasingSwapChain(UserContext);  
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D10DestroyDevice( void* UserContext ) {
    g_GUI.OnD3D10DestroyDevice(UserContext);
    if(g_RenderSprites) g_RenderSprites->OnD3D10DestroyDevice(UserContext);
    if(g_RenderIsoVolume) g_RenderIsoVolume->OnD3D10DestroyDevice(UserContext);
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    //SAFE_RELEASE( g_Effect10 );
    //SAFE_RELEASE( g_VertexLayout );
    //g_Mesh10.Destroy();
}
