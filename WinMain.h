
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

//======================================================================
// PROTOTYPE
//======================================================================
long GetScreenWidth( void );
long GetScreenHeight( void );
HWND GetWindow( void );
void AdDrawPolygons( unsigned long polys );

