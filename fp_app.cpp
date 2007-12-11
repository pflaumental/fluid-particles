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

D3DXMATRIXA16           g_CenterMesh;
float                   g_LightScale;
int                     g_NumActiveLights;
int                     g_ActiveLight;
int                     g_RenderType;

// Direct3D10 resources
CDXUTSDKMesh            g_Mesh10;
ID3D10InputLayout*      g_VertexLayout = NULL;

ID3D10Effect*           g_Effect10 = NULL;
ID3D10EffectTechnique*  g_TechRenderSceneWithTexture1Light = NULL;
ID3D10EffectTechnique*  g_TechRenderSceneWithTexture2Light = NULL;
ID3D10EffectTechnique*  g_TechRenderSceneWithTexture3Light = NULL;
ID3D10EffectShaderResourceVariable* g_ptxDiffuse = NULL;
ID3D10EffectVectorVariable* g_LightDir = NULL;
ID3D10EffectVectorVariable* g_LightDiffuse = NULL;
ID3D10EffectMatrixVariable* g_WorldViewProjection = NULL;
ID3D10EffectMatrixVariable* g_World = NULL;
ID3D10EffectScalarVariable* g_Time = NULL;
ID3D10EffectVectorVariable* g_MaterialDiffuseColor = NULL;
ID3D10EffectVectorVariable* g_MaterialAmbientColor = NULL;
ID3D10EffectScalarVariable* g_NumLights = NULL;


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
    g_Sim = new fp_Fluid(FP_NUM_PARTICLES_X, FP_NUM_PARTICLES_Y, FP_NUM_PARTICLES_Z,
            FP_PARTICLE_SPACING_X, FP_PARTICLE_SPACING_Y, FP_PARTICLE_SPACING_Z, center,
			FP_GLASS_RADIUS, FP_GLASS_FLOOR);
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
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK FP_ModifyDeviceSettings( 
        DXUTDeviceSettings* DeviceSettings, 
        void* UserContext ) {
    if( DXUT_D3D9_DEVICE == DeviceSettings->ver ) {
        D3DCAPS9 Caps;
        IDirect3D9 *pD3D = DXUTGetD3D9Object();
        pD3D->GetDeviceCaps( DeviceSettings->d3d9.AdapterOrdinal, DeviceSettings->d3d9.DeviceType, &Caps );

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

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime ) {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == DeviceSettings->ver && DeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D10_DEVICE == DeviceSettings->ver && DeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
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
    if(Time > 0.2)
        DXUTSetCallbackFrameMove( FP_OnFrameMove );
}

void CALLBACK FP_OnFrameMove( double Time, float ElapsedTime, void* UserContext ) {
    // Update the camera's position based on user input 
    g_Camera.FrameMove( ElapsedTime );
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
void CALLBACK FP_OnKeyboard( UINT Char, bool KeyDown, bool AltDown, void* UserContext ) {
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
    g_GUI.OnGUIEvent(Event, ControlID, Control, g_ActiveLight, g_NumActiveLights,
            mcVoxelSize, mcIsoLevel, g_LightScale, g_RenderSprites->m_SpriteSize,
            resetSim, g_RenderType);
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
        g_Sim = new fp_Fluid(FP_NUM_PARTICLES_X, FP_NUM_PARTICLES_Y, FP_NUM_PARTICLES_Z,
            FP_PARTICLE_SPACING_X, FP_PARTICLE_SPACING_Y, FP_PARTICLE_SPACING_Z, center,
			FP_GLASS_RADIUS, FP_GLASS_FLOOR);
        g_IsoVolume = new fp_IsoVolume(g_Sim, mcVoxelSize);
        g_RenderSprites->m_Particles = g_Sim->m_Particles;
        g_RenderIsoVolume->m_IsoVolume = g_IsoVolume;
        g_Camera.SetGlassPosition(&g_Sim->m_GlassPosition);
    }
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK FP_IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool Windowed, void* UserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D10CreateDevice(
        ID3D10Device* d3dDevice, 
        const DXGI_SURFACE_DESC *BackBufferSurfaceDesc, 
        void* UserContext ) {
    HRESULT hr;

    g_GUI.OnD3D10CreateDevice(d3dDevice, BackBufferSurfaceDesc, UserContext);

    D3DXVECTOR3 vCenter( 0.25767413f, -28.503521f, 111.00689f );

    D3DXMatrixTranslation( &g_CenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );
    D3DXMATRIXA16 m;
    D3DXMatrixRotationY( &m, D3DX_PI );
    g_CenterMesh *= m;
    D3DXMatrixRotationX( &m, D3DX_PI / 2.0f );
    g_CenterMesh *= m;

    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"fp_effect10.fx" ) );
    V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", dwShaderFlags, 
            0, d3dDevice, NULL, NULL, &g_Effect10, NULL, NULL ) );

    // Obtain technique objects
    g_TechRenderSceneWithTexture1Light = g_Effect10->GetTechniqueByName(
            "RenderSceneWithTexture1Light" );
    g_TechRenderSceneWithTexture2Light = g_Effect10->GetTechniqueByName( 
            "RenderSceneWithTexture2Light" );
    g_TechRenderSceneWithTexture3Light = g_Effect10->GetTechniqueByName( 
            "RenderSceneWithTexture3Light" );

    // Obtain variables
    g_ptxDiffuse = g_Effect10->GetVariableByName( "g_MeshTexture" )->AsShaderResource();
    g_LightDir = g_Effect10->GetVariableByName( "g_LightDir" )->AsVector();
    g_LightDiffuse = g_Effect10->GetVariableByName( "g_LightDiffuse" )->AsVector();
    g_WorldViewProjection = g_Effect10->GetVariableByName( "g_mWorldViewProjection" )
            ->AsMatrix();
    g_World = g_Effect10->GetVariableByName( "g_mWorld" )->AsMatrix();
    g_Time = g_Effect10->GetVariableByName( "g_fTime" )->AsScalar();
    g_MaterialAmbientColor = g_Effect10->GetVariableByName("g_MaterialAmbientColor")
            ->AsVector();
    g_MaterialDiffuseColor = g_Effect10->GetVariableByName( "g_MaterialDiffuseColor" )
            ->AsVector();
    g_NumLights = g_Effect10->GetVariableByName( "g_NumLights" )->AsScalar();

    // Create our vertex input layout
    const D3D10_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  
                D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, 
                D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, 
                D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3D10_PASS_DESC PassDesc;
    V_RETURN( g_TechRenderSceneWithTexture1Light->GetPassByIndex( 0 )->GetDesc(
            &PassDesc ) );
    V_RETURN( d3dDevice->CreateInputLayout( layout, 3, PassDesc.pIAInputSignature, 
            PassDesc.IAInputSignatureSize, &g_VertexLayout ) );

    // Load the mesh
    V_RETURN( g_Mesh10.Create( d3dDevice, L"tiny\\tiny.sdkmesh", true ) );

    // Set effect variables as needed
    D3DXCOLOR colorMtrlDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXCOLOR colorMtrlAmbient(0.35f, 0.35f, 0.35f, 0);
    V_RETURN( g_MaterialAmbientColor->SetFloatVector( (float*)&colorMtrlAmbient ) );
    V_RETURN( g_MaterialDiffuseColor->SetFloatVector( (float*)&colorMtrlDiffuse ) );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -15.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( FP_OBJECT_RADIUS*3.0f, FP_OBJECT_RADIUS*0.5f, FP_OBJECT_RADIUS*10.0f );  

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
    float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
    ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
    d3dDevice->ClearRenderTargetView( pRTV, ClearColor );
    ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
    d3dDevice->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );

    D3DXMATRIX  mWorldViewProjection;
    D3DXMATRIX  mWorld;
    D3DXMATRIX  mView;
    D3DXMATRIX  mProj;

    // Get the projection & view matrix from the camera class
    mWorld = g_CenterMesh * *g_Camera.GetWorldMatrix();
    mProj = *g_Camera.GetProjMatrix();
    mView = *g_Camera.GetViewMatrix();

    mWorldViewProjection = mWorld * mView * mProj;

    V( g_LightDir->SetRawValue( g_GUI.m_LightDir, 0, sizeof(D3DXVECTOR3)
            * FP_MAX_LIGHTS ) );
    V( g_LightDiffuse->SetFloatVectorArray( (float*)g_GUI.m_LightDiffuse, 0, 
            FP_MAX_LIGHTS ) );
    for (int i=0; i < FP_MAX_LIGHTS; i++) {
        g_RenderIsoVolume->m_Lights[i].Direction = g_GUI.m_LightDir[i];
        D3DCOLORVALUE diffuse = { g_GUI.m_LightDiffuseColor->r,
            g_GUI.m_LightDiffuseColor->g, g_GUI.m_LightDiffuseColor->b, 1.0f };
        g_RenderIsoVolume->m_Lights[i].Diffuse = diffuse;
    }
    V( g_WorldViewProjection->SetMatrix( (float*)&mWorldViewProjection ) );
    V( g_World->SetMatrix( (float*)&mWorld ) );
    V( g_Time->SetFloat( (float)Time ) );
    V( g_NumLights->SetInt( g_NumActiveLights ) );

    // Render the scene with this technique as defined in the .fx file
    ID3D10EffectTechnique *pRenderTechnique;
    switch( g_NumActiveLights )
    {
        case 1: pRenderTechnique = g_TechRenderSceneWithTexture1Light;
			break;
        case 2: pRenderTechnique = g_TechRenderSceneWithTexture2Light;
			break;
        case 3: pRenderTechnique = g_TechRenderSceneWithTexture3Light;
			break;
        default: pRenderTechnique = g_TechRenderSceneWithTexture1Light;
			break;
    }

    //Get the mesh
    //IA setup
    d3dDevice->IASetInputLayout( g_VertexLayout );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D10Buffer* pVB[1];
    pVB[0] = g_Mesh10.GetVB10(0,0);
    Strides[0] = (UINT)g_Mesh10.GetVertexStride(0,0);
    Offsets[0] = 0;
    d3dDevice->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    d3dDevice->IASetIndexBuffer( g_Mesh10.GetIB10(0), g_Mesh10.GetIBFormat10(0), 0 );

    //Render
    D3D10_TECHNIQUE_DESC techDesc;
    pRenderTechnique->GetDesc( &techDesc );
    SDKMESH_SUBSET* pSubset = NULL;
    ID3D10ShaderResourceView* pDiffuseRV = NULL;
    D3D10_PRIMITIVE_TOPOLOGY PrimType;

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        for( UINT subset = 0; subset < g_Mesh10.GetNumSubsets(0); ++subset )
        {
            // Get the subset
            pSubset = g_Mesh10.GetSubset( 0, subset );

            PrimType = CDXUTSDKMesh::GetPrimitiveType10( (SDKMESH_PRIMITIVE_TYPE)pSubset
                    ->PrimitiveType );
            d3dDevice->IASetPrimitiveTopology( PrimType );

            pDiffuseRV = g_Mesh10.GetMaterial(pSubset->MaterialID)->pDiffuseRV10;
            g_ptxDiffuse->SetResource( pDiffuseRV );

            pRenderTechnique->GetPassByIndex( p )->Apply(0);
            d3dDevice->DrawIndexed( (UINT)pSubset->IndexCount, 0, (UINT)pSubset
                    ->VertexStart );
        }
    }

    // Render GUI
    g_GUI.OnD3D10FrameRender(d3dDevice, Time, ElapsedTime, g_Camera.GetEyePt(),
            &mWorldViewProjection, &mWorld, &mView, &mProj, g_NumActiveLights,
            g_ActiveLight, g_LightScale);
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D10ReleasingSwapChain( void* UserContext ) {
    g_GUI.OnD3D10ReleasingSwapChain(UserContext);
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D10DestroyDevice( void* UserContext ) {
    g_GUI.OnD3D10DestroyDevice(UserContext);
    
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    SAFE_RELEASE( g_Effect10 );
    SAFE_RELEASE( g_VertexLayout );
    g_Mesh10.Destroy();
}
