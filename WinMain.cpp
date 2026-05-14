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
#pragma comment(lib,"ole32.lib")

#define LIST_LIMIT 1024
//======================================================================
// PROTOTYPE
//======================================================================
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK Dlg1Proc(HWND in_hWnd, UINT in_Message, WPARAM in_wParam, LPARAM in_lParam);
LRESULT CALLBACK CanvasProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD   ConvertStr2Dno( char* DataName );
DWORD   ConvertStr2Dno2( char* DataName );
BOOL    GetFileNameFromDno(LPSTR filename, DWORD dwID);
//======================================================================
// GLOBAL
//======================================================================
#define PAI   (3.1415926535897932384626433832795f)
#define PAI2  (PAI*2.0f)
char    ffxidir[512];
char    g_meshPath[512] = "";
char    g_texPath[512]  = "";
std::vector<std::string> g_ListArea;
std::string g_execDir;

extern  CArea       g_mArea;
extern  XMFLOAT4X4  g_mProjection, g_mView, g_mEyeMat;
extern  float       g_mEyeScale, g_mEyeAlph, g_mEyeBeta;
extern  float       g_mLightAlph, g_mLightBeta;
extern  XMFLOAT3    g_mEye, g_mEyebase, g_mAt, g_mUp;
extern  XMFLOAT3    g_mLightDir, g_mLightDirBase;
extern  float       g_mFov;
extern  float       g_mAspect;
extern  float       g_mNear_z;
extern  float       g_mFar_z;

long g_mScreenWidth  = 1280;
long g_mScreenHeight = 720;
static char* AppName   = "AreaTest ver0.10";
static char* ClassName = "Area Viewer";
static DWORD FPS;

HWND hWindow;
HWND hDlg1;
HWND hCanvas = NULL;
static std::vector<std::string> g_canvasLines;
static int g_scrollPos = 0;
static std::vector<int> g_objInstances;
static int  g_objInstCur         = -1;
static int  g_instBaseLine       = 0;
static int  g_canvasHighlightLine = -1;
static std::vector<CEffect*> g_combo4Effects;

unsigned long Polygons;
float      g_mDispArea = 1500.f;
float      g_mDispTree = 1500.f;
XMFLOAT3   g_mEntry    = { 0.f, 0.f, 0.f };
int        g_mDispValue = 0;
int        g_mAreaBright = 1;
char       g_mWeather[6] = "suny";
char       g_className[6];

static const LPCTSTR ListBright[] = {
    " そのまま ",
    " 2倍の明るさ ",
    " 4倍の明るさ "
};

static const LPCTSTR ListRange[] = {
    " 25 m, Near",
    " 75 m, Short-range",
    "1500 m, medium-range"
};

static const LPCTSTR ListWeather[] = {
    "suny","fine","clod","mist","fogd","snow","dark","thdr","aura"
};

//======================================================================
// ユーティリティ
//======================================================================
long GetScreenWidth( void )  { return g_mScreenWidth; }
long GetScreenHeight( void ) { return g_mScreenHeight; }
HWND GetWindow( void )       { return hWindow; }
void AdDrawPolygons( unsigned long polys ) { Polygons += polys; }

#define EPSILON 0.0000001f

DWORD ConvertStr2Dno2( char* DataName )
{
    int Dno, Type, Hi, Lo;
    Type = 1;
    sscanf(DataName, "%d-%d-%d,", &Type, &Hi, &Lo);
    Dno = Type*0x10000 + Hi*0x80 + Lo%0x80;
    return Dno;
}

DWORD ConvertStr2Dno( char* DataName )
{
    int Dno, Type, Hi, Lo;
    Type = 1;
    sscanf(DataName, "%d-%d,", &Hi, &Lo);
    Dno = Type*0x10000 + Hi*0x80 + Lo%0x80;
    return Dno;
}

BOOL GetFileNameFromDno(LPSTR filename, DWORD Dno)
{
    int no = LOWORD(Dno);
    const char* dir = ffxidir;
    if      (HIWORD(Dno)==1) sprintf(filename, "%sROM\\%d\\%d.dat",  dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==2) sprintf(filename, "%sROM2\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==3) sprintf(filename, "%sROM3\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==4) sprintf(filename, "%sROM4\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==5) sprintf(filename, "%sROM5\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==6) sprintf(filename, "%sROM6\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==7) sprintf(filename, "%sROM7\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==8) sprintf(filename, "%sROM8\\%d\\%d.dat", dir, no/0x80, no%0x80);
    else if (HIWORD(Dno)==9) sprintf(filename, "%sROM9\\%d\\%d.dat", dir, no/0x80, no%0x80);
    return TRUE;
}

// カメラ更新ヘルパー
static void UpdateEyeAndView()
{
    XMVECTOR eye = XMVectorAdd(
        XMVector3TransformNormal(LoadV(g_mEyebase), LoadM(g_mEyeMat)),
        LoadV(g_mAt));
    StoreV(g_mEye, eye);
    StoreM(g_mView, XMMatrixLookAtLH(LoadV(g_mEye), LoadV(g_mAt), LoadV(g_mUp)));
}

// カメラ行列再計算ヘルパー
static void RebuildEyeMat()
{
    StoreM(g_mEyeMat,
        XMMatrixMultiply(
            XMMatrixMultiply(
                XMMatrixScaling(g_mEyeScale, g_mEyeScale, g_mEyeScale),
                XMMatrixRotationX(g_mEyeBeta)),
            XMMatrixRotationY(g_mEyeAlph)));
    UpdateEyeAndView();
}

void MoveAt( void )
{
    XMVECTOR Pos = XMVector3Normalize(LoadV(g_mAt) - LoadV(g_mEye));
    g_mAt.x += XMVectorGetX(Pos) * 3.f;
    g_mAt.z += XMVectorGetZ(Pos) * 3.f;
    UpdateEyeAndView();
}

void BackAt( void )
{
    XMVECTOR Pos = XMVector3Normalize(LoadV(g_mAt) - LoadV(g_mEye));
    g_mAt.x -= XMVectorGetX(Pos) * 3.f;
    g_mAt.z -= XMVectorGetZ(Pos) * 3.f;
    UpdateEyeAndView();
}

static void MoveToObject(int objIdx)
{
    OBJINFO* p = g_mArea.GetObjInfo(objIdx);
    if (!p) return;
    g_mAt.x =  p->mObj.fTransX;
    g_mAt.y = -p->mObj.fTransY;  // m_mRootTransform の Y反転に合わせる
    g_mAt.z =  p->mObj.fTransZ;
    UpdateEyeAndView();
}

void LoadInitFile()
{
    std::ifstream ifs(".ini");
    if (!ifs.is_open()) return;
    std::string line;
    while (std::getline(ifs, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        val.erase(val.find_last_not_of(" \t\r\n") + 1);
        val.erase(0, val.find_first_not_of(" \t\r\n"));
        if (key == "MESH_PATH") {
            strncpy(g_meshPath, val.c_str(), sizeof(g_meshPath)-1);
            strncpy(ffxidir,    val.c_str(), sizeof(ffxidir)-1);
        } else if (key == "TEX_PATH") {
            strncpy(g_texPath, val.c_str(), sizeof(g_texPath)-1);
        }
    }
}

//======================================================================
// WinMain
//======================================================================
int __stdcall WinMain( HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show )
{
    HKEY hKey;
    char tmp[512];
    GetCurrentDirectory(sizeof(tmp), tmp); g_execDir = tmp;
    ffxidir[0] = 0;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    LoadInitFile();

    ffxidir[0] = 0;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\PlayOnline\\InstallFolder", 0, KEY_READ | KEY_WOW64_32KEY, &hKey)) {
        DWORD dwData = sizeof(ffxidir), dwType = REG_SZ;
        RegQueryValueEx(hKey, "0001", NULL, &dwType, (LPBYTE)ffxidir, &dwData);
        RegCloseKey(hKey);
    }
    if (!*ffxidir) {
        MessageBox(NULL, "Please start a PC on which you are installing the FinalFantasyXI",
                   "FF XI is not installed", MB_OK);
        strcpy(ffxidir, g_execDir.c_str());
        CoUninitialize();
        return -1;
    }
    if (lstrlen(ffxidir) > 0 && ffxidir[lstrlen(ffxidir)-1] != '\\')
        lstrcat(ffxidir, "\\");

    timeBeginPeriod(1);

    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = WinProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = inst;
    wc.hIcon         = LoadIcon(inst, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = ClassName;
    if (RegisterClass(&wc) == NULL) { CoUninitialize(); return false; }

    // カスタム描画キャンバス用ウィンドウクラス登録
    WNDCLASS wc2 = {};
    wc2.style         = CS_HREDRAW | CS_VREDRAW;
    wc2.lpfnWndProc   = CanvasProc;
    wc2.hInstance     = inst;
    wc2.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc2.lpszClassName = "AreaTestCanvas";
    RegisterClass(&wc2);

    long window_w = g_mScreenWidth  + GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXDLGFRAME);
    long window_h = g_mScreenHeight + GetSystemMetrics(SM_CYEDGE) + GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);

    hWindow = CreateWindowEx(WS_EX_APPWINDOW, ClassName, AppName,
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_VISIBLE,
        GetSystemMetrics(SM_CXSCREEN)/2 - window_w/2,
        GetSystemMetrics(SM_CYSCREEN)/2 - window_h/2,
        window_w, window_h, NULL, NULL, inst, NULL);

    hDlg1 = CreateDialog((HINSTANCE)GetWindowLongPtr(hWindow, GWLP_HINSTANCE),
                         MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)Dlg1Proc);
    InvalidateRect(hDlg1, NULL, TRUE);
    SetWindowPos(hDlg1, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(hDlg1, SW_HIDE);

    if (!InitD3D())    { CoUninitialize(); return false; }
    if (!InitRender()) { CoUninitialize(); return false; }

    MSG msg;
    while (true) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (!IsDialogMessage(hDlg1, &msg)) {
                if (msg.message == WM_QUIT) break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            static DWORD cnt = 0, BeforeTime = timeGetTime();
            DWORD NowTime = timeGetTime();
            if ((NowTime - BeforeTime - 25*cnt) >= 25) cnt++;
            if (NowTime - BeforeTime >= 500) {
                char FpsStr[128];
                sprintf(FpsStr, "%s  [ [ FPS : %03d/s ] [ POS: %d %d %d [ %upolygon/sec ]",
                    AppName, FPS*2,
                    (int)g_mAt.x, (int)g_mAt.y, (int)g_mAt.z,
                    Polygons*2);
                SetWindowText(hWindow, FpsStr);
                BeforeTime = NowTime;
                Polygons = 0; FPS = 0; cnt = 0;
            }
            FPS++;
            Rendering();
        }
    }

    UnInitRender();
    ReleaseD3D();
    timeEndPeriod(1);
    CoUninitialize();
    return (int)msg.wParam;
}

//======================================================================
// カスタム描画キャンバス ヘルパー
//======================================================================
static std::string SanitizeAreaName(const char* name)
{
    std::string result;
    for (int i = 0; i < 16 && name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];
        result += (c >= 0x20 && c < 0x7F) ? (char)c : ' ';
    }
    return result.empty() ? "(empty)" : result;
}

static const int CANVAS_LINE_H = 18;

static void UpdateScrollBar(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    int totalH = (int)g_canvasLines.size() * CANVAS_LINE_H;
    int pageH  = rc.bottom - rc.top;
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = max(0, totalH - 1);
    si.nPage  = (UINT)pageH;
    si.nPos   = g_scrollPos;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    g_scrollPos = GetScrollPos(hWnd, SB_VERT);
}

static void RefreshCanvas()
{
    if (!hCanvas) return;
    g_scrollPos = 0;
    UpdateScrollBar(hCanvas);
    InvalidateRect(hCanvas, NULL, TRUE);
}

static void MoveToObjInst(int instIdx)
{
    if (instIdx < 0 || instIdx >= (int)g_objInstances.size()) return;
    g_objInstCur          = instIdx;
    g_canvasHighlightLine = g_instBaseLine + instIdx;
    if (hCanvas) {
        g_scrollPos = max(0, g_canvasHighlightLine * CANVAS_LINE_H - 50);
        UpdateScrollBar(hCanvas);
        InvalidateRect(hCanvas, NULL, TRUE);
    }
    MoveToObject(g_objInstances[instIdx]);
}

static void SetCanvasMesh(CAreaMesh* pMesh)
{
    SetHighlightEffect(nullptr);
    SetHighlightMesh(pMesh);
    g_canvasLines.clear();
    g_objInstances.clear();
    g_objInstCur          = -1;
    g_canvasHighlightLine = -1;
    if (!pMesh) { RefreshCanvas(); return; }
    char buf[160];
    g_canvasLines.push_back("[ Mesh Info ]");
    g_canvasLines.push_back("AreaName  : " + SanitizeAreaName(pMesh->GetAreaName()));
    g_canvasLines.push_back("AreaType  : " + std::string(pMesh->GetAreaType()).substr(0, 4));
    sprintf(buf, "Vertices  : %lu", pMesh->GetNumVertices()); g_canvasLines.push_back(buf);
    sprintf(buf, "Faces     : %lu", pMesh->GetNumFaces());    g_canvasLines.push_back(buf);
    XMFLOAT3 lo = pMesh->GetBoxLow(), hi = pMesh->GetBoxHigh();
    sprintf(buf, "BoxLow    : X=%8.2f  Y=%8.2f  Z=%8.2f", lo.x, lo.y, lo.z); g_canvasLines.push_back(buf);
    sprintf(buf, "BoxHigh   : X=%8.2f  Y=%8.2f  Z=%8.2f", hi.x, hi.y, hi.z); g_canvasLines.push_back(buf);
    g_canvasLines.push_back("--- Instances ---");
    g_instBaseLine = (int)g_canvasLines.size();
    int cnt = 0;
    for (int i = 0; i < g_mArea.GetNObj(); i++) {
        OBJINFO* p = g_mArea.GetObjInfo(i);
        if (p && p->pAreaMesh == pMesh) {
            sprintf(buf, "OBJ[%03d]  pos(%8.2f,%8.2f,%8.2f)", i,
                    p->mObj.fTransX, p->mObj.fTransY, p->mObj.fTransZ);
            g_canvasLines.push_back(buf);
            g_objInstances.push_back(i);
            cnt++;
        }
    }
    if (cnt == 0) g_canvasLines.push_back("  (no instances)");
    RefreshCanvas();
}

static void SetCanvasEffect(CEffect* pEff)
{
    SetHighlightMesh(nullptr);
    SetHighlightEffMdl(nullptr);
    SetHighlightEffect(pEff);
    g_canvasLines.clear();
    if (!pEff) { RefreshCanvas(); return; }
    char buf[160];
    g_canvasLines.push_back("[ Effect Info ]");
    g_canvasLines.push_back("Name      : " + pEff->m_name);
    g_canvasLines.push_back("Class     : " + pEff->m_class);
    sprintf(buf, "lifetime  : %lu ms", pEff->m_lifeTime); g_canvasLines.push_back(buf);
    sprintf(buf, "pos       : X=%8.3f  Y=%8.3f  Z=%8.3f",
            pEff->m_p01.x, pEff->m_p01.y, pEff->m_p01.z);
    g_canvasLines.push_back(buf);
    sprintf(buf, "rot  [rad]: X=%8.3f  Y=%8.3f  Z=%8.3f",
            pEff->m_r09.x, pEff->m_r09.y, pEff->m_r09.z);
    g_canvasLines.push_back(buf);
    sprintf(buf, "scale     : X=%8.3f  Y=%8.3f  Z=%8.3f",
            pEff->m_s0F.x, pEff->m_s0F.y, pEff->m_s0F.z);
    g_canvasLines.push_back(buf);
    sprintf(buf, "color RGBA: R=%.3f  G=%.3f  B=%.3f  A=%.3f",
            pEff->m_color.x, pEff->m_color.y, pEff->m_color.z, pEff->m_color.w);
    g_canvasLines.push_back(buf);
    CEffectModel* pEfm = pEff->m_pEffectModel;
    g_canvasLines.push_back("--- EffectModel ---");
    g_canvasLines.push_back("Name      : " + pEfm->m_Name);
    g_canvasLines.push_back("Type      : " + pEfm->m_type.substr(0, 4));
    sprintf(buf, "ModelType : %d",  pEfm->m_ModelType);  g_canvasLines.push_back(buf);
    sprintf(buf, "ModelNo   : %lu", pEfm->m_ModelNo);    g_canvasLines.push_back(buf);
    sprintf(buf, "ModelTotal: %lu", pEfm->m_ModelTotal); g_canvasLines.push_back(buf);
    RefreshCanvas();
}

//======================================================================
// カスタム描画キャンバス ウィンドウプロシージャ
//======================================================================
LRESULT CALLBACK CanvasProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        int y = 2 - g_scrollPos;
        for (int li = 0; li < (int)g_canvasLines.size(); li++) {
            RECT lineRc = { 4, y + li * CANVAS_LINE_H, rc.right - 4, y + (li + 1) * CANVAS_LINE_H };
            if (lineRc.bottom <= 0) continue;
            if (lineRc.top >= rc.bottom) break;
            if (li == g_canvasHighlightLine) {
                HBRUSH hBr = CreateSolidBrush(RGB(173, 216, 230));
                FillRect(hdc, &lineRc, hBr);
                DeleteObject(hBr);
            }
            DrawText(hdc, g_canvasLines[li].c_str(), -1, &lineRc, DT_LEFT | DT_SINGLELINE);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SIZE:
        UpdateScrollBar(hWnd);
        return 0;

    case WM_VSCROLL: {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        switch (LOWORD(wParam)) {
        case SB_LINEUP:     si.nPos -= CANVAS_LINE_H;  break;
        case SB_LINEDOWN:   si.nPos += CANVAS_LINE_H;  break;
        case SB_PAGEUP:     si.nPos -= (int)si.nPage;  break;
        case SB_PAGEDOWN:   si.nPos += (int)si.nPage;  break;
        case SB_THUMBTRACK: si.nPos  = si.nTrackPos;   break;
        }
        si.fMask = SIF_POS;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        g_scrollPos = GetScrollPos(hWnd, SB_VERT);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        si.nPos  -= (delta / WHEEL_DELTA) * CANVAS_LINE_H * 3;
        si.fMask  = SIF_POS;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        g_scrollPos = GetScrollPos(hWnd, SB_VERT);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//======================================================================
// ダイアログプロシージャ
//======================================================================
LRESULT CALLBACK Dlg1Proc(HWND in_hWnd, UINT in_Message, WPARAM in_wParam, LPARAM in_lParam)
{
    int  i, ComboNo, w1, w2, w3;
    char buf[128], ww[512];

    switch (in_Message) {
    case WM_INITDIALOG: {
        g_ListArea.clear();
        std::string listPath = g_execDir + "\\List\\Area.lst";
        std::ifstream ifs(listPath);
        if (ifs) {
            std::string line;
            while (std::getline(ifs, line) && g_ListArea.size() < LIST_LIMIT)
                g_ListArea.push_back(line);
        }
        for (i = 0; i < (int)g_ListArea.size(); i++) {
            int w5, w6, w7;
            sscanf(g_ListArea[i].c_str(), "%d-%d-%d,%d,%d,%d,%s", &w1,&w2,&w3,&w5,&w6,&w7,ww);
            sprintf(buf, "%d-%d-%d,%s", w1,w2,w3,ww);
            SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_INSERTSTRING, (WPARAM)i, (LPARAM)buf);
        }
        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, 0L);
        for (i = 0; i < 3; i++)
            SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListRange[i]);
        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_SETCURSEL, (WPARAM)2, 0L);
        for (i = 0; i < 3; i++)
            SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_INSERTSTRING, (WPARAM)i, (LPARAM)ListRange[i]);
        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_SETCURSEL, (WPARAM)2, 0L);
        SetFocus(GetDlgItem(in_hWnd, IDC_COMBO1));
        // カスタム描画キャンバスを生成 (ナビゲーションボタン行の下)
        {
            RECT rc = { 7, 75, 7 + 278, 75 + 217 };
            MapDialogRect(in_hWnd, &rc);
            hCanvas = CreateWindowEx(
                WS_EX_CLIENTEDGE, "AreaTestCanvas", NULL,
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL,
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                in_hWnd, NULL,
                (HINSTANCE)GetWindowLongPtr(in_hWnd, GWLP_HINSTANCE), NULL);
        }
        break;
    }
    case WM_HSCROLL:
        break;
    case WM_COMMAND:
        switch (LOWORD(in_wParam)) {
        case IDOK:
            ShowWindow(in_hWnd, SW_HIDE);
            break;
        case IDC_COMBO1:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO1), CB_GETCURSEL, 0L, 0L);
                sscanf(g_ListArea[ComboNo].c_str(), "%d-%d-%d,%f,%f,%f,%s",
                       &w1,&w2,&w3, &g_mEntry.x, &g_mEntry.y, &g_mEntry.z, ww);
                g_mArea.SetArea(ConvertStr2Dno2((char*)g_ListArea[ComboNo].c_str()));
                if (!g_mArea.LoadMAP()) return -1;
                g_mAt.x = g_mEntry.x; g_mAt.y = g_mEntry.y; g_mAt.z = g_mEntry.z;
                g_mEye = g_mAt; g_mEye.z += -5; g_mEye.y += 1.0f;
                StoreM(g_mView, XMMatrixLookAtLH(LoadV(g_mEye), LoadV(g_mAt), LoadV(g_mUp)));

                // COMBO3: AreaMesh 名一覧（ファイル出現順）
                SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_RESETCONTENT, 0, 0);
                {
                    CAreaMesh* pMesh = (CAreaMesh*)g_mArea.GetAreaMeshs().Top();
                    for (int idx = 0; pMesh; idx++, pMesh = (CAreaMesh*)pMesh->Next) {
                        sprintf(buf, "[%03d] %s", idx, SanitizeAreaName(pMesh->GetAreaName()).c_str());
                        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_ADDSTRING, 0, (LPARAM)buf);
                    }
                    SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_SETCURSEL, 0, 0);
                }

                // COMBO4: 有効な EffectModel を持つ Effect 一覧
                SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_RESETCONTENT, 0, 0);
                g_combo4Effects.clear();
                {
                    CEffect* pEff = (CEffect*)g_mArea.m_Effects.Top();
                    for (; pEff; pEff = (CEffect*)pEff->Next) {
                        if (!pEff->m_pEffectModel) continue;
                        sprintf(buf, "[%03d] %s  class:%s  -> %s",
                                (int)g_combo4Effects.size(),
                                pEff->m_name.c_str(),
                                pEff->m_class.c_str(),
                                pEff->m_pEffectModel->m_Name.c_str());
                        SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_ADDSTRING, 0, (LPARAM)buf);
                        g_combo4Effects.push_back(pEff);
                    }
                    SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_SETCURSEL, 0, 0);
                }

                // キャンバス：Effectがあれば先頭Effect、なければ先頭メッシュを表示
                if (!g_combo4Effects.empty())
                    SetCanvasEffect(g_combo4Effects[0]);
                else
                    SetCanvasMesh((CAreaMesh*)g_mArea.GetAreaMeshs().Data(0));
            }
            break;
        case IDC_COMBO2:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO2), CB_GETCURSEL, 0L, 0L);
                sscanf(ListRange[ComboNo], "%d m,%s", &w1, ww);
                g_mDispArea = (float)w1;
            }
            break;
        case IDC_COMBO3:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                int sel = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO3), CB_GETCURSEL, 0, 0);
                SetCanvasMesh((CAreaMesh*)g_mArea.GetAreaMeshs().Data(sel));
            }
            break;
        case IDC_COMBO4:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                int sel = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO4), CB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)g_combo4Effects.size())
                    SetCanvasEffect(g_combo4Effects[sel]);
            }
            break;
        case IDC_BTN_OBJ_TOP:
            MoveToObjInst(0);
            break;
        case IDC_BTN_OBJ_PREV:
            MoveToObjInst(g_objInstCur > 0 ? g_objInstCur - 1 : 0);
            break;
        case IDC_BTN_OBJ_NEXT:
            MoveToObjInst(g_objInstCur + 1 < (int)g_objInstances.size()
                          ? g_objInstCur + 1
                          : (int)g_objInstances.size() - 1);
            break;
        case IDC_BTN_OBJ_LAST:
            MoveToObjInst((int)g_objInstances.size() - 1);
            break;
        case IDC_COMBO5:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO5), CB_GETCURSEL, 0L, 0L);
                sscanf(ListRange[ComboNo], "%d m,%s", &w1, ww);
                g_mDispTree = (float)w1;
            }
            break;
        case IDC_COMBO6:
            if (HIWORD(in_wParam) == CBN_SELCHANGE) {
                ComboNo = (int)SendMessage(GetDlgItem(in_hWnd, IDC_COMBO6), CB_GETCURSEL, 0L, 0L);
                memcpy(g_mWeather, ListWeather[ComboNo], 4);
                if (!g_mArea.LoadMAP()) return -1;
            }
            break;
        }
        break;
    case WM_CLOSE:
        ShowWindow(in_hWnd, SW_HIDE);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return 0;
}

//======================================================================
// メインウィンドウプロシージャ
//======================================================================
LRESULT CALLBACK WinProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static float alpha = 0.f, beta = 0.f;
    static float Delta = 0.f, Step = 0.2f;
    static bool  lDrag = false, rDrag = false;
    static short x1 = -1, y1 = -1, x2, y2;
    static bool  s_isFullscreen = false;
    static RECT  s_savedRect    = {};
    static DWORD s_savedStyle   = 0;

    OPENFILENAME sfn;
    char szFPath[256], szFName[256], strmsg[256];
    lstrcpy(szFPath, "*.mqo");
    ZeroMemory(&sfn, sizeof(sfn));
    sfn.lStructSize   = sizeof(sfn);
    sfn.hwndOwner     = NULL;
    sfn.lpstrFile     = szFPath;
    sfn.nMaxFile      = sizeof(szFPath);
    sfn.lpstrFilter   = "MQO Format(*.mqo)\0*.mqo\0";
    sfn.nFilterIndex  = 1;
    sfn.lpstrFileTitle = szFName;
    sfn.nMaxFileTitle = sizeof(szFName);
    sfn.lpstrTitle    = "MQO Save";
    sfn.lpstrInitialDir = NULL;

    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MAXIMIZE) {
            // 最大化ボタン → F11 と同じボーダーレス全画面トグル
            SendMessage(hWnd, WM_KEYDOWN, VK_F11, 0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_GETMINMAXINFO: {
        int bw = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        int bh = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        MINMAXINFO* mm = (MINMAXINFO*)lParam;
        mm->ptMinTrackSize.x = 320 + bw;
        mm->ptMinTrackSize.y = 180 + bh;
        return 0;
    }

    case WM_SIZING: {
        const double aspect = 16.0 / 9.0;
        RECT* rc = (RECT*)lParam;
        int bw = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        int bh = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        int cw = (rc->right - rc->left) - bw;
        int ch = (rc->bottom - rc->top) - bh;

        // 左右ドラッグ（またはコーナー）は幅優先、上下ドラッグは高さ優先
        bool widthDriven = (wParam != WMSZ_TOP && wParam != WMSZ_BOTTOM);
        if (widthDriven)
            ch = (int)(cw / aspect);
        else
            cw = (int)(ch * aspect);

        if (cw < 320) { cw = 320; ch = 180; }

        int nw = cw + bw;
        int nh = ch + bh;

        switch (wParam) {
        case WMSZ_RIGHT:
        case WMSZ_BOTTOMRIGHT:
        case WMSZ_BOTTOM:
            rc->right  = rc->left + nw;
            rc->bottom = rc->top  + nh;
            break;
        case WMSZ_LEFT:
        case WMSZ_BOTTOMLEFT:
            rc->left   = rc->right - nw;
            rc->bottom = rc->top   + nh;
            break;
        case WMSZ_TOP:
        case WMSZ_TOPRIGHT:
            rc->right  = rc->left + nw;
            rc->top    = rc->bottom - nh;
            break;
        case WMSZ_TOPLEFT:
            rc->left   = rc->right  - nw;
            rc->top    = rc->bottom - nh;
            break;
        }
        return TRUE;
    }

    case WM_SIZE: {
        if (wParam != SIZE_MINIMIZED) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int nw = rc.right  - rc.left;
            int nh = rc.bottom - rc.top;
            if (nw > 0 && nh > 0 && (nw != g_mScreenWidth || nh != g_mScreenHeight)) {
                g_mScreenWidth  = nw;
                g_mScreenHeight = nh;
                ResizeD3D11(nw, nh);
            }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_MOUSEWHEEL:
        if (wParam == 0x00780000)  MoveAt();
        else if (wParam == 0xff880000) BackAt();
        break;

    case WM_MOUSEMOVE:
        x2 = LOWORD(lParam);
        y2 = HIWORD(lParam);
        if (wParam & MK_LBUTTON) {
            if (abs(x2-x1) < 20 && abs(y2-y1) < 20) {
                g_mEyeAlph += (float)(x2-x1)/(float)g_mScreenWidth*2.f*PAI;
                g_mEyeBeta += (float)(y1-y2)/(float)g_mScreenWidth*2.f*PAI;
                g_mEyeAlph = (g_mEyeAlph > PAI2)  ? (g_mEyeAlph-PAI2)  : g_mEyeAlph;
                g_mEyeAlph = (g_mEyeAlph < -PAI2) ? (g_mEyeAlph+PAI2)  : g_mEyeAlph;
                g_mEyeBeta = (g_mEyeBeta >  PAI/2.f) ? ( PAI/2.f-0.02f) : g_mEyeBeta;
                g_mEyeBeta = (g_mEyeBeta < -PAI/2.f) ? (-PAI/2.f+0.02f) : g_mEyeBeta;
                RebuildEyeMat();
            }
        } else if (wParam & MK_MBUTTON) {
            if (abs(x2-x1) < 20 && abs(y2-y1) < 20) {
                XMVECTOR Pos = XMVector3Normalize(LoadV(g_mAt) - LoadV(g_mEye));
                Pos = XMVector3TransformCoord(Pos, XMMatrixRotationY(-PAI/2.f));
                g_mAt.x += XMVectorGetX(Pos) * ((x2-x1)/10.f);
                g_mAt.z += XMVectorGetZ(Pos) * ((x2-x1)/10.f);
                g_mAt.y += (y2-y1) / 10.f;
                UpdateEyeAndView();
            }
        } else if (wParam & MK_RBUTTON) {
            if (abs(x2-x1) < 20 && abs(y2-y1) < 20) {
                g_mLightAlph += (float)(x2-x1)/(float)g_mScreenWidth*2.f*PAI;
                g_mLightBeta += (float)(y1-y2)/(float)g_mScreenWidth*2.f*PAI;
                g_mLightAlph = (g_mLightAlph > PAI2)  ? (g_mLightAlph-PAI2)  : g_mLightAlph;
                g_mLightAlph = (g_mLightAlph < -PAI2) ? (g_mLightAlph+PAI2)  : g_mLightAlph;
                g_mLightBeta = (g_mLightBeta >  PAI/2.f) ? ( PAI/2.f-0.02f) : g_mLightBeta;
                g_mLightBeta = (g_mLightBeta < -PAI/2.f) ? (-PAI/2.f+0.02f) : g_mLightBeta;
                XMMATRIX mat = XMMatrixMultiply(
                    XMMatrixRotationX(g_mLightBeta),
                    XMMatrixRotationY(g_mLightAlph));
                StoreV(g_mLightDir,
                    XMVector3Normalize(XMVector3TransformNormal(LoadV(g_mLightDirBase), mat)));
            }
        }
        x1 = x2; y1 = y2;
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_MNU_OASPD) ShowWindow(hDlg1, SW_SHOW);
        if (LOWORD(wParam) == ID_MNU_META) {
            sfn.lpstrTitle = "MQO Save";
            if (GetSaveFileName(&sfn)) {
                if (!g_mArea.saveMQO(szFPath, szFName, g_mAt.x, g_mAt.y, g_mAt.z)) {
                    wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
                    MessageBox(NULL, strmsg, "Open Save File", MB_OK|MB_ICONINFORMATION);
                }
            }
        }
        if (LOWORD(wParam) == ID_MNU_EFFT) {
            sfn.lpstrTitle = "MQO Save Effect Data";
            if (GetSaveFileName(&sfn)) {
                if (!g_mArea.saveMQO2(szFPath, szFName, g_mAt.x, g_mAt.y, g_mAt.z)) {
                    wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
                    MessageBox(NULL, strmsg, "Open Save File", MB_OK|MB_ICONINFORMATION);
                }
            }
        }
        if (LOWORD(wParam) == ID_MNU_EFF2) {
            sfn.lpstrTitle = "MQO Save EffectModel Data";
            if (GetSaveFileName(&sfn)) {
                if (!g_mArea.saveMQO3(szFPath, szFName)) {
                    wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
                    MessageBox(NULL, strmsg, "Open Save File", MB_OK|MB_ICONINFORMATION);
                }
            }
        }
        if (LOWORD(wParam) == ID_MNU_FBX) {
            sfn.lpstrTitle   = "FBX Save";
            sfn.lpstrFilter  = "FBX Format(*.fbx)\0*.fbx\0";
            sfn.lpstrDefExt  = "fbx";
            lstrcpy(szFPath, "*.fbx");
            if (GetSaveFileName(&sfn)) {
                if (!g_mArea.saveFBX(szFPath, szFName, g_mAt.x, g_mAt.y, g_mAt.z)) {
                    wsprintf(strmsg, "File %s could not be processed correctly", szFPath);
                    MessageBox(NULL, strmsg, "FBX Save", MB_OK|MB_ICONINFORMATION);
                }
            }
            sfn.lpstrFilter = "MQO Format(*.mqo)\0*.mqo\0";
            sfn.lpstrDefExt = NULL;
            lstrcpy(szFPath, "*.mqo");
        }
        if (LOWORD(wParam) == ID_MNU_EXIT) {
            if (MessageBox(NULL, "Do you really want to quit?", "Quit Program",
                           MB_YESNO|MB_ICONQUESTION) == IDYES)
                SendMessage(hWnd, WM_CLOSE, 0L, 0L);
        }
        // fall through to WM_KEYDOWN

    case WM_KEYDOWN:
        if (wParam == VK_INSERT) {
            if (g_mEyeScale <= 0.2f) break;
            g_mEyeScale -= (GetKeyState(VK_CONTROL)&0x8000) ? 0.2f : 0.02f;
            RebuildEyeMat();
        } else if (wParam == VK_DELETE) {
            g_mEyeScale += (GetKeyState(VK_CONTROL)&0x8000) ? 0.2f : 0.02f;
            RebuildEyeMat();
        } else if (wParam == VK_PRIOR) {
            g_mAt.y += (GetKeyState(VK_CONTROL)&0x8000) ? 0.5f : 0.05f;
            UpdateEyeAndView();
        } else if (wParam == VK_NEXT) {
            g_mAt.y -= (GetKeyState(VK_CONTROL)&0x8000) ? 0.5f : 0.05f;
            UpdateEyeAndView();
        } else if (wParam == VK_UP   || wParam=='W' || wParam=='w') {
            MoveAt();
        } else if (wParam == VK_DOWN  || wParam=='S' || wParam=='s') {
            BackAt();
        } else if (wParam == VK_RIGHT || wParam=='D' || wParam=='d') {
            XMVECTOR Pos = XMVector3Normalize(LoadV(g_mAt) - LoadV(g_mEye));
            Pos = XMVector3TransformCoord(Pos, XMMatrixRotationY(PAI/2.f));
            g_mAt.x += XMVectorGetX(Pos)*3.f;
            g_mAt.z += XMVectorGetZ(Pos)*3.f;
            UpdateEyeAndView();
        } else if (wParam == VK_LEFT  || wParam=='A' || wParam=='a') {
            XMVECTOR Pos = XMVector3Normalize(LoadV(g_mAt) - LoadV(g_mEye));
            Pos = XMVector3TransformCoord(Pos, XMMatrixRotationY(-PAI/2.f));
            g_mAt.x += XMVectorGetX(Pos)*3.f;
            g_mAt.z += XMVectorGetZ(Pos)*3.f;
            UpdateEyeAndView();
        } else if (wParam == VK_F11) {
            if (!s_isFullscreen) {
                s_savedStyle = GetWindowLong(hWnd, GWL_STYLE);
                GetWindowRect(hWnd, &s_savedRect);
                SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                int sw = GetSystemMetrics(SM_CXSCREEN);
                int sh = GetSystemMetrics(SM_CYSCREEN);
                SetWindowPos(hWnd, HWND_TOP, 0, 0, sw, sh, SWP_FRAMECHANGED);
                s_isFullscreen = true;
            } else {
                SetWindowLong(hWnd, GWL_STYLE, s_savedStyle);
                SetWindowPos(hWnd, nullptr,
                    s_savedRect.left, s_savedRect.top,
                    s_savedRect.right  - s_savedRect.left,
                    s_savedRect.bottom - s_savedRect.top,
                    SWP_FRAMECHANGED | SWP_NOZORDER);
                s_isFullscreen = false;
            }
        }
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
