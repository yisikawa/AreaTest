// INCLUDE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "Render.h"
#include "Area.h"
#include <list>

using namespace std;
// Function
BOOL GetFileNameFromDno(LPSTR filename,DWORD dwID);
DWORD	ConvertStr2Dno( char* DataName );
HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
// DEFINE
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(3.1415926535897932384626433832795f*2.0f)
static const D3DXMATRIX matrixMirrorX(-1.0f,0,0,0,  0, 1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorY( 1.0f,0,0,0,  0,-1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorZ( 1.0f,0,0,0,  0, 1.0f,0,0,  0,0,-1.0f,0,  0,0,0,1.0f);
// �O���[�o��
extern	BOOL		g_mIsUseSoftware;
extern	D3DLIGHT9	g_mLight,g_mLightbase;
extern	D3DXMATRIX	g_mProjection, g_mView;
extern	D3DXVECTOR3	g_mAt,g_mEye,g_mUp;
extern	float		g_mTime;
extern	float		g_mDispArea;
extern	float		g_mDispTree;
extern	D3DLIGHT9	g_mLight,g_mLightbase;
extern	D3DXVECTOR3	g_mEntry;
extern	int			g_mAreaBright;
extern	char		g_mWeather[];
extern	float		g_mLightDist;
extern	D3DXVECTOR3	g_mLightPosition;
extern	D3DXMATRIX	g_mViewLight;					// ���C�g���猩���ꍇ�̃r���[�}�g���b�N�X


int Trim(char *s) {
	int i;
	int count = 0;

	/* ��|�C���^��? */
	if (s == NULL) { /* yes */
		return -1;
	}

	/* �����񒷂��擾���� */
	i = strlen(s);

	/* �������珇�ɋ󔒂łȂ��ʒu��T�� */
	while (--i >= 0 && s[i] == ' ') count++;

	/* �I�[�i��������t������ */
	s[i + 1] = '\0';

	/* �擪���珇�ɋ󔒂łȂ��ʒu��T�� */
	i = 0;
	while (s[i] != '\0' && s[i] == ' ') i++;
	strcpy(s, &s[i]);

	return i + count;
}

char * // ������ւ̃|�C���^
strrstr
(
const char *string, // �����Ώە�����
const char *pattern // �������镶����
)
{
	// ������I�[�ɒB����܂Ō������J��Ԃ��B
	const char *last = NULL;
	{for (const char *p = string; NULL != (p = strstr(p, pattern)); ++p)
	{
		last = p;
		if ('\0' == *p)
			return (char *)last;
	}}
	return (char *)last;
}//strrstr


// ���_�t�H�[�}�b�g
D3DVERTEXELEMENT9 VSFormat[] = 
{
// Area Mesh Stream
    { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0}, 
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   0}, 
    { 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_COLOR,	0}, 
    { 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD, 0}, 
    D3DDECL_END()
};


static char *pVertexShaders[3] =
{
//------1--�G���A�\���p-------------------------------------------------- 
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG�p�I�t�Z�b�g	    \n"
		"def	c[1],		0.0f, 2.0f, 1.0f, 2.0f	;	Phong�萔					\n"
		"def	c[2],		0.3f, 0.3f, 0.3f, 0.3f	;	�X�y�L�����[�F				\n"
		"def	c[3],		1.0f, 0.0f, 3.0f, 765.01f	;	�萔					\n"
		";--------------------------------------------------------------------------\n"
		"; ���W�ϊ�																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x3	r5,			v0,			c[10]										\n"
		"m3x3	r6,			v1,			c[10]										\n"
		"; Normalize																\n"
		"dp3	r6.w,		r6,			r6											\n"
		"rsq	r6.w,		r6.w													\n"
		"mul	r6,			r6,			r6.w										\n"
		";--------------------------------------------------------------------------\n"
		"; w = 1.0 ��																\n"
		";--------------------------------------------------------------------------\n"
		"mov	r5.w,		v0.w													\n"
//		"mov	r5.w,		c[3].x													\n"
		"mov	r6.w,		c[3].x													\n"
		";--------------------------------------------------------------------------\n"
		"; ���W�ϊ�																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	oPos,		r5,		c[6]											\n"
		";--------------------------------------------------------------------------\n"
		"; ���b�V���̃e�N�X�`���[													\n"
		";--------------------------------------------------------------------------\n"
		"mov	oT0.xy,		v3.xy													\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																		\n"
		";--------------------------------------------------------------------------\n"
		"add	r1,			c[5],	-r5												\n"
		"dp3	r1.w,		r1,			r1											\n"
		"rsq	r0.w,		r1.w													\n"
		"rcp	r0.w,		r0.w				; ���_����̋���					\n"
		"add	r0.w,		c[0].w,		-r0.w	; �����@1000 - ����					\n"
		"mul	oFog,		c[0].y,		r0.w	; �����@X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; ���C�g																	\n"
		";--------------------------------------------------------------------------\n"
		";���_���璸�_�̕���e														\n"
		"rsq	r1.w,		r1.w													\n"
		"mul	r1,			r1,			r1.w	; r1 = e = ���K��(���_-���_�j		\n"
		";Phong																		\n"
		"dp3	r2.w,		c[4],		r6		; (l.n)								\n"
		"dp3	r2.x,		c[4],		r1		; (l,e)								\n"
		"dp3	r2.y,		r6,			r1		; (n,e)								\n"
		"																			\n"
		"mul	r2.z,		r2.w,		r2.y			; r2.z = (l,n)(n,e)			\n"
		"mad	r2.z,		c[1].y,		r2.z,	-r2.x	; r2.z = 2(l,n)(n,e)-(l,e)	\n"
		"max	r2.z,		r2.z,		c[1].x			; ���̒l���J�b�g			\n"
		"mov	r2.w,		c[1].w						; 2��̂��߂̃p�����[�^		\n"
		"lit	r2.z,		r2.zzww						; r2.z = r2.z ^r2.w			\n"
		"mul	r3,			c[2],		r2.z			; ��߷�װ�̐F������		\n"
		"																			\n"
		"dp4	r2.z,		c[4],		r6				; (l,n)+ambient				\n"
		"mad	oD0,		v2,			r2.z,	r3		; �����o�[�g diffuse		\n"
		"mov	oD0.a,		v2.a						; alpha = model diffuse.a	\n"
	},
//------2--effect�\���p---
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
//		"def	c[0],		0.005f, 0.001f, 500.f, 1000.0f	; FOG�p�I�t�Z�b�g	    \n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG�p�I�t�Z�b�g	    \n"
		";--------------------------------------------------------------------------\n"
		"; ���W�ϊ�																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	r0,			v0,		c[3]											\n"
//		"mov	r0.w,		v0.w													\n"
		"mov	oPos,		r0														\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																	\n"
		";--------------------------------------------------------------------------\n"
		"dp3	r1.w,		r0,			r0											\n"
		"rsq	r0.w,		r1.w													\n"
		"mul	r0.w,		r0.w,		r1.w	; ���_����̋���					\n"
		"add	r0.w,		c[0].w,		-r0.w	; �����@500 - ����						\n"
		"mul	oFog,		c[0].y,		r0.w	; �����@X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; ���b�V���̃e�N�X�`���[													\n"
		";--------------------------------------------------------------------------\n"
		"mov	r1.xy,		v3.xy													\n"
		"add	oT0.xy,		r1.xy,		c[1].xy										\n"
		"																			\n"
		"mov	oD0,		c[2]						; �����o�[�g diffuse		\n"
		"mov	oD0.a,		v2.a						; alpha = model diffuse.a	\n"
	},
//----3----Weather�\���p---
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
//		"def	c[0],		0.005f, 0.001f, 500.f, 1000.0f	; FOG�p�I�t�Z�b�g	    \n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG�p�I�t�Z�b�g	    \n"
		";--------------------------------------------------------------------------\n"
		"; ���W�ϊ�																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	r0,			v0,		c[2]											\n"
		"mov	oPos,		r0														\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																	\n"
		";--------------------------------------------------------------------------\n"
		"dp3	r1.w,		r0,			r0											\n"
		"rsq	r0.w,		r1.w													\n"
		"mul	r0.w,		r0.w,		r1.w	; ���_����̋���					\n"
		"add	r0.w,		c[0].w,		-r0.w	; �����@500 - ����						\n"
		"mul	oFog,		c[0].y,		r0.w	; �����@X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; ���b�V���̃e�N�X�`���[													\n"
		";--------------------------------------------------------------------------\n"
		"mov	r1.xy,		v3.xy													\n"
		"add	oT0.xy,		r1.xy,		c[1].xy										\n"
		"																			\n"
		"mov	oD0,		v2							; alpha = model diffuse.a	\n"
	},
};


float Min4( float v1, float v2, float v3, float v4 )
{
	float val;

	val = min( v1, v2 );
	val = min( val, v3 );
	val = min( val, v4 );
	return val;
}

float Max4( float v1, float v2, float v3, float v4 )
{
	float val;

	val = max( v1, v2 );
	val = max( val, v3 );
	val = max( val, v4 );
	return val;
}

float Max5( float v1, float v2, float v3, float v4,float v5 )
{
	float val;

	val = max( v1, v2 );
	val = max( val, v3 );
	val = max( val, v4 );
	val = max( val, v5 );
	return val;
}

//		�R���X�g���N�^
CStream::CStream()
{
	m_pTexture		= NULL;
	m_PrimitiveType	= D3DPT_TRIANGLELIST;
	m_IndexStart	= 0;
	m_FaceCount		= 0;
	m_AlphaFlag		= 0;
	m_StencilFlag	= false;
}

//		�f�X�g���N�^
CStream::~CStream()
{
}

//		�C���f�b�N�X�f�[�^�ݒ�
void CStream::SetMeshAttr( D3DPRIMITIVETYPE PrimitiveType, unsigned long index_start, unsigned long face_count )
{
	m_PrimitiveType	= PrimitiveType;
	m_IndexStart	= index_start;
	m_FaceCount		= face_count;
}

//		�v���~�e�B�u�^�C�v�擾
D3DPRIMITIVETYPE CStream::GetPrimitiveType( void )
{
	return m_PrimitiveType;
}

//		�C���f�b�N�X�J�n�ʒu�擾
unsigned long CStream::GetIndexStart( void )
{
	return m_IndexStart;
}

//		�C���f�b�N�X�g�p���擾
unsigned long CStream::GetFaceCount( void )
{
	return m_FaceCount;
}

//		�R���X�g���N�^
CTexture::CTexture()
{
	m_pTexture		= NULL;
}

//		�f�X�g���N�^
CTexture::~CTexture()
{
	SAFE_RELEASE( m_pTexture );
}

//		�e�N�X�`���ݒ�
void CTexture::SetTexture( IDirect3DTexture9 *pTex )
{
	SAFE_RELEASE( m_pTexture );
	m_pTexture = pTex;
	if ( m_pTexture != NULL ) m_pTexture->AddRef();
}

//		�e�N�X�`���擾
IDirect3DTexture9 *CTexture::GetTexture( void )
{
	return m_pTexture;
}

//		�R���X�g���N�^
CAreaMesh::CAreaMesh()
{
	m_lpVB					= NULL;
	m_lpIB					= NULL;
	m_NumIndex = m_NumVertices = m_NumFaces = m_VBSize = m_IBSize = m_FVF = 0;
}

//		�f�X�g���N�^
CAreaMesh::~CAreaMesh()
{
	m_LStreams.clear();
}



//		Area���b�V���̐���
HRESULT CAreaMesh::LoadAreaMesh( char *pFile, CArea *pArea, unsigned long FVF )
{
#pragma pack(push,2)
	typedef struct _D3DTEXVERTEX
	{
		D3DXVECTOR3	v;	//���W
		D3DXVECTOR3	n;	//�@���x�N�g��
		DWORD		color;     //�F
		float		tu,tv;     // UV���W
	} D3DTEXVERTEX;

	typedef	struct _AREAMESHBOX
	{
		float		x1,x2,y1,y2,z1,z2;
	} AREAMESHBOX;
#pragma pack(pop)

	HRESULT			hr						= D3D_OK; 
	int				CountIndex =0,NumFaces=0,NumVertices=0,NumIndex = 0;
	D3DTEXVERTEX	*pV,*pVertex;
	AREAMESHBOX		*pAreaMeshBox;
	D3DXVECTOR3		BoxXYZ;
	char			TexName[18];
	CTexture		*pTexture;
	WORD			*pIndex,*pIdx;
	int				NumElement;
	int				Level;
	char			*Pos;
	int				nVertex,nVerReal;
	int				nIndex,NextPos;
	short			AlphaFlag=0;
	unsigned char	aFlag,bFlag;

	// �G���A�f�[�^�̔���

	aFlag = *pFile;bFlag = *(pFile+4);
	pFile += 16;
//	if( *(pFile+4)!=1 ) return -1;
	// �ʁA���_�̃J�E���g�̐���
	Pos		=	pFile + 0x020;
	Level		=	*(int*)Pos;
	if( Level > 64 ) {
		return -1;
	} else if( Level > 16 ) {
		Pos += Level*4;
	} else if( Level >=2 && Level <= 16 ) {
		Pos += 16*4;
	} else if( Level ==1 ) {
		Pos += 32;
	} else {
		return -1;
	}
	while( Level ){
		NumElement = *(int*)Pos;
		if( NumElement > 0xffff || NumElement<0 ) return -1;
		Pos += 32;
		for( int i=0; i<NumElement ; i++ ){
			nVertex     = (int)*(short*)(Pos+16);
			nVerReal	= (int)*(int*)(Pos+16);
			if( nVertex > 0xffff || nVertex<0 ) return -1;
			nIndex		= (*(int*)(Pos+16+4+nVertex*36));
			if( nIndex > 0xffff || nIndex < 0 ) return -1;
			NextPos = 16  +    4+ nVertex*36+    4+ nIndex*2;
			NextPos = 4*((NextPos+3)/4);
			NumVertices	+=	nVertex;
			NumFaces	+=	nIndex-2;
			NumIndex    +=  nIndex;
			Pos += NextPos;
		}
		Level--;
	}
	m_NumVertices = NumVertices;
	m_NumFaces = NumFaces;
	m_NumIndex = NumIndex;
	m_FVF = FVF;
	m_VBSize = sizeof(D3DTEXVERTEX)*NumVertices;
	m_IBSize = NumIndex*sizeof(WORD);
	hr = CreateIB( &m_lpIB, NumIndex*sizeof(WORD), D3DUSAGE_WRITEONLY );
	if FAILED( hr ) return hr;
	hr = CreateVB( &m_lpVB, NumVertices*sizeof(D3DTEXVERTEX), D3DUSAGE_WRITEONLY, FVF );
	if FAILED( hr ) return hr;
	if( FAILED( m_lpIB->Lock( 0,                 // �o�b�t�@�̍ŏ�����f�[�^���i�[����B
						m_IBSize, // ���[�h����f�[�^�̃T�C�Y�B
						(void**)&pIndex, // �Ԃ����C���f�b�N�X �f�[�^�B
						D3DLOCK_DISCARD ) ) )            // �f�t�H���g �t���O�����b�N�ɑ���B
		return E_FAIL;
	if( FAILED( m_lpVB->Lock( 0,                 // �o�b�t�@�̍ŏ�����f�[�^���i�[����B
						m_VBSize, // ���[�h����f�[�^�̃T�C�Y�B
						(void**)&pV, // �Ԃ����C���f�b�N�X �f�[�^�B
						D3DLOCK_DISCARD ) ) )            // �f�t�H���g �t���O�����b�N�ɑ���B
		return E_FAIL;
	// �ʁA���_�̊i�[
	D3DXVECTOR3	Low( 65535.f,65535.f,65535.f),High( -65535.f,-65535.f,-65535.f);
	CountIndex = NumFaces = NumVertices = 0;
	Pos		=	pFile + 0x20;
	Level	=	*(int*)Pos;
	if( Level > 64 ) {
		return -1;
	} else if( Level > 16 ) {
		Pos += Level*4;
	} else if( Level >=2 && Level <= 16 ) {
		Pos += 16*4;
	} else if( Level ==1 ) {
		pAreaMeshBox = (AREAMESHBOX*)(Pos+4);
		BoxXYZ.x = pAreaMeshBox->x1; BoxXYZ.y = pAreaMeshBox->y1; BoxXYZ.z = pAreaMeshBox->z1;
		SetBoxLow( BoxXYZ );
		BoxXYZ.x = pAreaMeshBox->x2; BoxXYZ.y = pAreaMeshBox->y2; BoxXYZ.z = pAreaMeshBox->z2;
		SetBoxHigh( BoxXYZ );
		if( pAreaMeshBox->x1>pAreaMeshBox->x2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		if( pAreaMeshBox->z1>pAreaMeshBox->z2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		if( pAreaMeshBox->y1>pAreaMeshBox->y2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		Pos += 32; // �S�̂́@�v�f��:i4,����xyz:f,���xyz:f,��:i4 
	} else {
		return -1;
	}
	while( Level ){
		NumElement = *(int*)Pos;
		if( NumElement > 0xffff || NumElement<0 ) return -1;
		pAreaMeshBox = (AREAMESHBOX*)(Pos+4);
		Pos += 32; // ���x������Box�@�v�f��:i4,����xyz:f,���xyz:f,��:i4
		for( int i=0; i<NumElement ; i++ ){
			strncpy(TexName,Pos,16);
			pTexture = (CTexture*)pArea->m_Textures.Top();
			int texNo = 0;
			while ( pTexture != NULL ) {
				if( !memcmp(TexName,pTexture->GetTexName(),16) ){
					break;
				}
				pTexture = (CTexture*)pTexture->Next;
				texNo++;
			}
			nVertex     = (int)*(short*)(Pos+16);
			nVerReal	= (int)*(int*)(Pos+16);
			AlphaFlag = (nVerReal>>28)&0x0f;
			if( nVertex > 0xffff || nVertex<0 ) return -1;
			nIndex		= (*(int*)(Pos+16+4+nVertex*36));
			if( nIndex > 0xffff || nIndex < 0 ) return -1;
			NextPos = 16  +    4+ nVertex*36+    4+ nIndex*2;
			NextPos = 4*((NextPos+3)/4);
			CStream		tStream;
			tStream.SetMeshAttr(D3DPT_TRIANGLESTRIP, CountIndex, nIndex-2 );
			tStream.SetpTexture(pTexture);
			tStream.m_TexNo = texNo;
			tStream.SetAlphaFlag( (short)AlphaFlag );
			pVertex = (D3DTEXVERTEX*)(Pos+16+4);
			tStream.SetAlphaFlag(AlphaFlag);
			tStream.m_vertNum = nVertex;
			tStream.m_faceNum = nIndex - 2;
			for( int j=0 ; j<nVertex ; j++ ) {
				Low.x = Low.x>pVertex[j].v.x?pVertex[j].v.x:Low.x;
				Low.y = Low.y>pVertex[j].v.y?pVertex[j].v.y:Low.y;
				Low.z = Low.z>pVertex[j].v.z?pVertex[j].v.z:Low.z;
				High.x = High.x<pVertex[j].v.x?pVertex[j].v.x:High.x;
				High.y = High.y<pVertex[j].v.y?pVertex[j].v.y:High.y;
				High.z = High.z<pVertex[j].v.z?pVertex[j].v.z:High.z;
			}
			if( aFlag==0x5f ) tStream.SetStencilFlag(true);
			pIdx = new WORD[nIndex];
			memcpy((char*)pIdx,(Pos+16+4+nVertex*36+4),sizeof(WORD)*nIndex);
			for( int j=0 ; j<nIndex ; j++ ) {
				pIdx[j] += NumVertices;
			}
			memcpy((char*)pV,(char*)pVertex,sizeof(D3DTEXVERTEX)*nVertex);
			pV += nVertex;
			memcpy((char*)pIndex,(char*)pIdx,sizeof(WORD)*nIndex);
			pIndex += nIndex;
//			SAFE_DELETES( pVertex );
			SAFE_DELETES( pIdx );
			m_LStreams.push_back( tStream );
			NumVertices += nVertex;
			NumFaces	+= nIndex -2;
			CountIndex	+= nIndex;
			Pos += NextPos;
		}
		Level--;
	}
	if( FAILED( hr = m_lpVB->Unlock() ) ) {
		return hr;
	}
	if( FAILED( hr = m_lpIB->Unlock() ) ) {
		return hr;
	}
	D3DXVECTOR3	TempLow  = GetBoxLow();
	D3DXVECTOR3	TempHigh  = GetBoxHigh();
	if( Low != TempLow )
		SetBoxLow( Low );
	if( High != TempHigh )
		SetBoxHigh( High );
	return hr;
}

//		�R���X�g���N�^
CArea::CArea()
{
	m_mArea				= 0;
	m_VertexFormat			= NULL;
	m_VertexSize			= 0;
	m_hVertexShader			= NULL;
	m_Textures.Init();
	D3DXMatrixIdentity( &m_mRootTransform );
	//m_mRootTransform		*= matrixMirrorY;
	//m_mRootTransform *= matrixMirrorZ;
	D3DXMatrixRotationZ(&m_mRootTransform,PAI);
	m_pObjInfo = NULL;
	m_nObj					= 0;

	m_AreaMeshs.Init();
	m_EffMeshs.Init();
}

//		�f�X�g���N�^
CArea::~CArea()
{
	m_Textures.Release();
	CAreaMesh *pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
	while( pAreaMesh ) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_AreaMeshs.Release();
	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
	while (pAreaMesh) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_EffMeshs.Release();
	CTexture* pTexture = (CTexture*)m_Textures.Top();
	while ( pTexture != NULL ) {
		pTexture->~CTexture();
		pTexture = (CTexture*)pTexture->Next;
	}
	m_Textures.Release();
	SAFE_DELETES( m_pObjInfo );
}

//		�f�[�^�̏�����
void	CArea::InitData(void)
{
	m_Textures.Release();
	CAreaMesh *pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
	while( pAreaMesh ) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_AreaMeshs.Release();
	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
	while (pAreaMesh) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_EffMeshs.Release();
	CTexture* pTexture = (CTexture*)m_Textures.Top();
	while ( pTexture != NULL ) {
		pTexture->~CTexture();
		pTexture = (CTexture*)pTexture->Next;
	}
	m_Textures.Release();
	SAFE_DELETES( m_pObjInfo );
	m_pObjInfo		= NULL;
	m_nObj			= 0;
}

//		�e�N�X�`���̓ǂݍ���
HRESULT CArea::LoadTextureFromFile( char *FileName  )
{
	HRESULT hr							= S_OK;

	// �t�@�C�����������Ɏ�荞��
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}
	// �e�N�X�`���̓ǂݍ���
	int			type,pos=0,next;
	while( pos<dwSize ) {
		next = *((int*)(pdat+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>dwSize ) break;
		type = *((int*)(pdat+pos+4));type &=0x7f;
		switch( type ) {
		case 0x20 :
			CTexture *pTexture;
			pTexture = new CTexture;
			m_Textures.InsertEnd( pTexture );
			// �e�N�X�`��
			char	TexName[18];
			strncpy(TexName,pdat+pos+16+1,16);TexName[16]='\0';
			pTexture->SetTexName(TexName);
			LPDIRECT3DTEXTURE9 pTex;
			UINT xx,yy;
			xx = *(UINT*)(pdat+pos+33+4);
			yy = *(UINT*)(pdat+pos+33+8);
			D3DLOCKED_RECT rc;
			if( *(DWORD*)(pdat+pos+33+0x28) == 'DXT3' ){
				hr = GetDevice()->CreateTexture(xx,yy,0,0 ,D3DFMT_DXT3,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) break;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
				  CopyMemory(rc.pBits,pdat+pos+33+0x28+12,(xx/4) * (yy/4) * 16 );
				  pTex->UnlockRect(0);
				}
			} else if( *(DWORD*)(pdat+pos+33+0x28) == 'DXT1' ){
				hr = GetDevice()->CreateTexture(xx,yy,0,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) break;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
				  CopyMemory(rc.pBits,pdat+pos+33+0x28+12,(xx/4) * (yy/4) * 8  );
				  pTex->UnlockRect(0);
				}
			} else {
				hr = GetDevice()->CreateTexture(xx,yy,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) return NULL;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
					for( DWORD jy=0; jy<yy; jy++ ){
						for( DWORD jx=0; jx<xx; jx++ ){
							DWORD *pp  = (DWORD *)rc.pBits;
							BYTE  *idx = (BYTE  *)(pdat+pos+33+0x28+0x400);
							DWORD *pal = (DWORD *)(pdat+pos+33+0x28);
							pp[(yy-jy-1)*xx+jx] = pal[idx[jy*xx+jx]];
						}
					}
				}
				pTex->UnlockRect(0);
			}
			pTexture->SetTexture( pTex );
			SAFE_RELEASE( pTex );
			break;
		}
		pos+=next;
	}
	// �I��
	delete pdat;
	return hr;
}

//		MMB�̃f�R�[�hsub
void CArea::DecodeMMBSub(BYTE *p)
{
	if(p[6] == 0xFF && p[7] == 0xFF)	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key1 = p[5] ^ 0xF0;
		DWORD key2 = key_table2[key1] ;

		DWORD decode_count = ((decode_length - 8) & ~0xf) / 2;

		DWORD *data1 = (DWORD *)(p + 8 + 0);
		DWORD *data2 = (DWORD *)(p + 8 + decode_count);
		for(DWORD pos = 0; pos < decode_count; pos += 8)
		{
			if(key2 & 1)
			{
				DWORD tmp;

				tmp = data1[0];
				data1[0] = data2[0];
				data2[0] = tmp;

				tmp = data1[1];
				data1[1] = data2[1];
				data2[1] = tmp;
			}
			key1 += 9;
			key2 += key1;
			data1 += 2;
			data2 += 2;
		}
	}
}

//		MMB�̃f�R�[�h
void CArea::DecodeMMB(BYTE*p)
{
	if(p[3] >= 5)
	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key = key_table[p[5] ^ 0xF0];
		int key_counter = 0;

		for(int pos = 8; pos < decode_length; pos++)
		{
			DWORD x = ((key & 0xFF) << 8) | (key & 0xFF);
			key += ++key_counter;

			p[pos] ^= (x >> (key & 7));
			key += ++key_counter;
		}
	}
	DecodeMMBSub(p);
}

//		MZB�̃f�R�[�h
void CArea::DecodeMZB(BYTE* p)
{
	if (p[3] >= 0x1B)
	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key = key_table[p[7] ^ 0xFF];
		int key_counter = 0;

		for (int pos = 8; pos < decode_length; )
		{
			int xor_length = ((key >> 4) & 7) + 16;

			if ((key & 1) && (pos + xor_length < decode_length))
			{
				for (int i = 0; i < xor_length; i++)
	 			{
					p[pos+i] ^= 0xFF;
				}
			}
			key += ++key_counter;
			pos += xor_length;
		}
		int node_count = (p[4] << 0) | (p[5] << 8) | (p[6] << 16);
		TEMPOBJINFO *node = (TEMPOBJINFO *)(p+32);
		for(int i = 0; i < node_count; i++)
		{
			for(int i = 0; i < 16; i++)
			{
				node->id[i] ^= 0x55;
			}
			node++;
		}
	}
}

//		���_�V�F�[�_�[�쐬
bool CArea::CreateVertexShader( void )
{
	HRESULT hr;
	ID3DXBuffer *pShader = NULL;

    // �V�F�[�f�B���O�p�o�[�e�b�N�X�V�F�[�_�[�쐬
    hr = GetDevice()->CreateVertexDeclaration( VSFormat, &m_VertexFormat );
    if( hr ) return false;
	// �V�F�[�_�[�̃A�Z���u��
	hr = D3DXAssembleShader( pVertexShaders[0],strlen(pVertexShaders[0]),0,NULL,NULL,&pShader,NULL );
	if SUCCEEDED( hr ){
		// �V�F�[�_�[����
		hr = GetDevice()->CreateVertexShader((DWORD*)pShader->GetBufferPointer(),&m_hVertexShader );
		pShader->Release();
	}
	return SUCCEEDED( hr );
}


//		�G���A���b�V���̓ǂݍ���
HRESULT CArea::LoadAreaFromFile( char *FileName, unsigned long FVF )
{
	char			MeshName[20];
	D3DXMATRIX		AreaMatrix,TempMatrix;
	TEMPOBJINFO		*pTobj;
	CAreaMesh		*pAreaMesh;
	HRESULT			hr			=	S_OK;
	unsigned long	cnt;
	char			*pFileBuf	= NULL;
	int				mFileSize	=	0;
	D3DXVECTOR3		BL,BH,BM1,BM2;

	// �t�@�C�����������Ɏ�荞��
	HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		mFileSize = GetFileSize(hFile,NULL);
	    pFileBuf = new char[mFileSize];
	    ReadFile(hFile,pFileBuf,mFileSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}
	// �G���A���b�V���̃C���t�H���[�V�����ǂݍ���
	int			mzbcnt=0,i,type,pos=0,next;
	while( pos<mFileSize ) {
		next = *((int*)(pFileBuf+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>mFileSize ) break;
		type = *((int*)(pFileBuf+pos+4));type &=0x7f;
		switch( type ) {
		case 0x1c : // AreaMesh
			DecodeMZB((BYTE*)&pFileBuf[pos+16]);
			if( mzbcnt==0 ) {
				m_nObj = (*(int*)(pFileBuf+pos+4+16) )&0xffffff;
				m_pObjInfo = new OBJINFO[ m_nObj ];
				pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
				for( i=0 ; i<m_nObj ; i++,pTobj++ ) {
					memcpy((char*)&m_pObjInfo[i],(char*)pTobj,sizeof(TEMPOBJINFO));
				}
			} else {
				int nObj = (*(int*)(pFileBuf+pos+4+16) )&0xffffff;
				OBJINFO *pObjInfo = new OBJINFO[ m_nObj + nObj];
				pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
				memcpy((char*)pObjInfo,(char*)m_pObjInfo,sizeof(OBJINFO)*m_nObj);
				for( i=0 ; i<nObj ; i++,pTobj++ ) {
					memcpy((char*)(pObjInfo+m_nObj+i),(char*)pTobj,sizeof(TEMPOBJINFO));
				}
				SAFE_DELETES( m_pObjInfo );
				m_pObjInfo = pObjInfo;
				m_nObj += nObj;
			}
			mzbcnt++;
			break;
		}
		pos+=next;
	}
	// �G���A���b�V���̃f�[�^�ǂݍ���
	pos=0;
	while( pos<mFileSize ) {
		next = *((int*)(pFileBuf+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>mFileSize ) break;
		type = *((int*)(pFileBuf+pos+4));type &=0x7f;
		switch( type ) {
		case 0x2e : // AreaMesh
			DecodeMMB((BYTE*)&pFileBuf[pos+16]);
			memcpy(MeshName,pFileBuf+pos+32,16);
			for( i=0 ; i<m_nObj ; i++ ) {
				if( !memcmp(MeshName,m_pObjInfo[i].mObj.id,16) ) break;
			}
			if (i >= m_nObj) {
				pAreaMesh = new CAreaMesh;
				if (pAreaMesh == NULL) return -1;
				memcpy(pAreaMesh->m_AreaName, pFileBuf + pos + 32, 16);
				pAreaMesh->LoadAreaMesh(pFileBuf + pos, this, FVF);
				if (pAreaMesh->GetlpVB() == NULL || pAreaMesh->GetlpIB() == NULL) {
					SAFE_DELETE(pAreaMesh);
				}
				else {
					//				m_LAreaMeshs.push_back(*pAreaMesh );
					m_EffMeshs.InsertEnd(pAreaMesh);
				}
			}
			else {
				pAreaMesh = new CAreaMesh;
				if (pAreaMesh == NULL) return -1;
				memcpy(pAreaMesh->m_AreaName, pFileBuf + pos + 32, 16);
				pAreaMesh->LoadAreaMesh(pFileBuf + pos, this, FVF);
				if (pAreaMesh->GetlpVB() == NULL || pAreaMesh->GetlpIB() == NULL) {
					SAFE_DELETE(pAreaMesh);
				}
				else {
					//				m_LAreaMeshs.push_back(*pAreaMesh );
					m_AreaMeshs.InsertEnd(pAreaMesh);
				}
			}
			break;
		}
		pos+=next;
	}
	for( i=0 ; i<m_nObj ; i++ ) {
		D3DXMatrixIdentity(&AreaMatrix);
		D3DXMatrixScaling(&TempMatrix,m_pObjInfo[i].mObj.fScaleX,m_pObjInfo[i].mObj.fScaleY,m_pObjInfo[i].mObj.fScaleZ );
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationX(&TempMatrix,m_pObjInfo[i].mObj.fRotX);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationY(&TempMatrix,m_pObjInfo[i].mObj.fRotY);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationZ(&TempMatrix,m_pObjInfo[i].mObj.fRotZ);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixTranslation(&TempMatrix,m_pObjInfo[i].mObj.fTransX,m_pObjInfo[i].mObj.fTransY,m_pObjInfo[i].mObj.fTransZ );
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		m_pObjInfo[i].mMat = AreaMatrix;
		m_pObjInfo[i].pAreaMesh = NULL;
		pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
		while( pAreaMesh ) {
			if( !memcmp(pAreaMesh->m_AreaName,m_pObjInfo[i].mObj.id,16) ) {
				m_pObjInfo[i].pAreaMesh = pAreaMesh;				
				break;
			}
			pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
		}
		if( pAreaMesh==NULL ) continue;
		if( (m_pObjInfo[i].mObj.fe&0xfff0fff0)!=0 ) continue;
	}
	SAFE_DELETES( pFileBuf );
	return hr;
}


//
//		�����_�����O
unsigned long CArea::Rendering( float PosX, float PosY, float PosZ )
{
	float			DispArea;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		AreaMatrix;
	D3DXVECTOR3		mPlate;
	D3DXVECTOR3		BL,BL2,BL3,BL4,BH,BH2,BH3,BH4;
	unsigned long	count = 0;

	//---------------------------------------------------------
	// �s�N�Z���V�F�[�_�[�ݒ�
	//---------------------------------------------------------
	GetDevice()->SetPixelShader( NULL );
	mPlate = g_mAt - g_mEye;D3DXVec3Normalize(&mPlate,&mPlate);
	GetDevice()->SetRenderState( D3DRS_FOGENABLE,			TRUE );
	GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
	GetDevice()->SetRenderState( D3DRS_STENCILENABLE,		FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );	
	GetDevice()->SetRenderState( D3DRS_ALPHAREF ,			0x00 );
	GetDevice()->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_NOTEQUAL );
	GetDevice()->SetRenderState( D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA );
	GetDevice()->SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA );
	GetDevice()->SetRenderState( D3DRS_SHADEMODE,			D3DSHADE_GOURAUD );

	//---------------------------------------------------------
	// �V�F�[�_�[�ݒ�
	//---------------------------------------------------------
	GetDevice()->SetTransform(D3DTS_VIEW, &g_mView);
	GetDevice()->SetTransform(D3DTS_PROJECTION, &g_mProjection);
	GetDevice()->SetVertexShaderConstantF(5, (float*)&g_mEye, 1);
	GetDevice()->SetSoftwareVertexProcessing(g_mIsUseSoftware);
//	GetDevice()->LightEnable(0, TRUE);
	// �ϊ��s��
	D3DXMATRIX mTransform = g_mView * g_mProjection;
	D3DXMatrixTranspose(&mTransform, &mTransform);
	GetDevice()->SetVertexShaderConstantF(6, (float*)&mTransform, 4);
	// ���C�g�̕�����ϊ�
	D3DXVECTOR4 LightDir(-g_mLight.Direction.x, -g_mLight.Direction.y, -g_mLight.Direction.z, 0.7f);
	GetDevice()->SetVertexShaderConstantF(4, (float*)&LightDir, 1);

	IDirect3DVertexDeclaration9	*VertexFormat;
//	GetDevice()->SetVertexShader( NULL );
	GetDevice()->SetVertexShader(m_hVertexShader);
	GetDevice()->SetVertexDeclaration(m_VertexFormat);

	//------------------------------------------------------
	// �eArea 1�̃����_�����O
	//------------------------------------------------------
	for( int i=0 ; i<m_nObj ; i++) {
		if( (m_pObjInfo[i].mObj.fe&0xfff0ffff)==0 ) { 
			DispArea = g_mDispArea;
		} else {
			DispArea = g_mDispTree;
		}
		if( (pAreaMesh = m_pObjInfo[i].pAreaMesh)==NULL )
			continue;
		AreaMatrix = m_pObjInfo[i].mMat;
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&m_mRootTransform);
		BL = pAreaMesh->GetBoxLow();
		BH = pAreaMesh->GetBoxHigh();
		BL2.x = BL.x; BL2.y = BL.y; BL2.z = BH.z;
		BL3.x = BH.x; BL3.y = BL.y; BL3.z = BH.z;
		BL4.x = BH.x; BL4.y = BL.y; BL4.z = BL.z;
		BH2.x = BH.x; BH2.y = BH.y; BH2.z = BL.z;
		BH3.x = BL.x; BH3.y = BH.y; BH3.z = BL.z;
		BH4.x = BL.x; BH4.y = BH.y; BH4.z = BH.z;
		D3DXVec3TransformCoord(&BH,&BH,&AreaMatrix );
		D3DXVec3TransformCoord(&BL,&BL,&AreaMatrix );
		D3DXVec3TransformCoord(&BL2,&BL2,&AreaMatrix );
		D3DXVec3TransformCoord(&BH2,&BH2,&AreaMatrix );
		if( Min4(BL.x,BH.x,BL2.x,BH2.x)>PosX+DispArea ) continue;
		if( Max4(BL.x,BH.x,BL2.x,BH2.x)<PosX-DispArea ) continue;
		if( Min4(BL.z,BH.z,BL2.z,BH2.z)>PosZ+DispArea ) continue;
		if( Max4(BL.z,BH.z,BL2.z,BH2.z)<PosZ-DispArea ) continue;
		bool OutFlag = true;
		float val;
		D3DXVECTOR3 TempVec;
		TempVec.x = BL.x; TempVec.y = BL.y;  TempVec.z = BL.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BH.x; TempVec.y = BH.y;  TempVec.z = BH.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BL2.x; TempVec.y = BL2.y;  TempVec.z = BL2.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BL3.x; TempVec.y = BL3.y;  TempVec.z = BL3.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BL4.x; TempVec.y = BL4.y;  TempVec.z = BL4.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BH2.x; TempVec.y = BH2.y;  TempVec.z = BH2.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BH3.x; TempVec.y = BH3.y;  TempVec.z = BH3.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		TempVec.x = BH4.x; TempVec.y = BH4.y;  TempVec.z = BH4.z;
		val = mPlate.x*(TempVec.x - g_mEye.x)+mPlate.y*(TempVec.y - g_mEye.y)+
			        mPlate.z*(TempVec.z - g_mEye.z);
		if( val>=0.f ) OutFlag = false;
		if( OutFlag ) continue;
		
		//---------------------------------------------------------
		// ���_�o�b�t�@���f�o�C�X�ɐݒ�
		//---------------------------------------------------------
		GetDevice()->SetStreamSource( 0, pAreaMesh->GetlpVB(), 0,D3DXGetFVFVertexSize(pAreaMesh->m_FVF) );

		//---------------------------------------------------------
		// �C���f�b�N�X�o�b�t�@���f�o�C�X�ɐݒ�
		//---------------------------------------------------------
		GetDevice()->SetIndices( pAreaMesh->m_lpIB );


		//---------------------------------------------------------
		// �T�[�t�F�C�X���ƂɃ����_�����O
		//---------------------------------------------------------
		// �}�g���b�N�X�ݒ�
		D3DXMatrixTranspose( &AreaMatrix, &AreaMatrix );
		GetDevice()->SetVertexShaderConstantF( 10, (float*)&AreaMatrix, 4 );

		// �����_�����O
		list<CStream>::iterator its = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite = pAreaMesh->m_LStreams.end();
		for( ; its != ite ; its++ ) {	
			if( its->GetAlphaFlag() & 0x02 || its->GetStencilFlag() ) {
				GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_NONE );
			} else {
				if( (m_pObjInfo[i].mObj.fScaleX*m_pObjInfo[i].mObj.fScaleZ)<0.f )
					GetDevice()->SetRenderState( D3DRS_CULLMODE,	D3DCULL_CCW );
				else
					GetDevice()->SetRenderState( D3DRS_CULLMODE,	D3DCULL_CW );
			}
			CTexture *pTexture = (CTexture*)its->GetpTexture();
			if( (its->GetAlphaFlag() & 0x08) || its->GetStencilFlag() ) {
				if( its->GetStencilFlag() ) {
					GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
					GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		TRUE );
				} else {
					GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
					GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );
				}
				GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			} else {
				GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
				GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );
				GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
			}
			//
			// c14 : �}�e���A��
			//
			if( pTexture != NULL ) {
				// �f�o�C�X�Ƀe�N�X�`���ݒ�
				GetDevice()->SetTexture( 0, pTexture->GetTexture() );
			} else {
				GetDevice()->SetTexture( 0, NULL );
			}
			HRESULT hr = GetDevice()->DrawIndexedPrimitive(
					its->GetPrimitiveType(),
					0,
					0,
					pAreaMesh->GetNumVertices(),
					its->GetIndexStart(),
					its->GetFaceCount() );

		}
		count += pAreaMesh->GetNumFaces();

	}
	//---------------------------------------------------------
	// �㏈��
	//-----------------------------------------------------------
	GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );	
	GetDevice()->SetRenderState( D3DRS_FOGENABLE,			FALSE );
	//GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	return count;
}

//		MAP�f�[�^�ǂݍ���
bool CArea::LoadMAP()
{
	unsigned long FVF = (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);
	char FileName[512],FileName2[512];

	// �f�[�^������
	InitData();

	if( (GetArea()&0xeffff) == 0 ) return true;

	//  �t�@�C���̃��[�h
	GetFileNameFromDno(FileName,GetArea());
	GetFileNameFromDno(FileName2,ConvertStr2Dno("0-0"));
	//  �t�@�C���̃��[�h
	HRESULT hr;
	//�@�e�N�X�`���A���b�V���̃��[�h
//	hr = LoadTextureFromFile( FileName2 );
//	if( hr ) return false;
	hr = LoadTextureFromFile( FileName );
	if( hr ) return false;
	hr = LoadAreaFromFile( FileName, FVF );
	if( hr ) return false;
	return true;
}

//======================================================================
//		MQO�Z�[�u		�ʏ�f�[�^��MQO�t�H�[�}�b�g�ŏo�͂��܂�
//======================================================================
bool CArea::saveMQO(char *FPath, char *FName,float posX,float posY,float posZ)
{
	FILE			*fd;
	char			*ptr, path[256],texpath[256];
	D3DTEXVERTEX	*pV;
	D3DTEXVERTEX	mVertex;
	D3DXVECTOR3     mVer;
	WORD			*pIndex, *pI;
	int				cnt,nVer, nFace,idxmin,idxmax;
	int				i1, i2, i3, t1, t2, t3;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		AreaMatrix;
	float			DispArea;
	D3DXVECTOR3		BL, BL2, BL3, BL4, BH, BH2, BH3, BH4;

	if ((ptr = strrstr(FPath, FName))) *ptr = '\0';
	if ((ptr = strrstr(FName, ".mqo"))) *ptr = '\0';
	sprintf(path, "%s%s.mqo", FPath, FName);
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if ((fd = fopen(path, "w")) == NULL) return false;
	fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
	fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
	fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
	fprintf(fd, "	head 0.0000\n");
	fprintf(fd, "	pich 0.0000\n");
	fprintf(fd, "	ortho 1\n");
	fprintf(fd, "	zoom2 2.0000\n");
	fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
	fprintf(fd, "%d {\n", m_Textures.Count);
	CTexture *pTexture = (CTexture*)m_Textures.Top();
	int texNo = 0;
	while (pTexture != NULL) {
		if (pTexture == NULL) continue;
		char	texName[256];
		strcpy(texName, pTexture->GetTexName()); Trim(texName);
		fprintf(fd, "    \"%s\" col(1.000 1.000 1.000 1.000)", texName);
		fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.bmp\")\n", texName);
		sprintf(texpath, "%s%s.bmp", FPath, texName);
		D3DXSaveTextureToFile(texpath, D3DXIFF_BMP, pTexture->GetTexture(), NULL);
		pTexture = (CTexture*)pTexture->Next;
		texNo++;
	}
	fprintf(fd, "}\n");
	for (int num = 0; num<m_nObj; num++) {
		if ((m_pObjInfo[num].mObj.fe & 0xfff0ffff) == 0) {
			DispArea = g_mDispArea;
		}
		else {
			DispArea = g_mDispTree;
		}
		if ((pAreaMesh = m_pObjInfo[num].pAreaMesh) == NULL) continue;
		AreaMatrix = m_pObjInfo[num].mMat;
		D3DXMatrixMultiply(&AreaMatrix, &AreaMatrix, &m_mRootTransform);
		BL = pAreaMesh->GetBoxLow();
		BH = pAreaMesh->GetBoxHigh();
		BL2.x = BL.x; BL2.y = BL.y; BL2.z = BH.z;
		BL3.x = BH.x; BL3.y = BL.y; BL3.z = BH.z;
		BL4.x = BH.x; BL4.y = BL.y; BL4.z = BL.z;
		BH2.x = BH.x; BH2.y = BH.y; BH2.z = BL.z;
		BH3.x = BL.x; BH3.y = BH.y; BH3.z = BL.z;
		BH4.x = BL.x; BH4.y = BH.y; BH4.z = BH.z;
		D3DXVec3TransformCoord(&BH, &BH, &AreaMatrix);
		D3DXVec3TransformCoord(&BL, &BL, &AreaMatrix);
		D3DXVec3TransformCoord(&BL2, &BL2, &AreaMatrix);
		D3DXVec3TransformCoord(&BH2, &BH2, &AreaMatrix);
		if (Min4(BL.x, BH.x, BL2.x, BH2.x)>posX+DispArea) continue;
		if (Max4(BL.x, BH.x, BL2.x, BH2.x)<posX-DispArea) continue;
		if (Min4(BL.z, BH.z, BL2.z, BH2.z)>posZ+DispArea) continue;
		if (Max4(BL.z, BH.z, BL2.z, BH2.z)<posZ-DispArea) continue;
		ptr = pAreaMesh->GetAreaName();
		*(ptr + 17) = 0x0;
		for (int i = 0; i < 18; i++ ) {
			if (*ptr++ == 0x20) *ptr = 0x0;
		}
		// �o�[�e�b�N�X�o�b�t�@���f�o�C�X�ɐݒ�
		pAreaMesh->m_lpVB->Lock(0, pAreaMesh->m_VBSize, (void **)&pV, D3DLOCK_READONLY);
		// �C���f�b�N�X�o�b�t�@���f�o�C�X�ɐݒ�
		pAreaMesh->m_lpIB->Lock(0, pAreaMesh->m_IBSize, (void **)&pIndex, D3DLOCK_READONLY);
		nVer = nFace = 0;
		// 
		list<CStream>::iterator its2 = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite2 = pAreaMesh->m_LStreams.end();
		for (int count = 0; its2 != ite2; its2++, count++) {
			fprintf(fd, "Object \"%s%02d\" {\n", pAreaMesh->GetAreaName(), count);
			fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 0.898 0.498 0.698\n   color_type 0\n");
			pI = pIndex + its2->GetIndexStart();
			idxmin = 65535; idxmax = 0;
			for (unsigned int i = 0; i < its2->GetFaceCount() + 2; i++) {
				i1 = *pI++;
				if (i1 > idxmax) idxmax = i1;
				if (i1 < idxmin) idxmin = i1;
			}
			fprintf(fd, "vertex %d {\n", idxmax-idxmin+1);
			for (unsigned long verCnt = idxmin; verCnt <= idxmax; verCnt++) {
				mVertex = *(pV + verCnt); mVer = mVertex.v;
				D3DXVec3TransformCoord(&mVer, &mVer, &AreaMatrix);
				fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
			}
			fprintf(fd, "\t}\n");
			if (its2->GetPrimitiveType() == D3DPT_TRIANGLESTRIP) {
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					i1 = (*pI++) - idxmin; nVer++; i2 = (*pI++) - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2) {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							*pI++; nVer++;
							break;
						}
						nFace++;
						i1 = i2; i2 = i3;
					}
				}
				fprintf(fd, "face %d {\n", nFace);
				//fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					cnt = 0;
					i1 = (*pI++) - idxmin; nVer++;
					i2 = (*pI++) - idxmin; nVer++;
					while( nVer<its2->GetFaceCount()+2 )  {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							*pI++; nVer++; break;
						}
						if ( nVer % 2) {
							t1 = i3; t2 = i2; t3 = i1;
						}
						else {
							t1 = i1; t2 = i2; t3 = i3;
						}
						fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
							t1 , t2 , t3, its2->m_TexNo,
							(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
							(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
						cnt++; nFace++;
						i1 = i2; i2 = i3;
					}
				}
				//i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin;
				//for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
				//	i3 = (*pI++) - idxmin;
				//	if ((idxmin+i) % 2) {
				//		t1 = i1; t2 = i2; t3 = i3;
				//	}
				//	else {
				//		t1 = i3; t2 = i2; t3 = i1;
				//	}
				//	fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
				//		t1 , t2 , t3, count,
				//		(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
				//		(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				//	i1 = i2; i2 = i3;
				//}
			}
			else if (its2->GetPrimitiveType() == D3DPT_TRIANGLELIST) {
				fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
					i1 = (*pI++)-idxmin; i2 = (*pI++)-idxmin; i3 = (*pI++)-idxmin;
					t1 = i3; t2 = i2; t3 = i1;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, its2->m_TexNo,
						(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
						(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				}
			}
			fprintf(fd, "\t}\n");
			fprintf(fd, "}\n");
		}
		//break;
		pAreaMesh->m_lpIB->Unlock();
		pAreaMesh->m_lpVB->Unlock();
	}
	fprintf(fd, "EOF");
	fclose(fd);
	return true;
}

//======================================================================
//		MQO�Z�[�u �G�t�F�N�g�f�[�^���ׂĂ�MQO�t�H�[�}�b�g�ŏo�͂��܂�
//======================================================================
bool CArea::saveMQO2(char *FPath, char *FName, float posX, float posY, float posZ)
{
	FILE			*fd;
	char			*ptr, path[256], texpath[256];
	D3DTEXVERTEX	*pV;
	D3DTEXVERTEX	mVertex;
	D3DXVECTOR3     mVer;
	WORD			*pIndex, *pI;
	int				cnt, nVer, nFace, idxmin, idxmax;
	int				i1, i2, i3, t1, t2, t3;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		AreaMatrix;
	float			DispArea;
	D3DXVECTOR3		BL, BL2, BL3, BL4, BH, BH2, BH3, BH4;

	if ((ptr = strrstr(FPath, FName))) *ptr = '\0';
	if ((ptr = strrstr(FName, ".mqo"))) *ptr = '\0';
	sprintf(path, "%s%s.mqo", FPath, FName);
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if ((fd = fopen(path, "w")) == NULL) return false;
	fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
	fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
	fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
	fprintf(fd, "	head 0.0000\n");
	fprintf(fd, "	pich 0.0000\n");
	fprintf(fd, "	ortho 1\n");
	fprintf(fd, "	zoom2 2.0000\n");
	fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
	fprintf(fd, "%d {\n", m_Textures.Count);
	CTexture *pTexture = (CTexture*)m_Textures.Top();
	int texNo = 0;
	while (pTexture != NULL) {
		if (pTexture == NULL) continue;
		char	texName[256];
		strcpy(texName, pTexture->GetTexName()); Trim(texName);
		fprintf(fd, "    \"%s\" col(1.000 1.000 1.000 1.000)", texName);
		fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.bmp\")\n", texName);
		sprintf(texpath, "%s%s.bmp", FPath, texName);
		D3DXSaveTextureToFile(texpath, D3DXIFF_BMP, pTexture->GetTexture(), NULL);
		pTexture = (CTexture*)pTexture->Next;
		texNo++;
	}
	fprintf(fd, "}\n");
	AreaMatrix = m_mRootTransform;
	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
	while (pAreaMesh) {
		ptr = pAreaMesh->GetAreaName();
		*(ptr + 17) = 0x0;
		for (int i = 0; i < 18; i++) {
			if (*ptr++ == 0x20) *ptr = 0x0;
		}
		// �o�[�e�b�N�X�o�b�t�@���f�o�C�X�ɐݒ�
		pAreaMesh->m_lpVB->Lock(0, pAreaMesh->m_VBSize, (void **)&pV, D3DLOCK_READONLY);
		// �C���f�b�N�X�o�b�t�@���f�o�C�X�ɐݒ�
		pAreaMesh->m_lpIB->Lock(0, pAreaMesh->m_IBSize, (void **)&pIndex, D3DLOCK_READONLY);
		nVer = nFace = 0;
		// 
		list<CStream>::iterator its2 = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite2 = pAreaMesh->m_LStreams.end();
		for (int count = 0; its2 != ite2; its2++, count++) {
			fprintf(fd, "Object \"%s%02d\" {\n", pAreaMesh->GetAreaName(), count);
			fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 0.898 0.498 0.698\n   color_type 0\n");
			pI = pIndex + its2->GetIndexStart();
			idxmin = 65535; idxmax = 0;
			for (unsigned int i = 0; i < its2->GetFaceCount() + 2; i++) {
				i1 = *pI++;
				if (i1 > idxmax) idxmax = i1;
				if (i1 < idxmin) idxmin = i1;
			}
			fprintf(fd, "vertex %d {\n", idxmax - idxmin + 1);
			for (unsigned long verCnt = idxmin; verCnt <= idxmax; verCnt++) {
				mVertex = *(pV + verCnt); mVer = mVertex.v;
				D3DXVec3TransformCoord(&mVer, &mVer, &AreaMatrix);
				fprintf(fd, "        %4.4f %4.4f %4.4f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
			}
			fprintf(fd, "\t}\n");
			if (its2->GetPrimitiveType() == D3DPT_TRIANGLESTRIP) {
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					i1 = (*pI++) - idxmin; nVer++; i2 = (*pI++) - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2) {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							*pI++; nVer++;
							break;
						}
						nFace++;
						i1 = i2; i2 = i3;
					}
				}
				fprintf(fd, "face %d {\n", nFace);
				//fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					cnt = 0;
					i1 = (*pI++) - idxmin; nVer++;
					i2 = (*pI++) - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2)  {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							*pI++; nVer++; break;
						}
						if (nVer % 2) {
							t1 = i3; t2 = i2; t3 = i1;
						}
						else {
							t1 = i1; t2 = i2; t3 = i3;
						}
						fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
							t1, t2, t3, its2->m_TexNo,
							(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
							(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
						cnt++; nFace++;
						i1 = i2; i2 = i3;
					}
				}
				//i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin;
				//for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
				//	i3 = (*pI++) - idxmin;
				//	if ((idxmin+i) % 2) {
				//		t1 = i1; t2 = i2; t3 = i3;
				//	}
				//	else {
				//		t1 = i3; t2 = i2; t3 = i1;
				//	}
				//	fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
				//		t1 , t2 , t3, count,
				//		(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
				//		(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				//	i1 = i2; i2 = i3;
				//}
			}
			else if (its2->GetPrimitiveType() == D3DPT_TRIANGLELIST) {
				fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
					i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin; i3 = (*pI++) - idxmin;
					t1 = i3; t2 = i2; t3 = i1;
//					t1 = i1; t2 = i2; t3 = i3;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, its2->m_TexNo,
						(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
						(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				}
			}
			fprintf(fd, "\t}\n");
			fprintf(fd, "}\n");
		}
		//break;
		pAreaMesh->m_lpIB->Unlock();
		pAreaMesh->m_lpVB->Unlock();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	fprintf(fd, "EOF");
	fclose(fd);
	return true;
}

//======================================================================
//
//		�e�N�X�`���J�E���g
//
//		�X�g���[�����̃e�N�X�`���f�[�^���J�E���g����
//======================================================================
int CAreaMesh::countTextures(void)
{
	int numTex = 0;

	list<CStream>::iterator its = this->m_LStreams.begin();
	list<CStream>::iterator ite = this->m_LStreams.end();
	for (; its != ite; its++) {
		CTexture *pTexture = (CTexture*)its->GetpTexture();
		if (pTexture)
			numTex++;
	}
	return numTex;
}



//		�R���X�g���N�^
CListBase::CListBase()
{
	ReferenceCount = 1;
	Prev = Next = NULL;
	pParentList = NULL;
}

//		�f�X�g���N�^
CListBase::~CListBase()
{
	if ( pParentList != NULL )
	{
		pParentList->Erase( this );
	}
}

//		�J��
long CListBase::Release( void )
{
	long ref = ReferenceCount - 1;

	// �Q�Ƃ��Ȃ��Ȃ�����j��
	if ( --ReferenceCount == 0 ) delete this;

	return ref;
}

//		�Q�ƃJ�E���^�C���N�������g
void CListBase::AddRef( void )
{
	ReferenceCount++;
}

//		�R���X�g���N�^
CList::CList()
{
	Init();
}

//		�f�X�g���N�^
CList::~CList()
{
	Release();
}

//		������
void CList::Init( void )
{
	ListTop = NULL;
	ListEnd = NULL;
	Count = 0;
}

//		�擪�擾
LPCListBase CList::Top( void )
{
	return ListTop;
}

//		�I�[�擾
LPCListBase CList::End( void )
{
	return ListEnd;
}

//		���X�g���
void CList::Release( void )
{
	LPCListBase p = ListTop;
	while ( p != NULL )
	{
		// p �̎������O�Ɏ擾�ip �� Release() ���̂���Ă�\�������j
		LPCListBase pp = p->Next;
		// ���
		p->Release();
		// ��
		p = pp;
	}
	Init();
}

//		���X�g�̐擪�ɑ}��
void CList::InsertTop( LPCListBase p )
{
	// ���̃��X�g�ɓo�^����Ă�Ƃ��͂����炩��ؒf
	if ( p->pParentList != NULL ) p->pParentList->Erase( p );
	p->pParentList = this;

	// �ڑ�
	p->Prev = NULL;
	p->Next = ListTop;
	ListTop = p;
	if ( p->Next != NULL ) p->Next->Prev = p;
	if ( ListEnd == NULL ) ListEnd = p;
	Count++;
}
//		���X�g�̏I�[�ɑ}��
void CList::InsertEnd( LPCListBase p )
{
	// ���̃��X�g�ɓo�^����Ă�Ƃ��͂����炩��ؒf
	if ( p->pParentList != NULL ) p->pParentList->Erase( p );
	p->pParentList = this;

	// �ڑ�
	CListBase *pEnd = ListEnd;

	p->Prev = pEnd;
	p->Next = NULL;
	ListEnd = p;

	if ( pEnd == NULL )	{
		ListTop = p;
	} else {
		pEnd->Next = p;
	}
	Count++;
}

//		�^�[�Q�b�g�̑O�ɂɑ}��
void CList::InsertPrev( LPCListBase pTarget, LPCListBase pIt )
{
	if( pTarget == NULL ) 
		return;
	// ���̃��X�g�ɓo�^����Ă�Ƃ��͂����炩��ؒf
	if ( pIt->pParentList != NULL ) pIt->pParentList->Erase( pIt );
	pIt->pParentList = this;

	// �ڑ�
	pIt->Prev = pTarget->Prev;
	pIt->Next = pTarget;
	pTarget->Prev = pIt;
	if( ListTop == pTarget ) {
		ListTop = pIt;
		pIt->Prev = NULL;
	}
	Count++;
}

//		�^�[�Q�b�g�̎��ɑ}��
void CList::InsertNext( LPCListBase pTarget,LPCListBase pIt )
{
	if( pTarget == NULL ) 
		return;
	// ���̃��X�g�ɓo�^����Ă�Ƃ��͂����炩��ؒf
	if ( pIt->pParentList != NULL ) pIt->pParentList->Erase( pIt );
	pIt->pParentList = this;

	// �ڑ�
	pIt->Prev = pTarget;
	pIt->Next = pTarget->Next;
	pTarget->Next = pIt;
	if( ListEnd == pTarget ) 
		ListEnd = pIt;
	Count++;
}

//		���X�g����폜
void CList::Erase( LPCListBase p )
{
	if ( p->pParentList != NULL ) p->pParentList = NULL;

	BYTE flag = 0x00;
	if ( p->Prev == NULL ) flag |= 0x01;		// �O�ɉ����Ȃ��Ƃ�
	if ( p->Next == NULL ) flag |= 0x02;		// ��ɉ����Ȃ��Ƃ�

	//	�Y������f�[�^�̍폜

	switch ( flag )
	{
	///////////////////////////////////// �O��ɉ�������Ƃ�
	case 0x00:
		p->Prev->Next = p->Next;
		p->Next->Prev = p->Prev;
		break;
	///////////////////////////////////// �O�ɉ����Ȃ��Ƃ�
	case 0x01:
		ListTop = p->Next;
		ListTop->Prev = NULL;
		break;
	///////////////////////////////////// ��ɉ����Ȃ��Ƃ�
	case 0x02:
		ListEnd = ListEnd->Prev;
		p->Prev->Next = NULL;
		break;
	///////////////////////////////////// �O��ɉ����Ȃ��Ƃ�
	case 0x03:
		ListTop = NULL;
		ListEnd = NULL;
		break;
	}
	Count--;
}


//		����̃f�[�^���o��
LPCListBase CList::Data( long no )
{
	LPCListBase p = ListTop;
	while ( (p != NULL) && no-- ) {
		p = p->Next;
	}
	return p;
}

//		�T�C�Y�擾
long CList::Size( void )
{
	return Count;
}
