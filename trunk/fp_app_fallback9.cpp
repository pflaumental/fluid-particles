#define D3DXFX_LARGEADDRESS_HANDLE
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"

#include "fp_gui.h"
#include "fp_global.h"
#include "fp_test_particle_sim.h"
#include "fp_render_sprites.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 
#define FP_MAX_LIGHTS 3

#define IDC_NUM_LIGHTS          6
#define IDC_NUM_LIGHTS_STATIC   7
#define IDC_ACTIVE_LIGHT        8

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern CModelViewerCamera      g_Camera;               // A model viewing camera
extern fp_GUI                  g_GUI;
extern D3DXMATRIXA16           g_mCenterMesh;

extern fp_RenderSprites* g_pRenderSprites;

extern float                   g_fLightScale;
extern int                     g_NumActiveLights;
extern int                     g_nActiveLight;

extern D3DXVECTOR3*            g_pvParticle;
extern int                     g_nPaticles;

// Direct3D9 resources
ID3DXEffect*            g_pEffect9 = NULL;       // D3DX effect interface
ID3DXMesh*              g_pMesh9 = NULL;         // Mesh object
IDirect3DTexture9*      g_pMeshTexture9 = NULL;  // Mesh texture

D3DXHANDLE g_hLightDir;
D3DXHANDLE g_hLightDiffuse;
D3DXHANDLE g_hmWorldViewProjection;
D3DXHANDLE g_hmWorld;
D3DXHANDLE g_hMaterialDiffuseColor;
D3DXHANDLE g_hfTime;
D3DXHANDLE g_hNumLights;
D3DXHANDLE g_hRenderSceneWithTexture1Light;
D3DXHANDLE g_hRenderSceneWithTexture2Light;
D3DXHANDLE g_hRenderSceneWithTexture3Light;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK FP_IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK FP_OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK FP_OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void    CALLBACK FP_OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void    CALLBACK FP_OnD3D9LostDevice( void* pUserContext );
void    CALLBACK FP_OnD3D9DestroyDevice( void* pUserContext );
HRESULT FP_LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh );


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK FP_IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // No fallback defined by this app, so reject any device that doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(2,0) )
        return false;

    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object(); 
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{    
    //HRESULT hr;

    g_GUI.OnD3D9CreateDevice(pd3dDevice, pBackBufferSurfaceDesc, pUserContext);
    g_pRenderSprites->OnCreateDevice(pd3dDevice, pBackBufferSurfaceDesc);

    //// Load the mesh
    //V_RETURN( FP_LoadMesh( pd3dDevice, L"tiny\\tiny.x", &g_pMesh9 ) );

    //D3DXVECTOR3* pData; 
    //D3DXVECTOR3 vCenter;
    //V( g_pMesh9->LockVertexBuffer( 0, (LPVOID*) &pData ) );
    //float fDummy;
    //V( D3DXComputeBoundingSphere( pData, g_pMesh9->GetNumVertices(), D3DXGetFVFVertexSize( g_pMesh9->GetFVF() ), &vCenter, &fDummy ) );
    //V( g_pMesh9->UnlockVertexBuffer() );

    //D3DXMatrixTranslation( &g_mCenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );
    //D3DXMATRIXA16 m;
    //D3DXMatrixRotationY( &m, D3DX_PI );
    //g_mCenterMesh *= m;
    //D3DXMatrixRotationX( &m, D3DX_PI / 2.0f );
    //g_mCenterMesh *= m;

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
    //V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &g_pEffect9, NULL ) );

    //// Create the mesh texture from a file
    //V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"tiny\\tiny_skin.dds" ) );

    //V_RETURN( D3DXCreateTextureFromFileEx( pd3dDevice, str, D3DX_DEFAULT, D3DX_DEFAULT, 
    //                                       D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, 
    //                                       D3DX_DEFAULT, D3DX_DEFAULT, 0, 
    //                                       NULL, NULL, &g_pMeshTexture9 ) );

    //// Set effect variables as needed
    //D3DXCOLOR colorMtrlDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    //D3DXCOLOR colorMtrlAmbient(0.35f, 0.35f, 0.35f, 0);

    //D3DXHANDLE hMaterialAmbientColor = g_pEffect9->GetParameterByName(NULL,"g_MaterialAmbientColor");
    //D3DXHANDLE hMaterialDiffuseColor = g_pEffect9->GetParameterByName(NULL,"g_MaterialDiffuseColor");
    //D3DXHANDLE hMeshTexture = g_pEffect9->GetParameterByName(NULL,"g_MeshTexture");

    //V_RETURN( g_pEffect9->SetValue( hMaterialAmbientColor, &colorMtrlAmbient, sizeof(D3DXCOLOR) ) );
    //V_RETURN( g_pEffect9->SetValue( hMaterialDiffuseColor, &colorMtrlDiffuse, sizeof(D3DXCOLOR) ) );    
    //V_RETURN( g_pEffect9->SetTexture( hMeshTexture, g_pMeshTexture9) );

    //g_hLightDir = g_pEffect9->GetParameterByName(NULL,"g_LightDir");
    //g_hLightDiffuse = g_pEffect9->GetParameterByName(NULL,"g_LightDiffuse");
    //g_hmWorldViewProjection = g_pEffect9->GetParameterByName(NULL,"g_mWorldViewProjection");
    //g_hmWorld = g_pEffect9->GetParameterByName(NULL,"g_mWorld");
    //g_hMaterialDiffuseColor = g_pEffect9->GetParameterByName(NULL,"g_MaterialDiffuseColor");
    //g_hfTime = g_pEffect9->GetParameterByName(NULL,"g_fTime");
    //g_hNumLights = g_pEffect9->GetParameterByName(NULL,"g_NumLights");
    //g_hRenderSceneWithTexture1Light = g_pEffect9->GetTechniqueByName("RenderSceneWithTexture1Light");
    //g_hRenderSceneWithTexture2Light = g_pEffect9->GetTechniqueByName("RenderSceneWithTexture2Light");
    //g_hRenderSceneWithTexture3Light = g_pEffect9->GetTechniqueByName("RenderSceneWithTexture3Light");

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -15.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    g_Camera.SetViewParams( &vecEye, &vecAt );
    //g_Camera.SetRadius( FP_OBJECT_RADIUS*3.0f, FP_OBJECT_RADIUS*0.5f, FP_OBJECT_RADIUS*10.0f );
    g_Camera.SetRadius(30.0f, 1.0f, 500.0f);    

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This function loads the mesh and ensures the mesh has normals; it also optimizes the 
// mesh for the graphics card's vertex cache, which improves performance by organizing 
// the internal triangle list for less cache misses.
//--------------------------------------------------------------------------------------
HRESULT FP_LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh )
{
    ID3DXMesh* pMesh = NULL;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
    // sample we'll ignore the X file's embedded materials since we know 
    // exactly the model we're loading.  See the mesh samples such as
    // "OptimizedMesh" for a more generic mesh loading example.
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, strFileName ) );
    V_RETURN( D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, pd3dDevice, NULL, NULL, NULL, NULL, &pMesh) );

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        ID3DXMesh* pTempMesh;
        V( pMesh->CloneMeshFVF( pMesh->GetOptions(), pMesh->GetFVF() | D3DFVF_NORMAL, pd3dDevice, &pTempMesh ) );
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
    V( pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL) );
    delete []rgdwAdjacency;

    *ppMesh = pMesh;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK FP_OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, 
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;
    //initPointSprites(pd3dDevice);
    g_GUI.OnD3D9ResetDevice(pd3dDevice, pBackBufferSurfaceDesc, pUserContext);
    g_pRenderSprites->OnResetDevice(pd3dDevice, pBackBufferSurfaceDesc);

    if( g_pEffect9 ) V_RETURN( g_pEffect9->OnResetDevice() );
    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 2.0f, 4000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );    

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_GUI.RenderSettingsDialog(fElapsedTime) )
        return;    

    HRESULT hr;
    D3DXMATRIXA16 mWorldViewProjection;
    //UINT iPass, cPasses;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
   
    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR(0.0f,0.05f,0.15f,0.55f), 1.0f, 0) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        mWorld = /*g_mCenterMesh * */*g_Camera.GetWorldMatrix();
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection = mWorld * mView * mProj;

        //V( g_pEffect9->SetValue( g_hLightDir, g_GUI.m_LightDir, sizeof(D3DXVECTOR3)*FP_MAX_LIGHTS ) );
        //V( g_pEffect9->SetValue( g_hLightDiffuse, g_GUI.m_cLightDiffuse, sizeof(D3DXCOLOR)*FP_MAX_LIGHTS ) );

        //// Update the effect's variables.  Instead of using strings, it would 
        //// be more efficient to cache a handle to the parameter by calling 
        //// ID3DXEffect::GetParameterByName
        //V( g_pEffect9->SetMatrix( g_hmWorldViewProjection, &mWorldViewProjection ) );
        //V( g_pEffect9->SetMatrix( g_hmWorld, &mWorld ) );

        //D3DXCOLOR vWhite = D3DXCOLOR(1,1,1,1);
        //V( g_pEffect9->SetValue(g_hMaterialDiffuseColor, &vWhite, sizeof(D3DXCOLOR) ) );
        //V( g_pEffect9->SetFloat( g_hfTime, (float)fTime ) );      
        //V( g_pEffect9->SetInt( g_hNumLights, g_NumActiveLights ) );

        //// Render the scene with this technique as defined in the .fx file
        //switch( g_NumActiveLights )
        //{
        //    case 1: V( g_pEffect9->SetTechnique( g_hRenderSceneWithTexture1Light ) ); break;
        //    case 2: V( g_pEffect9->SetTechnique( g_hRenderSceneWithTexture2Light ) ); break;
        //    case 3: V( g_pEffect9->SetTechnique( g_hRenderSceneWithTexture3Light ) ); break;
        //}

        //// Apply the technique contained in the effect and render the mesh
        //V( g_pEffect9->Begin(&cPasses, 0) );
        //for (iPass = 0; iPass < cPasses; iPass++)
        //{
        //    V( g_pEffect9->BeginPass(iPass) );
        //    V( g_pMesh9->DrawSubset(0) );
        //    V( g_pEffect9->EndPass() );
        //}
        //V( g_pEffect9->End() );

        pd3dDevice->SetTransform( D3DTS_WORLD, &mWorld );
        pd3dDevice->SetTransform( D3DTS_PROJECTION, &mProj );
        pd3dDevice->SetTransform( D3DTS_VIEW, &mView );        
       
        g_pRenderSprites->OnFrameRender(pd3dDevice, fTime, fElapsedTime);

        g_GUI.OnD3D9FrameRender(pd3dDevice, fTime, fElapsedTime, g_Camera.GetEyePt(),
                &mWorldViewProjection, &mWorld, &mView, &mProj, g_NumActiveLights,
                g_nActiveLight, g_fLightScale);



        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9LostDevice( void* pUserContext )
{
    g_GUI.OnD3D9LostDevice(pUserContext);
    if(g_pRenderSprites) g_pRenderSprites->OnLostDevice();
    if(g_pEffect9) g_pEffect9->OnLostDevice();    
    //shutDownPointSprites();    
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK FP_OnD3D9DestroyDevice( void* pUserContext )
{
    g_GUI.OnD3D9DestroyDevice(pUserContext);
    if(g_pRenderSprites) g_pRenderSprites->OnDetroyDevice();
    SAFE_RELEASE(g_pEffect9);
    SAFE_RELEASE(g_pMesh9);
    SAFE_RELEASE(g_pMeshTexture9);
}
