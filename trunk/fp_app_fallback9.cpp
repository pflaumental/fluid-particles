#define D3DXFX_LARGEADDRESS_HANDLE
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"

#include "fp_gui.h"
#include "fp_global.h"
#include "fp_cpu_sph.h"
#include "fp_render_sprites.h"
#include "fp_render_iso_volume.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 

#define IDC_NUM_LIGHTS          6
#define IDC_NUM_LIGHTS_STATIC   7
#define IDC_ACTIVE_LIGHT        8

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern CModelViewerCamera      g_Camera;               // A model viewing camera
extern D3DXMATRIXA16           g_CenterMesh;

extern fp_GUI                  g_GUI;
extern fp_RenderSprites*       g_RenderSprites;
extern fp_RenderIsoVolume*     g_RenderIsoVolume;

extern float                   g_LightScale;
extern int                     g_NumActiveLights;
extern int                     g_ActiveLight;
extern int                     g_RenderType;

extern D3DXVECTOR3*            g_Particle;
extern int                     g_Paticles;

extern fp_Fluid*               g_Sim;

// Direct3D9 resources
ID3DXEffect*            g_Effect9 = NULL;       // D3DX effect interface
ID3DXMesh*              g_Mesh9 = NULL;         // Mesh object
IDirect3DTexture9*      g_MeshTexture9 = NULL;  // Mesh texture

D3DXHANDLE g_LightDir;
D3DXHANDLE g_LightDiffuse;
D3DXHANDLE g_WorldViewProjection;
D3DXHANDLE g_World;
D3DXHANDLE g_MaterialDiffuseColor;
D3DXHANDLE g_Time;
D3DXHANDLE g_NumLights;
D3DXHANDLE g_RenderSceneWithTexture1Light;
D3DXHANDLE g_RenderSceneWithTexture2Light;
D3DXHANDLE g_RenderSceneWithTexture3Light;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK FP_IsD3D9DeviceAcceptable(
        D3DCAPS9* Caps, 
        D3DFORMAT AdapterFormat, 
        D3DFORMAT BackBufferFormat, 
        bool Windowed, 
        void* UserContext);
HRESULT CALLBACK FP_OnD3D9CreateDevice( 
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext);
HRESULT CALLBACK FP_OnD3D9ResetDevice(
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc, 
        void* UserContext);
void    CALLBACK FP_OnD3D9FrameRender(
        IDirect3DDevice9* d3dDevice,
        double Time,
        float ElapsedTime,
        void* UserContext );
void    CALLBACK FP_OnD3D9LostDevice( void* UserContext );
void    CALLBACK FP_OnD3D9DestroyDevice( void* UserContext );
HRESULT FP_LoadMesh( IDirect3DDevice9* d3dDevice, WCHAR* FileName, ID3DXMesh** Mesh );


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK FP_IsD3D9DeviceAcceptable(
        D3DCAPS9* Caps,
        D3DFORMAT AdapterFormat, 
        D3DFORMAT BackBufferFormat,
        bool Windowed,
        void* UserContext ) {
    // No fallback defined by this app, so reject any device that doesn't support at
    //least ps2.0
    if( Caps->PixelShaderVersion < D3DPS_VERSION(2,0) )
        return false;

    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object(); 
    if( FAILED( pD3D->CheckDeviceFormat( Caps->AdapterOrdinal, Caps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D9CreateDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext) {    
    //HRESULT hr;

    g_GUI.OnD3D9CreateDevice(d3dDevice, BackBufferSurfaceDesc, UserContext);
    g_RenderSprites->OnCreateDevice(d3dDevice, BackBufferSurfaceDesc);
    g_RenderIsoVolume->OnCreateDevice(d3dDevice, BackBufferSurfaceDesc);

    //// Load the mesh
    //V_RETURN( FP_LoadMesh( d3dDevice, L"tiny\\tiny.x", &g_Mesh9 ) );

    //D3DXVECTOR3* pData; 
    //D3DXVECTOR3 vCenter;
    //V( g_Mesh9->LockVertexBuffer( 0, (LPVOID*) &pData ) );
    //float fDummy;
    //V( D3DXComputeBoundingSphere( pData, g_Mesh9->GetNumVertices(), D3DXGetFVFVertexSize( g_Mesh9->GetFVF() ), &vCenter, &fDummy ) );
    //V( g_Mesh9->UnlockVertexBuffer() );

    //D3DXMatrixTranslation( &g_CenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );
    //D3DXMATRIXA16 m;
    //D3DXMatrixRotationY( &m, D3DX_PI );
    //g_CenterMesh *= m;
    //D3DXMatrixRotationX( &m, D3DX_PI / 2.0f );
    //g_CenterMesh *= m;

    //// Read the D3DX effect file
    //WCHAR str[MAX_PATH];
    //DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE | D3DXSHADER_NO_PRESHADER | D3DXFX_LARGEADDRESSAWARE;
    //#ifdef DEBUG_VS
    //    dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    //#endif
    //#ifdef DEBUG_PS
    //    dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    //#endif
    //V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"fp_effect9.fx" ) );
    //V_RETURN( D3DXCreateEffectFromFile( d3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &g_Effect9, NULL ) );

    //// Create the mesh texture from a file
    //V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"tiny\\tiny_skin.dds" ) );

    //V_RETURN( D3DXCreateTextureFromFileEx( d3dDevice, str, D3DX_DEFAULT, D3DX_DEFAULT, 
    //                                       D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, 
    //                                       D3DX_DEFAULT, D3DX_DEFAULT, 0, 
    //                                       NULL, NULL, &g_MeshTexture9 ) );

    //// Set effect variables as needed
    //D3DXCOLOR colorMtrlDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    //D3DXCOLOR colorMtrlAmbient(0.35f, 0.35f, 0.35f, 0);

    //D3DXHANDLE hMaterialAmbientColor = g_Effect9->GetParameterByName(NULL,"g_MaterialAmbientColor");
    //D3DXHANDLE hMaterialDiffuseColor = g_Effect9->GetParameterByName(NULL,"g_MaterialDiffuseColor");
    //D3DXHANDLE hMeshTexture = g_Effect9->GetParameterByName(NULL,"g_MeshTexture");

    //V_RETURN( g_Effect9->SetValue( hMaterialAmbientColor, &colorMtrlAmbient, sizeof(D3DXCOLOR) ) );
    //V_RETURN( g_Effect9->SetValue( hMaterialDiffuseColor, &colorMtrlDiffuse, sizeof(D3DXCOLOR) ) );    
    //V_RETURN( g_Effect9->SetTexture( hMeshTexture, g_MeshTexture9) );

    //g_LightDir = g_Effect9->GetParameterByName(NULL,"g_LightDir");
    //g_LightDiffuse = g_Effect9->GetParameterByName(NULL,"g_LightDiffuse");
    //g_WorldViewProjection = g_Effect9->GetParameterByName(NULL,"g_WorldViewProjection");
    //g_World = g_Effect9->GetParameterByName(NULL,"g_World");
    //g_MaterialDiffuseColor = g_Effect9->GetParameterByName(NULL,"g_MaterialDiffuseColor");
    //g_Time = g_Effect9->GetParameterByName(NULL,"g_Time");
    //g_NumLights = g_Effect9->GetParameterByName(NULL,"g_NumLights");
    //g_RenderSceneWithTexture1Light = g_Effect9->GetTechniqueByName("RenderSceneWithTexture1Light");
    //g_RenderSceneWithTexture2Light = g_Effect9->GetTechniqueByName("RenderSceneWithTexture2Light");
    //g_RenderSceneWithTexture3Light = g_Effect9->GetTechniqueByName("RenderSceneWithTexture3Light");

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -15.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( FP_OBJECT_RADIUS*3.0f, FP_OBJECT_RADIUS*0.5f, FP_OBJECT_RADIUS*100.0f );
    g_Camera.SetGlassPosition(&g_Sim->m_GlassPosition);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This function loads the mesh and ensures the mesh has normals; it also optimizes the 
// mesh for the graphics card's vertex cache, which improves performance by organizing 
// the internal triangle list for less cache misses.
//--------------------------------------------------------------------------------------
HRESULT FP_LoadMesh( IDirect3DDevice9* d3dDevice, WCHAR* FileName, ID3DXMesh** Mesh ) {
    ID3DXMesh* pMesh = NULL;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
    // sample we'll ignore the X file's embedded materials since we know 
    // exactly the model we're loading.  See the mesh samples such as
    // "OptimizedMesh" for a more generic mesh loading example.
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, FileName ) );
    V_RETURN( D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, d3dDevice, NULL, NULL, NULL,
            NULL, &pMesh) );

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        ID3DXMesh* pTempMesh;
        V( pMesh->CloneMeshFVF( pMesh->GetOptions(), pMesh->GetFVF()
                | D3DFVF_NORMAL, d3dDevice, &pTempMesh ) );
        V( D3DXComputeNormals( pTempMesh, NULL ) );

        SAFE_RELEASE( pMesh );
        pMesh = pTempMesh;
    }

    // Optimize the mesh for this graphics card's vertex cache 
    // so when rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader 
    // on those vertices so it will improve perf.     
    rgdwAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( rgdwAdjacency == NULL )
        return E_OUTOFMEMORY;
    V( pMesh->GenerateAdjacency(1e-6f,rgdwAdjacency) );
    V( pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, 
            NULL, NULL) );
    delete []rgdwAdjacency;

    *Mesh = pMesh;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D9ResetDevice(
        IDirect3DDevice9* d3dDevice, 
        const D3DSURFACE_DESC* BackBufferSurfaceDesc,
        void* UserContext ) {
    HRESULT hr;
    //initPointSprites(d3dDevice);
    g_GUI.OnD3D9ResetDevice(d3dDevice, BackBufferSurfaceDesc, UserContext);
    g_RenderSprites->OnResetDevice(d3dDevice, BackBufferSurfaceDesc);
    g_RenderIsoVolume->OnResetDevice(d3dDevice, BackBufferSurfaceDesc);

    if( g_Effect9 ) V_RETURN( g_Effect9->OnResetDevice() );
    // Setup the camera's projection parameters
    float fAspectRatio = BackBufferSurfaceDesc->Width / (float)BackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 2.0f, 4000.0f );
    g_Camera.SetWindow( BackBufferSurfaceDesc->Width, BackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );    

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9FrameRender(
        IDirect3DDevice9* d3dDevice, 
        double Time, 
        float ElapsedTime, 
        void* UserContext) {
    // If the settings dialog is being shown, then render it instead of rendering the
    //app's scene
    if( g_GUI.RenderSettingsDialog(ElapsedTime) )
        return;    

    HRESULT hr;
    D3DXMATRIXA16 WorldViewProjection;
    //UINT iPass, cPasses;
    D3DXMATRIXA16 World;
    D3DXMATRIXA16 View;
    D3DXMATRIXA16 Proj;
   
    // Clear the render target and the zbuffer 
    V( d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR(0.0f,
            0.05f,0.15f,0.55f), 1.0f, 0) );

    // Render the scene
    if( SUCCEEDED( d3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        World = /*g_CenterMesh * */*g_Camera.GetWorldMatrix();
        Proj = *g_Camera.GetProjMatrix();
        View = *g_Camera.GetViewMatrix();

        WorldViewProjection = World * View * Proj;

        for (int i=0; i < FP_MAX_LIGHTS; i++) {
            g_RenderIsoVolume->m_Lights[i].Direction = g_GUI.m_LightDir[i];
            D3DCOLORVALUE diffuse = { g_GUI.m_LightDiffuseColor->r,
                g_GUI.m_LightDiffuseColor->g, g_GUI.m_LightDiffuseColor->b, 1.0f };
            g_RenderIsoVolume->m_Lights[i].Diffuse = diffuse;
        }

        //V( g_Effect9->SetValue( g_LightDir, g_GUI.m_LightDir, sizeof(D3DXVECTOR3)*FP_MAX_LIGHTS ) );
        //V( g_Effect9->SetValue( g_LightDiffuse, g_GUI.m_cLightDiffuse, sizeof(D3DXCOLOR)*FP_MAX_LIGHTS ) );

        //// Update the effect's variables.  Instead of using strings, it would 
        //// be more efficient to cache a handle to the parameter by calling 
        //// ID3DXEffect::GetParameterByName
        //V( g_Effect9->SetMatrix( g_WorldViewProjection, &WorldViewProjection ) );
        //V( g_Effect9->SetMatrix( g_World, &World ) );

        //D3DXCOLOR vWhite = D3DXCOLOR(1,1,1,1);
        //V( g_Effect9->SetValue(g_MaterialDiffuseColor, &vWhite, sizeof(D3DXCOLOR) ) );
        //V( g_Effect9->SetFloat( g_Time, (float)fTime ) );      
        //V( g_Effect9->SetInt( g_NumLights, g_NumActiveLights ) );

        //// Render the scene with this technique as defined in the .fx file
        //switch( g_NumActiveLights )
        //{
        //    case 1: V( g_Effect9->SetTechnique( g_RenderSceneWithTexture1Light ) ); break;
        //    case 2: V( g_Effect9->SetTechnique( g_RenderSceneWithTexture2Light ) ); break;
        //    case 3: V( g_Effect9->SetTechnique( g_RenderSceneWithTexture3Light ) ); break;
        //}

        //// Apply the technique contained in the effect and render the mesh
        //V( g_Effect9->Begin(&cPasses, 0) );
        //for (iPass = 0; iPass < cPasses; iPass++)
        //{
        //    V( g_Effect9->BeginPass(iPass) );
        //    V( g_Mesh9->DrawSubset(0) );
        //    V( g_Effect9->EndPass() );
        //}
        //V( g_Effect9->End() );

        d3dDevice->SetTransform( D3DTS_WORLD, &World );
        d3dDevice->SetTransform( D3DTS_PROJECTION, &Proj );
        d3dDevice->SetTransform( D3DTS_VIEW, &View );        
       
        if(g_RenderType == FP_GUI_RENDER_TYPE_POINT_SPRITE)
            g_RenderSprites->OnFrameRender(d3dDevice, Time, ElapsedTime);
        else if(g_RenderType == FP_GUI_RENDER_TYPE_ISO_SURFACE)
            g_RenderIsoVolume->OnFrameRender(d3dDevice, Time, ElapsedTime);

        g_GUI.OnD3D9FrameRender(d3dDevice, Time, ElapsedTime, g_Camera.GetEyePt(),
                &WorldViewProjection, &World, &View, &Proj, g_NumActiveLights,
                g_ActiveLight, g_LightScale);



        V( d3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9LostDevice( void* UserContext ) {
    g_GUI.OnD3D9LostDevice(UserContext);
    if(g_RenderSprites) g_RenderSprites->OnLostDevice();
    if(g_RenderIsoVolume) g_RenderIsoVolume->OnLostDevice();
    if(g_Effect9) g_Effect9->OnLostDevice();    
    //shutDownPointSprites();    
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9DestroyDevice( void* UserContext )
{
    g_GUI.OnD3D9DestroyDevice(UserContext);
    if(g_RenderSprites) g_RenderSprites->OnDetroyDevice();
    if(g_RenderIsoVolume) g_RenderIsoVolume->OnDetroyDevice();
    SAFE_RELEASE(g_Effect9);
    SAFE_RELEASE(g_Mesh9);
    SAFE_RELEASE(g_MeshTexture9);
}
