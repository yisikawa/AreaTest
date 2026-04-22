#pragma once

#include <d3d9.h>
#include <d3dx9.h>

// 文字列操作
int Trim(char *s);
char* strrstr(const char *string, const char *pattern);

// 数値計算
float Min4(float v1, float v2, float v3, float v4);
float Max4(float v1, float v2, float v3, float v4);
float Max5(float v1, float v2, float v3, float v4, float v5);

// パス変換
bool convert_path(char* src, const char* base);

// 行列・3D演算
BOOL IsMirrorMatrix(const D3DXMATRIX* pMat);
D3DXVECTOR3* ComputeFaceNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV0, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2);

// デコード処理
void DecodeMMB(BYTE* p);
void DecodeMMBSub(BYTE* p);
void DecodeMZB(BYTE* p);
