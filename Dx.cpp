

//======================================================================
// INCLUDE
//======================================================================
#include "WinMain.h"
#include "Dx.h"


//======================================================================
// GLOBAL
//======================================================================

//



//======================================================================
//		DirectXGraphics初期化
//======================================================================
bool InitD3D( void )
{
	HRESULT hr;
	D3DDISPLAYMODE d3ddm;
	
	//==============================================================================
	// Direct3D オブジェクトを作成
	//==============================================================================
	g_pDirect3D = Direct3DCreate9( D3D_SDK_VERSION );
	if ( g_pDirect3D == NULL ) {
		MessageBox( NULL, "Direct3Dの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP );
		return false;
	}

	//==============================================================================
	// 現在の画面モードを取得
	//==============================================================================
	hr = g_pDirect3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	if FAILED( hr ) {
		MessageBox( NULL, "画面モードの取得に失敗しました", "Error", MB_OK | MB_ICONSTOP );
		return false;
	}

	//==============================================================================
	// Direct3D 初期化パラメータの設定
	//==============================================================================
	ZeroMemory( &g_md3dpp, sizeof(D3DPRESENT_PARAMETERS) );

	g_md3dpp.BackBufferCount	= 1;
	g_md3dpp.Windowed			= TRUE;
	g_md3dpp.BackBufferWidth	= GetScreenWidth();
	g_md3dpp.BackBufferHeight	= GetScreenHeight();

	// ウインドウ : 現在の画面モードを使用
	g_md3dpp.BackBufferFormat		= d3ddm.Format;
	g_md3dpp.MultiSampleType		= D3DMULTISAMPLE_NONE;
	g_md3dpp.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	//g_md3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;
	g_md3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_DEFAULT;
	g_md3dpp.hDeviceWindow			= GetWindow();

	// Z バッファの自動作成
	g_md3dpp.EnableAutoDepthStencil	= TRUE;
	g_md3dpp.AutoDepthStencilFormat	= D3DFMT_D24S8;
	g_md3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;//ダブルステンシル

	//==============================================================================
	// シェーダーバージョン取得
	//==============================================================================
	D3DCAPS9 caps;
	g_pDirect3D->GetDeviceCaps( 0, D3DDEVTYPE_HAL, &caps );
	g_mVertexShaderVersion = caps.VertexShaderVersion;
	g_mMaxVertexShaderConst = caps.MaxVertexShaderConst;

	//==============================================================================
	// デバイスの生成
	//==============================================================================

	// 頂点シェーダー1.1？
	if ( g_mVertexShaderVersion >= D3DVS_VERSION(1,1) ) {
		// HARDWARE T&L
		if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
			// SOFTWARE T&L
			if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
				// REFERENCE RASTERIZE
				if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {				
					MessageBox( NULL,"Direct3Dデバイスの生成に失敗しました", "Error" , MB_OK | MB_ICONSTOP );
					return false;
				}
			}
		}
	} else {
		g_mIsUseSoftware = TRUE;	// HARDWARE&SOFTWARE T&L
		if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_MIXED_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
			// SOFTWARE T&L
			if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {
				// REFERENCE RASTERIZE
				if FAILED( g_pDirect3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice ) ) {				
					MessageBox( NULL,"Direct3Dデバイスの生成に失敗しました", "Error" , MB_OK | MB_ICONSTOP );
					return false;
				}
			}
		}
	}

	return true;
}

//======================================================================
//
//		DirectXGraphics開放
//
//======================================================================
void ReleaseD3D( void )
{
	if ( g_pD3DDevice != NULL ) g_pD3DDevice->Release();
	if ( g_pDirect3D != NULL ) g_pDirect3D->Release();
}


//======================================================================
//
//		頂点バッファ生成
//
//======================================================================
HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf )
{
	HRESULT hr = g_pD3DDevice->CreateVertexBuffer(
					size,
					Usage,
					fvf,
					D3DPOOL_MANAGED,
					lpVB,NULL );
	return hr;
}

//======================================================================
//
//		インデックスバッファ生成
//
//======================================================================
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage )
{
	HRESULT hr = g_pD3DDevice->CreateIndexBuffer(
					size,
					Usage,
					D3DFMT_INDEX16,
					D3DPOOL_MANAGED,
					lpIB,NULL );
	return hr;
}
