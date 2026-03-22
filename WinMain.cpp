//======================================================================
// INCLUDE
//======================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "WinMain.h"
#include "Render.h"
#include "Area.h"
#include "resource.h"
#include <commdlg.h>
#include <string>
#include <vector>
#include <fstream>

#pragma comment(lib,"comdlg32.lib")

#define LIST_LIMIT 1024
//======================================================================
// PROTOTYPE
//======================================================================
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, UINT wParam, LONG lParam );
LRESULT CALLBACK Dlg1Proc(HWND in_hWnd, UINT in_Message, WPARAM in_wParam, LPARAM in_lParam);
DWORD	ConvertStr2Dno( char* DataName );
DWORD	ConvertStr2Dno2( char* DataName );
BOOL GetFileNameFromDno(LPSTR filename,DWORD dwID);
//======================================================================
// GLOBAL
//======================================================================
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(PAI*2.0f)
char	ffxidir[512];
std::vector<std::string> g_ListArea;
std::string g_execDir;
extern	CArea			g_mArea;
extern	D3DLIGHT9		g_mLight,g_mLightbase;
extern	D3DXMATRIX		g_mProjection, g_mView,g_mEyeMat;
extern	float			g_mEyeScale,g_mEyeAlph,g_mEyeBeta;
extern	float			g_mLightAlph,g_mLightBeta;
extern	D3DXVECTOR3		g_mEye,g_mEyebase,g_mAt,g_mUp;
extern	float			g_mFov;				// FOV : 60度
extern	float			g_mAspect;			// 画面のアスペクト比
extern	float			g_mNear_z;			// 最近接距離
extern	float			g_mFar_z;			// 最遠方距離

long g_mScreenWidth	= 800;
long g_mScreenHeight	= 600;
static char *AppName = "AreaTest ver0.10";
static char *ClassName = "Area Viewer";
static DWORD FPS;

HWND hWindow;			// ウィンドウハンドル
HWND hDlg1;			// ダイアログ１

unsigned long Polygons;
float		g_mDispArea =	1500.f;
float		g_mDispTree =	1500.f;
D3DXVECTOR3	g_mEntry(0.f,0.f,0.f);
int			g_mDispValue=0;
int			g_mAreaBright=1;
char		g_mWeather[6]="suny";
char		g_className[6];


// 明るさ表示

	static const	LPCTSTR		ListBright[] = {
		" そのまま ",
		" 2倍の明るさ ",
		" 4倍の明るさ "
	};

// Disp Range

static const	LPCTSTR		ListRange[] = {
" 25 m, Near",
" 75 m, Short-range",
"1500 m, medium-range"
};

// 天候　表示

static const	LPCTSTR		ListWeather[] = {
"suny",
"fine",
"clod",
"mist",
"fogd",
"snow",
"dark",
"thdr",
"aura"
};

// Area
//char	ListArea[1024][256];
//int	NumListArea = 0;
//char	execDir[512];

//======================================================================
//
//		各種関数
//
//======================================================================
long GetScreenWidth( void ) { return g_mScreenWidth; }
long GetScreenHeight( void ) { return g_mScreenHeight; }
HWND GetWindow( void ) { return hWindow; }
void AdDrawPolygons( unsigned long polys ) { Polygons += polys; }

#define	EPSILON	0.0000001f

DWORD	ConvertStr2Dno2( char* DataName )
{
	int		Dno,Type,Hi,Lo;
	Type = 1;
	sscanf(DataName,"%d-%d-%d,",&Type,&Hi,&Lo);
	Dno = Type*0x10000 + Hi*0x80 + Lo%0x80;
	return Dno ;
}

DWORD	ConvertStr2Dno( char* DataName )
{
	int		Dno,Type,Hi,Lo;
	Type = 1;
	sscanf(DataName,"%d-%d,",&Hi,&Lo);
	Dno = Type*0x10000 + Hi*0x80 + Lo%0x80;
	return Dno ;
}

BOOL GetFileNameFromDno(LPSTR filename, DWORD Dno)
{
	int	no = LOWORD(Dno);

	const char* dir = ffxidir;
	if (HIWORD(Dno) == 1) sprintf(filename, "%sROM\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 2) sprintf(filename, "%sROM2\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 3) sprintf(filename, "%sROM3\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 4) sprintf(filename, "%sROM4\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 5) sprintf(filename, "%sROM5\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 6) sprintf(filename, "%sROM6\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 7) sprintf(filename, "%sROM7\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 8) sprintf(filename, "%sROM8\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	else if (HIWORD(Dno) == 9) sprintf(filename, "%sROM9\\%d\\%d.dat", dir, no / 0x80, no % 0x80);
	return TRUE;
}

void	MoveAt( void )
{
	D3DXVECTOR3	Pos,Post;

	Pos = g_mAt - g_mEye;
	D3DXVec3Normalize(&Pos,&Pos);
//	if( GetKeyState(VK_CONTROL)&0x8000 ){
		g_mAt.x += Pos.x*0.3f*10.f;
		g_mAt.z += Pos.z*0.3f*10.f;
	//} else {
	//	g_mAt.x += Pos.x*0.3f;
	//	g_mAt.z += Pos.z*0.3f;
	//}
	Post = g_mAt;
	//if( GetKeyState(VK_SHIFT)&0x8000 ) {
	//	if( g_mArea.CalcYPosition( &Post,20000.f,20000.f ) ) 
	//		g_mAt.y = Post.y+1.8f;
	//} else {
	//	if( g_mArea.CalcYPosition( &Post,2.f,5.f ) ) 
	//		g_mAt.y = Post.y+1.8f;
	//}
	D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
	g_mEye += g_mAt;
	D3DXVECTOR3 Eye;
	//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
	D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
}

void BackAt( void )
{
	D3DXVECTOR3	Pos,Post;

	Pos = g_mAt - g_mEye;
	D3DXVec3Normalize(&Pos,&Pos);
	//if( GetKeyState(VK_CONTROL)&0x8000 ){
		g_mAt.x -= Pos.x*0.3f*10.f;
		g_mAt.z -= Pos.z*0.3f*10.f;
	//} else {
	//	g_mAt.x -= Pos.x*0.3f;
	//	g_mAt.z -= Pos.z*0.3f;
	//}
	Post = g_mAt;
	//if( GetKeyState(VK_SHIFT)&0x8000 ) {
	//	if( g_mArea.CalcYPosition( &Post,20000.f,20000.f ) ) 
	//		g_mAt.y = Post.y+1.8f;
	//} else {
	//	if( g_mArea.CalcYPosition( &Post,2.f,5.f ) ) 
	//		g_mAt.y = Post.y+1.8f;	
	//}
	D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
	g_mEye += g_mAt;
	D3DXVECTOR3 Eye;
	//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
	D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
}

//======================================================================
//		WinMain関数
//======================================================================
int __stdcall WinMain( HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show )
{
	HKEY hKey;
	char tmp[512];
	GetCurrentDirectory(sizeof(tmp), tmp); g_execDir = tmp;
#if 0
	strcpy(ffxidir,"C:\\cross1");
#else
	ffxidir[0]=0;
	if( ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\PlayOnline\\InstallFolder",0,KEY_READ,&hKey) ){
		DWORD dwData = sizeof(ffxidir);
		DWORD dwType = REG_SZ;
		RegQueryValueEx(hKey,"0001",NULL,&dwType,(LPBYTE)ffxidir,&dwData);
		RegCloseKey( hKey );
	}
	if( !*ffxidir ){
		MessageBox(NULL,"Please start a PC on which you are installing the FinalFantasyXI！","FF XI is not installed",MB_OK);
		strcpy(ffxidir, g_execDir.c_str());
		return -1;
	}
#endif
	if( lstrlen(ffxidir)>0 ){
		if( ffxidir[lstrlen(ffxidir)-1]!='\\' ){
			lstrcat(ffxidir,"\\");
		}
	}
	timeBeginPeriod( 1 );

	//============================================================
	// ウィンドウクラス登録
	//============================================================
	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc		= WinProc; 
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= inst;
	wc.hIcon			= LoadIcon(inst, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= ( HBRUSH )GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName		= MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName	= ClassName;
	if ( RegisterClass( &wc ) == NULL ) return false;
	
	//============================================================
	// ウィンドウサイズ取得
	//============================================================
	long window_w = g_mScreenWidth + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
	long window_h = g_mScreenHeight + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);

	//============================================================
	// ウィンドウ生成
	//============================================================
	hWindow = CreateWindowEx(
				WS_EX_APPWINDOW,
				ClassName,
				AppName,
				WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE ,
				GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
				GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
				window_w,
				window_h,
				NULL,
				NULL,
				inst,
				NULL );
	// ダイアログ1作成
	hDlg1 = CreateDialog((HINSTANCE)GetWindowLong(hWindow, GWL_HINSTANCE), MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)Dlg1Proc);
	InvalidateRect(hDlg1, NULL, TRUE);
	SetWindowPos(hDlg1, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hDlg1, SW_HIDE);
	//============================================================
	// DirectXGraphics初期化
	//============================================================
	if ( InitD3D() == false ) return false;
	//============================================================
	// 描画処理初期化
	//============================================================
	if ( InitRender() == false ) return false;
	//============================================================
	// メッセージループ
	//============================================================
	MSG msg;
	D3DXVECTOR3	PosPC;
	while ( true )
	{
		//==================================================
		// メッセージ処理
		//==================================================
		if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
			if( !IsDialogMessage(hDlg1,&msg)  ) {
				if ( msg.message == WM_QUIT ) break;
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		//==================================================
		// 他
		//==================================================
		else{
			//======================================
			// FPSの計測
			//======================================
			
			static DWORD cnt=0,BeforeTime = timeGetTime();
			DWORD NowTime = timeGetTime();

			if ( (NowTime - BeforeTime-25*cnt) >= 25 ) {
				cnt++;
			}
			if ( NowTime - BeforeTime >= 500 ) {
				char FpsStr[128];
				sprintf( FpsStr, "%s  [ [ FPS : %03d/s ] [ POS: %d %d %d [ %upolygon/sec ]",
					AppName, FPS * 2, (int)g_mAt.x, (int)g_mAt.y, (int)g_mAt.z, Polygons * 2);
				SetWindowText( hWindow, FpsStr );
				BeforeTime = NowTime;
				Polygons = 0;
				FPS = 0;
				cnt = 0;
			}

			FPS++;

			//======================================
			// Direct3Dの描画
			//======================================

			// バックバッファと Z バッファをクリア
			GetDevice()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER| D3DCLEAR_STENCIL, D3DCOLOR_XRGB(200,200,255), 1.f, 0 );
			// シーン開始
			if SUCCEEDED( GetDevice()->BeginScene() ) {
				// 各種処理
				Rendering();

				// シーン終了
				GetDevice()->EndScene();

				// バックバッファの内容をプライマリに転送
				if FAILED( GetDevice()->Present( NULL, NULL, NULL, NULL ) ) {
					// リセット
					GetDevice()->Reset( GetAdapter() );
				}
			}
		}
	}

	//============================================================
	// 描画処理開放
	//============================================================
	UnInitRender();

	//============================================================
	// DirectXGraphics開放
	//============================================================
	ReleaseD3D();

	//============================================================
	// おしまい
	//============================================================
	timeEndPeriod( 1 );

	return msg.wParam;
}

//　モーションスピード　イベント処理
LRESULT CALLBACK Dlg1Proc(HWND in_hWnd, UINT in_Message,WPARAM in_wParam, LPARAM in_lParam )
{ 
	FILE			*fd;
	D3DXVECTOR3		Pos,Post,DPos,Dview;
	int				i,tType,ComboNo;
	char			ListName[256],ComboString[256];
	int				index,w1,w2,w3;
	char			tName[6],tClass[6],ww[512];
	CEffect			*pEffect;
	CKeyFrame		*pKeyframe;

	switch( in_Message ) {
        case WM_INITDIALOG:
		{
			g_ListArea.clear();
			std::string listPath = g_execDir + "\\List\\Area.lst";
			std::ifstream ifs(listPath);
			if (ifs) {
				std::string line;
				while (std::getline(ifs, line) && g_ListArea.size() < LIST_LIMIT) {
					g_ListArea.push_back(line);
				}
			}
		}
			//for( i=0 ; i<3 ; i++ ) {     
		 //       SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListBright[i]);
			//}
		    //SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_SETCURSEL, (WPARAM)g_mAreaBright, 0L);
//			for( i=0 ; i<NumListArea ; i++ ) { 
			for (i = 0; i < (int)g_ListArea.size() ; i++) {
					int	w5, w6, w7;
				sscanf(g_ListArea[i].c_str(),"%d-%d-%d,%d,%d,%d,%s",&w1,&w2,&w3,&w5,&w6,&w7,ww);
				sprintf(ComboString,"%d-%d-%d,%s",w1,w2,w3,ww);
		        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ComboString);
			}
		    SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0L);
			for( i=0 ; i<3 ; i++ ) {     
		        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListRange[i]);
			}
		    SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_SETCURSEL, (WPARAM)2, 0L);
 			for( i=0 ; i<3 ; i++ ) {     
		        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListRange[i]);
			}
		    SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_SETCURSEL, (WPARAM)2, 0L);
  	//		for( i=0 ; i<7 ; i++ ) {     
		 //       SendMessage(GetDlgItem(in_hWnd, IDC_COMBO6), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListWeather[i]);
			//}
		 //   SendMessage(GetDlgItem(in_hWnd, IDC_COMBO6), CB_SETCURSEL, (WPARAM)0, 0L);
			SetFocus(GetDlgItem(in_hWnd, IDC_COMBO1));
            break;
        case WM_HSCROLL:
           break;
		case WM_COMMAND:
			switch( LOWORD(in_wParam ) ) {
				case IDOK:
					ShowWindow(in_hWnd,SW_HIDE);
					break;
			case IDC_LIST1: // ｷｰﾌﾚｰﾑ
				if (HIWORD(in_wParam) == LBN_SELCHANGE) {
					while (SendMessage(GetDlgItem(in_hWnd, IDC_LIST2), LB_GETCOUNT, 0, 0) != 0) {
						SendMessage(GetDlgItem(in_hWnd, IDC_LIST2), LB_DELETESTRING, 0, 0);
					}
					index = SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_GETCURSEL, (WORD)0, 0L);
					SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_GETTEXT, (WORD)index, (LONG)ww);
					sscanf(ww, "[%02x] ｷｰﾌﾚｰﾑ (%4s)", &tType,tName);
					switch (tType){
						case 0x21:
						case 0x22:
						case 0x23:
						case 0x24:
						case 0x25:
						case 0x26:
						case 0x27:
						case 0x28:
						case 0x29:
						case 0x2d:
						case 0x2e:
						case 0x2f:
						case 0x60:
						case 0x61:
						case 0x62:
							pKeyframe = (CKeyFrame*)g_mArea.m_KeyFrames.Top();
							while (pKeyframe) {
								if (!memcmp(tName, pKeyframe->m_type.c_str(), strlen(tName))) {
									pKeyframe->outputValue(GetDlgItem(in_hWnd, IDC_LIST2));
									break;
								}
								pKeyframe = (CKeyFrame*)pKeyframe->Next;
							}
							SendMessage(GetDlgItem(in_hWnd, IDC_LIST2), LB_SETCURSEL, (WPARAM)0, 0L);
							break;
					}
				}
				break;
				case IDC_COMBO1:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						//	エリアの設定
						ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_GETCURSEL, 0L, 0L);
						sscanf(g_ListArea[ComboNo].c_str(),"%d-%d-%d,%f,%f,%f,%s",&w1,&w2,&w3,
							&g_mEntry.x,&g_mEntry.y,&g_mEntry.z,ww);
						g_mArea.SetArea( ConvertStr2Dno2((char*)g_ListArea[ComboNo].c_str()) );
						if( !g_mArea.LoadMAP() ) return -1;
						g_mAt.x = g_mEntry.x;g_mAt.y = g_mEntry.y;g_mAt.z = g_mEntry.z;
						D3DXVECTOR3 Eye;
						g_mEye = g_mAt; g_mEye.z += -5; g_mEye.y += 1.0;
						//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
						D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
						while (SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_DELETESTRING, 0, 0);
						}
						pEffect = (CEffect*)g_mArea.m_Effects.Top();
						while (pEffect) {
							index = SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_FINDSTRINGEXACT, 0, (LPARAM)pEffect->m_class.c_str());
							if (SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_FINDSTRINGEXACT,
								0, (LPARAM)pEffect->m_class.c_str()) < 0) {
								SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_ADDSTRING, (WPARAM)0, (LPARAM)pEffect->m_class.c_str());
							}
							pEffect = (CEffect*)pEffect->Next;
						}
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_SETCURSEL, (WPARAM)0, 0L);
						while (SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_DELETESTRING, 0, 0);
						}
						index = SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETCURSEL, (WORD)0, 0L);
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETLBTEXT, (WORD)index, (LONG)g_className);
						pEffect = (CEffect*)g_mArea.m_Effects.Top();
						while (pEffect) {
							if (!memcmp(pEffect->m_class.c_str(), g_className, 4)) {
								sprintf(ComboString, "ID[%s] class[%s]", pEffect->m_name.c_str(), pEffect->m_class.c_str());
								SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_ADDSTRING, (WPARAM)0, (LPARAM)ComboString);
							}
							pEffect = (CEffect*)pEffect->Next;
						}
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_SETCURSEL, (WPARAM)0, 0L);
						while (SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_DELETESTRING, 0, 0);
						}
						index = SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETCURSEL, (WORD)0, 0L);
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETLBTEXT, (WORD)index, (LONG)ComboString);
						sscanf(ComboString, "ID[%c%c%c%c] class[%c%c%c%c]", 
							&tName[0],&tName[1],&tName[2],&tName[3],
							&tClass[0],&tClass[1],&tClass[2],&tClass[3]);
						pEffect = (CEffect*)g_mArea.m_Effects.Top();
						while (pEffect) {
							if (!memcmp(tName, pEffect->m_name.c_str(), pEffect->m_name.length())&&
								!memcmp(tClass, pEffect->m_class.c_str(),pEffect->m_class.length()) ) {
								pEffect->outputProp(GetDlgItem(in_hWnd, IDC_LIST1));
								break;
							}
							pEffect = (CEffect*)pEffect->Next;
						}
						SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), CB_SETCURSEL, (WPARAM)0, 0L);
					}
					break;
				case IDC_COMBO2:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						//	表示距離の設定
						ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_GETCURSEL, 0L, 0L);
						sscanf(ListRange[ComboNo],"%d m,%s",&w1,ww);
						g_mDispArea = (float)w1;
					}
					break;
				case IDC_COMBO3:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						while (SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_DELETESTRING, 0, 0);
						}
						index = SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETCURSEL, (WORD)0, 0L);
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETLBTEXT, (WORD)index, (LONG)g_className);
						pEffect = (CEffect*)g_mArea.m_Effects.Top();
						while (pEffect) {
							if (!memcmp(pEffect->m_class.c_str(), g_className, 4)) {
								sprintf(ComboString, "ID[%s] class[%s]", pEffect->m_name.c_str(), pEffect->m_class.c_str());
								SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_ADDSTRING, (WPARAM)0, (LPARAM)ComboString);
							}
							pEffect = (CEffect*)pEffect->Next;
						}
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_SETCURSEL, (WPARAM)0, 0L);
					}
					break;
				case IDC_COMBO4:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						while (SendMessage(GetDlgItem(in_hWnd, IDC_LIST2), LB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_LIST2), LB_DELETESTRING, 0, 0);
						}
						while (SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_GETCOUNT, 0, 0) != 0) {
							SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), LB_DELETESTRING, 0, 0);
						}
						index = SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETCURSEL, (WORD)0, 0L);
						SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETLBTEXT, (WORD)index, (LONG)ComboString);
						sscanf(ComboString, "ID[%c%c%c%c] class[%c%c%c%c]", 
							&tName[0],&tName[1],&tName[2],&tName[3],
							&tClass[0],&tClass[1],&tClass[2],&tClass[3]);
						pEffect = (CEffect*)g_mArea.m_Effects.Top();
						while (pEffect) {
							if (!memcmp(tName, pEffect->m_name.c_str(), pEffect->m_name.length())&&
								!memcmp(tClass, pEffect->m_class.c_str(),pEffect->m_class.length()) ) {
								pEffect->outputProp(GetDlgItem(in_hWnd, IDC_LIST1));
								break;
							}
							pEffect = (CEffect*)pEffect->Next;
						}
						SendMessage(GetDlgItem(in_hWnd, IDC_LIST1), CB_SETCURSEL, (WPARAM)0, 0L);
					}
					break;
				case IDC_COMBO5:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						//	表示距離の設定
						ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_GETCURSEL, 0L, 0L);
						sscanf(ListRange[ComboNo],"%d m,%s",&w1,ww);
						g_mDispTree = (float)w1;
					}
					break;
				case IDC_COMBO6:
					if( HIWORD(in_wParam) == CBN_SELCHANGE ) {
						//	天候表示の設定
						ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO6), CB_GETCURSEL, 0L, 0L);
						memcpy(g_mWeather,ListWeather[ComboNo],4);
						if( !g_mArea.LoadMAP() ) return -1;
					}
					break;
				}
				break;
		case WM_CLOSE:
				ShowWindow(in_hWnd,SW_HIDE);
				break;
		case WM_DESTROY:
				PostQuitMessage(0);
				break;
	}
	return 0;
}
//========================================================================
//
//		メッセージ処理
//
//========================================================================
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, UINT wParam, LONG lParam )
{
static float	alpha = 0.,beta = 0.;
static float	Delta=0.,Step=0.2f;
	D3DXVECTOR3 Pos,Post;
	D3DXMATRIX	mm,m1,m2;
static bool		lDrag = false,rDrag = false;
static short	x1=-1,y1=-1,x2,y2;
OPENFILENAME	sfn;
char szFPath[256], szFName[256], strmsg[256];

	lstrcpy(szFPath, "*.mqo");
	ZeroMemory(&sfn, sizeof(sfn));
	sfn.lStructSize = sizeof(sfn);
	sfn.hwndOwner = NULL;
	sfn.lpstrFile = szFPath;
	sfn.nMaxFile = sizeof(szFPath);
	sfn.lpstrFilter = "MQO Format(*.mqo)\0*.mqo\0";
	sfn.nFilterIndex = 1;
	sfn.lpstrFileTitle = szFName;
	sfn.nMaxFileTitle = sizeof(szFName);
	sfn.lpstrTitle = "MQO Save";
	sfn.lpstrInitialDir = NULL;
	switch (msg)
	{
		//==============================================
		//	終了時
		//==============================================
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		case WM_MOUSEWHEEL:
			if (wParam == 0x00780000) {
				MoveAt();
			} else if (wParam == 0xff880000) {
				BackAt();
			}
			break;
		case WM_MOUSEMOVE:
			x2 = LOWORD(lParam);
			y2 = HIWORD(lParam);
			if( wParam & MK_LBUTTON ) {
				if( abs(x2-x1)<20 && abs(y2-y1) <20 ) {
					g_mEyeAlph += (float)(x2-x1)/(float)g_mScreenWidth*2.f*PAI;
					g_mEyeBeta += (float)(y1-y2)/(float)g_mScreenWidth*2.f*PAI;
					g_mEyeAlph = (g_mEyeAlph>PAI2)?(g_mEyeAlph-PAI2):g_mEyeAlph;
					g_mEyeAlph = (g_mEyeAlph<-PAI2)?(g_mEyeAlph+PAI2):g_mEyeAlph;
					g_mEyeBeta = (g_mEyeBeta>(PAI/2.f))?(PAI/2.f-0.02f):g_mEyeBeta;
					g_mEyeBeta = (g_mEyeBeta<(-PAI/2.f))?(-PAI/2.f+0.02f):g_mEyeBeta;
					D3DXMATRIX matY,matX,matS;
					D3DXMatrixRotationY(&matY,g_mEyeAlph);
					D3DXMatrixRotationX(&matX,g_mEyeBeta);
					D3DXMatrixScaling(&matS,g_mEyeScale,g_mEyeScale,g_mEyeScale);
					g_mEyeMat = matS * matX * matY;
					D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
					g_mEye += g_mAt;
					D3DXVECTOR3 Eye;
					//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) 
					//	g_mEye = Eye;
					D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
				}
			}
			else if (wParam & MK_MBUTTON) {
				if (abs(x2 - x1)<20 && abs(y2 - y1) <20) {
					g_mLightAlph += (float)(x2 - x1) / (float)g_mScreenWidth*2.f*PAI;
					g_mLightBeta += (float)(y1 - y2) / (float)g_mScreenWidth*2.f*PAI;
					g_mLightAlph = (g_mLightAlph>PAI2) ? (g_mLightAlph - PAI2) : g_mLightAlph;
					g_mLightAlph = (g_mLightAlph<-PAI2) ? (g_mLightAlph + PAI2) : g_mLightAlph;
					g_mLightBeta = (g_mLightBeta>(PAI / 2.f)) ? (PAI / 2.f - 0.02f) : g_mLightBeta;
					g_mLightBeta = (g_mLightBeta<(-PAI / 2.f)) ? (-PAI / 2.f + 0.02f) : g_mLightBeta;
					D3DXMATRIX mat, matY, matX;
					D3DXMatrixRotationY(&matY, g_mLightAlph);
					D3DXMatrixRotationX(&matX, g_mLightBeta);
					mat = matX * matY;
					D3DXVec3TransformNormal((D3DXVECTOR3*)&g_mLight.Direction, (D3DXVECTOR3*)&g_mLightbase.Direction, &mat);
					D3DXVec3Normalize((D3DXVECTOR3*)&g_mLight.Direction, (D3DXVECTOR3*)&g_mLight.Direction);
					GetDevice()->SetLight(0, &g_mLight);
				}
			}
			else if (wParam & MK_RBUTTON) {
				if (abs(x2-x1)<20 && abs(y2 - y1) <20) {
					Pos = g_mAt - g_mEye;
					D3DXVec3Normalize(&Pos, &Pos);
					D3DXMatrixRotationY(&mm, -PAI / 2.);
					D3DXVec3TransformCoord(&Pos, &Pos, &mm);
					//	if( GetKeyState(VK_CONTROL)&0x8000 ){
					g_mAt.x += Pos.x*((x2 - x1)/10.f);
					g_mAt.z += Pos.z*((x2 - x1)/10.f);
//					g_mAt.x += (x2 - x1) / 10.f;
					g_mAt.y += (y2 - y1) / 10.f;
					D3DXVec3TransformNormal(&g_mEye, &g_mEyebase, &g_mEyeMat);
					g_mEye += g_mAt;
					D3DXMatrixLookAtLH(&g_mView, &g_mEye, &g_mAt, &g_mUp);
				}
			}
			x1 = x2; y1 = y2;
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == ID_MNU_OASPD ) {
				ShowWindow( hDlg1,SW_SHOW );
			}
			if( LOWORD(wParam) == ID_MNU_W320 ) {
				long window_w = 320 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 240 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 320; g_mScreenHeight = 240;
			}
			if( LOWORD(wParam) == ID_MNU_W640 ) {
				long window_w = 640 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 480 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 640; g_mScreenHeight = 480;
			}
			if( LOWORD(wParam) == ID_MNU_W800 ) {
				long window_w = 800 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 600 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 800; g_mScreenHeight = 600;
			}
			if( LOWORD(wParam) == ID_MNU_W1280) {
				long window_w = 1280 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 720 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 1280; g_mScreenHeight = 720;
			}
			if( LOWORD(wParam) == ID_MNU_W1920 ) {
				long window_w = 1920 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 1080 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 1920; g_mScreenHeight = 1080;
			}
			if( LOWORD(wParam) == ID_MNU_W1600 ) {
				long window_w = 1600 + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
				long window_h = 1200 + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
				MoveWindow(hWnd,
					GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
					GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
					window_w,window_h,true);
				g_mScreenWidth = 1600; g_mScreenHeight = 1200;
			}
			if (LOWORD(wParam) == ID_MNU_META) {
				sfn.lpstrTitle = "MQO Save";
				if (GetSaveFileName(&sfn)){
					if (!g_mArea.saveMQO(szFPath, szFName, g_mAt.x, g_mAt.y, g_mAt.z)) {
						wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
						MessageBox(NULL, strmsg, "Open Save File", MB_OK | MB_ICONINFORMATION);
					}
				}
			}
			if (LOWORD(wParam) == ID_MNU_EFFT) {
				//wsprintf(strmsg, "This feature is not supported", szFPath);
				//MessageBox(NULL, strmsg, "not supported", MB_OK | MB_ICONINFORMATION);
				sfn.lpstrTitle = "MQO Save Effect Data";
				if (GetSaveFileName(&sfn)){
					if (!g_mArea.saveMQO2(szFPath, szFName, g_mAt.x, g_mAt.y, g_mAt.z)) {
						wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
						MessageBox(NULL, strmsg, "Open Save File", MB_OK | MB_ICONINFORMATION);
					}
				}
			}
			if (LOWORD(wParam) == ID_MNU_EFF2) {
				//wsprintf(strmsg, "This feature is not supported", szFPath);
				//MessageBox(NULL, strmsg, "not supported", MB_OK | MB_ICONINFORMATION);
				sfn.lpstrTitle = "MQO Save EffectModel Data";
				if (GetSaveFileName(&sfn)){
					if (!g_mArea.saveMQO3(szFPath, szFName)) {
						wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
						MessageBox(NULL, strmsg, "Open Save File", MB_OK | MB_ICONINFORMATION);
					}
				}
			}
			if (LOWORD(wParam) == ID_MNU_EXIT) {
				if( MessageBox(NULL, "Do you really want to quit?", "Quit Program", MB_YESNO | MB_ICONQUESTION ) == IDYES ) {
					SendMessage(hWnd, WM_CLOSE, 0L, 0L);
				}
			}
		case WM_KEYDOWN:
			if( wParam==VK_INSERT ){
				if( g_mEyeScale <=0.2f ) break;
				if( GetKeyState(VK_CONTROL)&0x8000 ){
					g_mEyeScale -= 0.02f * 10.f;
				} else {
					g_mEyeScale -= 0.02f;
				}
				D3DXMATRIX matY,matX,matS;
				D3DXMatrixRotationY(&matY,g_mEyeAlph);
				D3DXMatrixRotationX(&matX,g_mEyeBeta);
				D3DXMatrixScaling(&matS,g_mEyeScale,g_mEyeScale,g_mEyeScale);
				g_mEyeMat = matS * matX * matY;
				D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				g_mEye += g_mAt;
				D3DXVECTOR3 Eye;
				//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			} else if( wParam==VK_DELETE ){
				if( GetKeyState(VK_CONTROL)&0x8000 ){
					g_mEyeScale += 0.02f * 10.f;
				} else {
					g_mEyeScale += 0.02f;
				}
				D3DXMATRIX matY,matX,matS;
				D3DXMatrixRotationY(&matY,g_mEyeAlph);
				D3DXMatrixRotationX(&matX,g_mEyeBeta);
				D3DXMatrixScaling(&matS,g_mEyeScale,g_mEyeScale,g_mEyeScale);
				g_mEyeMat = matS * matX * matY;
				D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				g_mEye += g_mAt;
				D3DXVECTOR3 Eye;
				//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			} else if( wParam==VK_PRIOR ){
				if (GetKeyState(VK_CONTROL) & 0x8000){
					g_mAt.y += 0.05f * 10.f;
				}
				else {
					g_mAt.y += 0.05f;
				}
				D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				g_mEye += g_mAt;
				D3DXVECTOR3 Eye;
				//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			} else if( wParam==VK_NEXT ){
				if (GetKeyState(VK_CONTROL) & 0x8000){
					g_mAt.y -= 0.05f * 10.f;
				}
				else {
					g_mAt.y -= 0.05f;
				}
				D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				g_mEye += g_mAt;
				D3DXVECTOR3 Eye;
				//if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			} else if( wParam==VK_HOME ){
			} else if( wParam==VK_UP || wParam=='W' || wParam=='w' ){
				MoveAt();
			}
			else if (wParam == VK_DOWN || wParam == 'S' || wParam == 's'){
				BackAt();
			}
			else if (wParam == VK_RIGHT || wParam == 'D' || wParam == 'd'){
				Pos = g_mAt - g_mEye;
				D3DXVec3Normalize(&Pos, &Pos);
				D3DXMatrixRotationY(&mm, PAI / 2.);
				D3DXVec3TransformCoord(&Pos, &Pos, &mm);
				g_mAt.x += Pos.x*0.3f* 10.f;
				g_mAt.z += Pos.z*0.3f* 10.f;
				D3DXVec3TransformNormal(&g_mEye, &g_mEyebase, &g_mEyeMat);
				g_mEye += g_mAt;
				D3DXMatrixLookAtLH(&g_mView, &g_mEye, &g_mAt, &g_mUp);
				//g_mEyeAlph +=PAI/64.f;
				//g_mEyeAlph = (g_mEyeAlph>PAI2)?(g_mEyeAlph-PAI2):g_mEyeAlph;
				//g_mEyeAlph = (g_mEyeAlph<-PAI2)?(g_mEyeAlph+PAI2):g_mEyeAlph;
				//D3DXMATRIX matY,matX,matS;
				//D3DXMatrixRotationY(&matY,g_mEyeAlph);
				//D3DXMatrixRotationX(&matX,g_mEyeBeta);
				//D3DXMatrixScaling(&matS,g_mEyeScale,g_mEyeScale,g_mEyeScale);
				//g_mEyeMat = matS * matX * matY;
				//D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				//g_mEye += g_mAt;
				//D3DXVECTOR3 Eye;
				////if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				//D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			}
			else if (wParam == VK_LEFT || wParam == 'A' || wParam == 'a'){
				Pos = g_mAt - g_mEye;
				D3DXVec3Normalize(&Pos, &Pos);
				D3DXMatrixRotationY(&mm, -PAI / 2.);
				D3DXVec3TransformCoord(&Pos, &Pos, &mm);
				g_mAt.x += Pos.x*0.3f* 10.f;
				g_mAt.z += Pos.z*0.3f* 10.f;
				D3DXVec3TransformNormal(&g_mEye, &g_mEyebase, &g_mEyeMat);
				g_mEye += g_mAt;
				D3DXMatrixLookAtLH(&g_mView, &g_mEye, &g_mAt, &g_mUp);
				//g_mEyeAlph -=PAI/64.f;
				//g_mEyeAlph = (g_mEyeAlph>PAI2)?(g_mEyeAlph-PAI2):g_mEyeAlph;
				//g_mEyeAlph = (g_mEyeAlph<-PAI2)?(g_mEyeAlph+PAI2):g_mEyeAlph;
				//D3DXMATRIX matY,matX,matS;
				//D3DXMatrixRotationY(&matY,g_mEyeAlph);
				//D3DXMatrixRotationX(&matX,g_mEyeBeta);
				//D3DXMatrixScaling(&matS,g_mEyeScale,g_mEyeScale,g_mEyeScale);
				//g_mEyeMat = matS * matX * matY;
				//D3DXVec3TransformNormal(&g_mEye,&g_mEyebase,&g_mEyeMat);
				//g_mEye += g_mAt;
				//D3DXVECTOR3 Eye;
				////if( g_mArea.CalcEyePosition( &g_mAt, &Eye ) ) g_mEye = Eye;
				//D3DXMatrixLookAtLH( &g_mView, &g_mEye, &g_mAt, &g_mUp );
			}
			break;
		//==============================================
		//	その他
		//==============================================
		default:
			return DefWindowProc( hWnd, msg, wParam, lParam );
	}

	return 0;
}

