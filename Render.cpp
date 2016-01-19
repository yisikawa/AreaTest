

//======================================================================
// INCLUDE
//======================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "WinMain.h"
#include "Render.h"
#include "Area.h"

//======================================================================
// PROTOTYPE
//======================================================================
DWORD	ConvertStr2Dno2( char* DataName );
//======================================================================
// DEFINE
//======================================================================
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(PAI*2.0f)

inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

//======================================================================
// GLOBAL
//======================================================================
static LPDIRECT3D9				g_pDirect3D;
static LPDIRECT3DDEVICE9		g_pD3DDevice;
static D3DPRESENT_PARAMETERS	g_md3dpp;
unsigned long					g_mVertexShaderVersion;
int								g_mMaxVertexShaderConst = 0; // ���_�V�F�[�_�[�@MAX�@Matrix
BOOL							g_mIsUseSoftware = FALSE;

float		g_mTime				=	0.;
		CArea		g_mArea;
extern	HWND 		hDlg1,hTrack;
extern	float		g_mDispArea;
extern	float		g_mDispTree;
float				g_mFov			= PAI / 4.f;		// FOV : 60�x
float				g_mAspect		= 1.34f;		// ��ʂ̃A�X�y�N�g��
float				g_mNear_z		= 0.1f;			// �ŋߐڋ���
float				g_mFar_z		= 1400.0f;		// �ŉ�������
D3DLIGHT9			g_mLight,g_mLightbase;
static float		fTime		= 0;
extern	unsigned long	VertexShaderVersion;
extern	int				MaxVertexShaderConst; // ���_�V�F�[�_�[�@MAX�@Matrix

D3DXMATRIX			g_mProjection, g_mView, g_mEyeMat;
float				g_mEyeScale=1.f,g_mEyeAlph = 0.f,g_mEyeBeta = 0.f;
float				g_mLightAlph = 0.f,g_mLightBeta = 0.f;
D3DXVECTOR3			g_mEye,g_mEyebase( 0.0f,	 1.1f, -4.5f);
D3DXVECTOR3			g_mAt(	0.0f,	 1.1f,	0.0f);
D3DXVECTOR3			g_mUp(	0.0f,	 1.0f,	0.0f);
LPDIRECT3DSURFACE9	g_pBackBuffer;					// �o�b�N�o�b�t�@
LPDIRECT3DSURFACE9	g_pZBuffer;						// Z�o�b�t�@
float				g_mLightDist = 1.5f;
D3DXVECTOR3			g_mLightPosition(0.f,0.f,0.f);
D3DXMATRIX			g_mViewLight;					// ���C�g���猩���ꍇ�̃r���[�}�g���b�N�X

extern	long g_mScreenWidth;
extern	long g_mScreenHeight;
//======================================================================
//		�e��֐�
//======================================================================
LPDIRECT3DDEVICE9 GetDevice(void) { return g_pD3DDevice; }
D3DPRESENT_PARAMETERS *GetAdapter(void) { return &g_md3dpp; }
unsigned long GetVertexShaderVersion(void) { return g_mVertexShaderVersion; }
//======================================================================
//		DirectXGraphics������
//======================================================================
bool InitD3D(void)
{
	HRESULT hr;
	D3DDISPLAYMODE d3ddm;

	//==============================================================================
	// Direct3D �I�u�W�F�N�g���쐬
	//==============================================================================
	g_pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pDirect3D == NULL) {
		MessageBox(NULL, "It failed to create a Direct3D", "Error", MB_OK | MB_ICONSTOP);
		return false;
	}

	//==============================================================================
	// ���݂̉�ʃ��[�h���擾
	//==============================================================================
	hr = g_pDirect3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	if FAILED(hr) {
		MessageBox(NULL, "It failed to get the screen mode", "Error", MB_OK | MB_ICONSTOP);
		return false;
	}

	//==============================================================================
	// Direct3D �������p�����[�^�̐ݒ�
	//==============================================================================
	ZeroMemory(&g_md3dpp, sizeof(D3DPRESENT_PARAMETERS));

	g_md3dpp.BackBufferCount = 1;
	g_md3dpp.Windowed = TRUE;
	g_md3dpp.BackBufferWidth = GetScreenWidth();
	g_md3dpp.BackBufferHeight = GetScreenHeight();

	// �E�C���h�E : ���݂̉�ʃ��[�h���g�p
	g_md3dpp.BackBufferFormat = d3ddm.Format;
	g_md3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	g_md3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	//g_md3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;
	g_md3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	g_md3dpp.hDeviceWindow = GetWindow();

	// Z �o�b�t�@�̎����쐬
	g_md3dpp.EnableAutoDepthStencil = TRUE;
	g_md3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	g_md3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;//�_�u���X�e���V��

	//==============================================================================
	// �V�F�[�_�[�o�[�W�����擾
	//==============================================================================
	D3DCAPS9 caps;
	g_pDirect3D->GetDeviceCaps(0, D3DDEVTYPE_HAL, &caps);
	g_mVertexShaderVersion = caps.VertexShaderVersion;
	g_mMaxVertexShaderConst = caps.MaxVertexShaderConst;

	//==============================================================================
	// �f�o�C�X�̐���
	//==============================================================================

	// ���_�V�F�[�_�[1.1�H
	if (g_mVertexShaderVersion >= D3DVS_VERSION(1, 1)) {
		// HARDWARE T&L
		if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
			// SOFTWARE T&L
			if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
				// REFERENCE RASTERIZE
				if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
					MessageBox(NULL, "It failed to generate the Direct3D device", "Error", MB_OK | MB_ICONSTOP);
					return false;
				}
			}
		}
	}
	else {
		g_mIsUseSoftware = TRUE;	// HARDWARE&SOFTWARE T&L
		if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_MIXED_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
			// SOFTWARE T&L
			if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
				// REFERENCE RASTERIZE
				if FAILED(g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_md3dpp, &g_pD3DDevice)) {
					MessageBox(NULL, "It failed to generate the Direct3D device", "Error", MB_OK | MB_ICONSTOP);
					return false;
				}
			}
		}
	}

	return true;
}

//======================================================================
//		DirectXGraphics�J��
//======================================================================
void ReleaseD3D(void)
{
	if (g_pD3DDevice != NULL) g_pD3DDevice->Release();
	if (g_pDirect3D != NULL) g_pDirect3D->Release();
}


//======================================================================
//		���_�o�b�t�@����
//======================================================================
HRESULT CreateVB(LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf)
{
	HRESULT hr = g_pD3DDevice->CreateVertexBuffer(
		size,
		Usage,
		fvf,
		D3DPOOL_MANAGED,
		lpVB, NULL);
	return hr;
}

//======================================================================
//		�C���f�b�N�X�o�b�t�@����
//======================================================================
HRESULT CreateIB(LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage)
{
	HRESULT hr = g_pD3DDevice->CreateIndexBuffer(
		size,
		Usage,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		lpIB, NULL);
	return hr;
}

//======================================================================
//		�����_�����O
//======================================================================
void Rendering( void )
{
	D3DXVECTOR3		Pos;
	static unsigned long OldTime = timeGetTime();
	unsigned long NowTime = timeGetTime();

	fTime = (float)(NowTime - OldTime) / 1000.0f;
	OldTime = NowTime;
//	g_mTime += fTime*g_mMotionSpeed;

	// �ϊ��K�p�i�����̓A�j������

	//-----------------------------------------------
	// �����_�����O
	//-----------------------------------------------
	unsigned long poly = 0;
	//	���C�g�ʒu�̌v�Z
	g_mLightPosition = g_mAt+g_mLightDist*-(D3DXVECTOR3)g_mLight.Direction;
	D3DXMatrixLookAtLH( &g_mViewLight,&g_mLightPosition,&g_mAt,&g_mUp);

	poly += g_mArea.Rendering(g_mAt.x,g_mAt.y,g_mAt.z);
	AdDrawPolygons( poly );
}

//======================================================================
//		3D��Ԃ̐���
//======================================================================
bool Create3DSpace( void )
{
	HRESULT	hr;
	//===========================================================
	// �o�b�N�o�b�t�@�擾
	//===========================================================
	hr = GetDevice()->GetRenderTarget( 0,&g_pBackBuffer );
	if FAILED( hr ) return false;

	//===========================================================
	// Z�o�b�t�@����
	//===========================================================
	hr = GetDevice()->GetDepthStencilSurface( &g_pZBuffer );
	if FAILED( hr ) return false;

	//===========================================================
	// �v���W�F�N�V�����s��̐ݒ�
	//===========================================================
	// �s�񐶐�
	D3DXMatrixPerspectiveFovLH( &g_mProjection, g_mFov, g_mAspect, g_mNear_z, g_mFar_z );

	//===========================================================
	// �f�t�H���g�̃J�����̐ݒ�
	//===========================================================
	D3DXMatrixIdentity(&g_mEyeMat);
	D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
	//=================================================
	// �����_�����O�X�e�[�g
	//=================================================
	float	start	 = 0.0f;
	float	end		 = 1.0f;
	GetDevice()->SetRenderState( D3DRS_DITHERENABLE,		TRUE );
	GetDevice()->SetRenderState( D3DRS_ZENABLE,				TRUE );
	GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,		TRUE );
	GetDevice()->SetRenderState( D3DRS_FOGTABLEMODE,		D3DFOG_NONE );
	GetDevice()->SetRenderState( D3DRS_FOGVERTEXMODE,		D3DFOG_LINEAR );
	GetDevice()->SetRenderState( D3DRS_FOGCOLOR,			D3DCOLOR_XRGB(200,200,255) );
	GetDevice()->SetRenderState( D3DRS_FOGSTART,			*(DWORD*)(&start) );
	GetDevice()->SetRenderState( D3DRS_FOGEND,				*(DWORD*)(&end) );
	//=================================================
	// ���C�g
	//=================================================
	memset( &g_mLight, 0x00, sizeof(D3DLIGHT9) );
	memset( &g_mLightbase, 0x00, sizeof(D3DLIGHT9) );
	g_mLight.Type			= D3DLIGHT_DIRECTIONAL;
	g_mLight.Diffuse.a		= 1.0f;
	g_mLight.Diffuse.r		= 0.8f;
	g_mLight.Diffuse.g		= 0.8f;
	g_mLight.Diffuse.b		= 0.8f;
	g_mLight.Ambient.a		= 1.0f;
	g_mLight.Ambient.r		= 0.5f;
	g_mLight.Ambient.g		= 0.5f;
	g_mLight.Ambient.b		= 0.5f;
	g_mLight.Specular.a	= 1.0f;
	g_mLight.Specular.r	= 0.5f;
	g_mLight.Specular.g	= 0.5f;
	g_mLight.Specular.b	= 0.5f;
	D3DXVec3Normalize( (D3DXVECTOR3*)&g_mLightbase.Direction, &D3DXVECTOR3( 0.3f, -1.0f, 0.3f) );
	g_mLight.Direction = g_mLightbase.Direction;
	GetDevice()->SetLight( 0, &g_mLight );
	GetDevice()->LightEnable( 0, TRUE );

	//===========================================================
	// ���C�g�����̃J�����̐ݒ�
	//===========================================================

	g_mLightPosition = g_mAt+g_mLightDist*-(D3DXVECTOR3)g_mLight.Direction;
	D3DXMatrixLookAtLH( &g_mViewLight,&g_mLightPosition,&g_mAt,&g_mUp);
	return true;
}

//======================================================================
//		������
//======================================================================
bool InitRender( void )
{
	D3DXVECTOR3		Pos,Post;
	char			ComboString[128];
	int				w1,w2,w3;
	char			ww[128];
	//--------------------------------------------------
	// �����ݒ�
	//--------------------------------------------------
	if ( !Create3DSpace() )
	{
		MessageBox( NULL, "It failed to initialize", "Error", MB_OK );
		return false;
	}
	//--------------------------------------------------
	// ���f���f�[�^�ǂݍ��݁i���_�t�H�[�}�b�g���w��
	//--------------------------------------------------
	// unsigned long ModelFVF = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
	// MAP �ݒ�
	GetWindowText(GetDlgItem(hDlg1, IDC_COMBO1), ComboString, sizeof(ComboString));
	sscanf(ComboString,"%d-%d-%d,%s",&w1,&w2,&w3,ww);
	g_mArea.SetArea( ConvertStr2Dno2(ComboString) );
	// MAP �����ݒ�
	if( !g_mArea.LoadMAP() ) return false;
	g_mArea.CreateVertexShader();
	return true;
}

//======================================================================
//		�J��
//======================================================================
void UnInitRender( void )
{
}



