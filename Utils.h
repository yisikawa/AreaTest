#pragma once

#include <windows.h>
#include <DirectXMath.h>
using namespace DirectX;

// 文字列操作
int Trim(char *s);
char* strrstr(const char *string, const char *pattern);

// 数値計算
float Min4(float v1, float v2, float v3, float v4);
float Max4(float v1, float v2, float v3, float v4);
float Max5(float v1, float v2, float v3, float v4, float v5);

// パス変換
bool convert_path(char* src, const char* base);
