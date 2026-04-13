
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>
#include <commctrl.h>
#include <string>
#include "resource.h"

//======================================================================
// ENCODING
//======================================================================
// UTF-8リテラルをWindows ANSI APIに渡すための変換
std::string Utf8ToSjis(const std::string& utf8);
#define U8TOA(s) Utf8ToSjis(s).c_str()

//======================================================================
// PROTOTYPE
//======================================================================
long GetScreenWidth( void );
long GetScreenHeight( void );
HWND GetWindow( void );
void AdDrawPolygons( unsigned long polys );

