
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <windows.h>
#include <commctrl.h>
#include <string>
#include "resource.h"

//======================================================================
// PROTOTYPE
//======================================================================
long GetScreenWidth( void );
long GetScreenHeight( void );
HWND GetWindow( void );
void AdDrawPolygons( unsigned long polys );
