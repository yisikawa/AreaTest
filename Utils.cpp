#include "Utils.h"
#include <string>
#include <algorithm>
#include <stdio.h>


//======================================================================
// 文字列操作
//======================================================================
int Trim(char *s) {
	if (s == NULL) return -1;
	std::string str(s);
	size_t start = str.find_first_not_of(' ');
	if (start == std::string::npos) {
		s[0] = '\0';
		return 0;
	}
	size_t end = str.find_last_not_of(' ');
	str = str.substr(start, end - start + 1);
	strcpy(s, str.c_str());
	return (int)str.length();
}

char* strrstr(const char *string, const char *pattern) {
	if (!string || !pattern) return NULL;
	std::string s(string);
	size_t pos = s.rfind(pattern);
	if (pos == std::string::npos) return NULL;
	return (char*)(string + pos);
}

//======================================================================
// 数値計算
//======================================================================
float Min4(float v1, float v2, float v3, float v4) {
	float val = (std::min)(v1, v2);
	val = (std::min)(val, v3);
	return (std::min)(val, v4);
}

float Max4(float v1, float v2, float v3, float v4) {
	float val = (std::max)(v1, v2);
	val = (std::max)(val, v3);
	return (std::max)(val, v4);
}

float Max5(float v1, float v2, float v3, float v4, float v5) {
	float val = (std::max)(v1, v2);
	val = (std::max)(val, v3);
	val = (std::max)(val, v4);
	return (std::max)(val, v5);
}

//======================================================================
// パス変換
//======================================================================
bool convert_path(char* src, const char* base) {
	static const char* search_term = "ROM";
	const char* relative_path = strstr(src, search_term);
	if (relative_path != NULL) {
		char buf[256];
		snprintf(buf, sizeof(buf), "%s%s", base, relative_path);
		strcpy(src, buf);
		return true;
	}
	return false;
}

//======================================================================
// 行列・3D演算
//======================================================================
BOOL IsMirrorMatrix(const D3DXMATRIX* pMat) {
	FLOAT det = D3DXMatrixDeterminant(pMat);
	return (det < 0.0f);
}

D3DXVECTOR3* ComputeFaceNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV0, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2) {
	D3DXVECTOR3 edge1 = *pV1 - *pV0;
	D3DXVECTOR3 edge2 = *pV2 - *pV0;
	D3DXVec3Cross(pOut, &edge1, &edge2);
	D3DXVec3Normalize(pOut, pOut);
	return pOut;
}
