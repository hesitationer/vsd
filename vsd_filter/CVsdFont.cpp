/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFont.cpp - CVsdFont class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "CVsdFont.h"

/*** macros *****************************************************************/

/*** static member **********************************************************/

#ifdef DEBUG
int CVsdFont::m_iObjCnt = 0;
#endif

/*** �R���X�g���N�^ *********************************************************/

CVsdFont::CVsdFont( const char *szFontName, int iSize, UINT uAttr ){
	//DebugMsgD( "new CFont %d:%X\n", ++m_iObjCnt, this );
	CreateFont( szFontName, iSize, uAttr );
}

CVsdFont::CVsdFont( LPCWSTR szFontName, int iSize, UINT uAttr ){
	//DebugMsgD( "new CFont %d:%X\n", ++m_iObjCnt, this );
	char szFont[ LF_FACESIZE ];
	
	WideCharToMultiByte(
		CP_ACP,				// �R�[�h�y�[�W
		0,					// �������x�ƃ}�b�s���O���@�����肷��t���O
		szFontName,			// ���C�h������̃A�h���X
		-1,					// ���C�h������̕�����
		szFont,				// �V������������󂯎��o�b�t�@�̃A�h���X
		sizeof( szFont ),	// �V������������󂯎��o�b�t�@�̃T�C�Y
		NULL,				// �}�b�v�ł��Ȃ������̊���l�̃A�h���X
		NULL				// ����̕������g�����Ƃ��ɃZ�b�g����t���O�̃A�h���X
	);
	
	CreateFont( szFont, iSize, uAttr );
}

/*** �t�H���g�쐬 ***********************************************************/

void CVsdFont::CreateFont( const char *szFontName, int iSize, UINT uAttr ){
	m_uAttr = uAttr;
	
	m_LogFont.lfHeight			= iSize;						// �����Z���܂��͕����̍���
	m_LogFont.lfWidth			= 0;							// ���ϕ�����
	m_LogFont.lfEscapement		= 0;							// ��������̕�����X���Ƃ̊p�x
	m_LogFont.lfOrientation		= 0;							// �x�[�X���C����X���Ƃ̊p�x
	m_LogFont.lfWeight			= uAttr & ATTR_BOLD ? FW_BOLD : FW_REGULAR;	// �t�H���g�̑���
	m_LogFont.lfItalic			= uAttr & ATTR_ITALIC ? TRUE : FALSE;		// �C�^���b�N�̎w��
	m_LogFont.lfUnderline		= FALSE;						// �����t���w��
	m_LogFont.lfStrikeOut		= FALSE;						// �ł��������t���w��
	m_LogFont.lfCharSet			= DEFAULT_CHARSET;				// �L�����N�^�Z�b�g
	m_LogFont.lfOutPrecision	= OUT_DEFAULT_PRECIS;			// �o�͐��x
	m_LogFont.lfClipPrecision	= CLIP_DEFAULT_PRECIS;			// �N���b�s���O�̐��x
	m_LogFont.lfQuality			= PROOF_QUALITY;				// �o�͕i��
	m_LogFont.lfPitchAndFamily	= FIXED_PITCH | FF_DONTCARE;	// �s�b�`�ƃt�@�~��
	strcpy( m_LogFont.lfFaceName, szFontName );					// �t�H���g��
	
	CreateFont();
}

const MAT2 CVsdFont::mat = {{ 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 }};

void CVsdFont::CreateFont( void ){
	
	// DC, FONT �n���h���擾
	HDC		hdc			= GetDC( NULL );
	HFONT	hFont		= CreateFontIndirect( &m_LogFont );
	HFONT	hFontOld	= ( HFONT )SelectObject( hdc, hFont );
	
	// tmAscent �擾
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
    GLYPHMETRICS	gm;
	
	int	iBitmapDepth = IsNoAntialias() ? GGO_BITMAP : GGO_GRAY8_BITMAP;
	
	// �v���|�[�V���i���� Space ���擾
	GetGlyphOutlineW( hdc, ' ', iBitmapDepth, &gm, 0, NULL, &mat );
	m_iFontW_Space = gm.gmCellIncX;
	
	GetGlyphOutlineW( hdc, 'B', iBitmapDepth, &gm, 0, NULL, &mat );
	m_iFontW = gm.gmCellIncX;	// 'W' ����ԕ����L��
	m_iFontH = tm.tmHeight;
	
	SelectObject( hdc, hFontOld );
	DeleteObject( hFont );
	ReleaseDC( NULL, hdc );			/* �E�C���h�E��HDC��� */
}

// ���� glyph �쐬
CFontGlyph& CVsdFont::CreateFontGlyph( WCHAR c ){
	
	// DC, FONT �n���h���擾
	HDC		hdc			= GetDC( NULL );
	HFONT	hFont		= CreateFontIndirect( &m_LogFont );
	HFONT	hFontOld	= ( HFONT )SelectObject( hdc, hFont );
	
	// tmAscent �擾
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
    GLYPHMETRICS	gm;
	
	int	iBitmapDepth = IsNoAntialias() ? GGO_BITMAP : GGO_GRAY8_BITMAP;
	
	// �K�v�z��T�C�Y�擾
	int iSize = GetGlyphOutlineW( hdc, c, iBitmapDepth, &gm, 0, NULL, &mat );
	
	CFontGlyph	*pGlyph;
	if( c <= FONT_CHAR_LAST ){
		pGlyph = &m_FontGlyph[ c - FONT_CHAR_FIRST ];
	}else{
		pGlyph = new CFontGlyph;
	}
	
	if( iSize > 0 ){
		pGlyph->pBuf = new BYTE[ iSize ];
		
		// �t�H���g�f�[�^�擾
		GetGlyphOutlineW( hdc, c, iBitmapDepth, &gm, iSize, pGlyph->pBuf, &mat );
		pGlyph->iW			= gm.gmBlackBoxX;
		pGlyph->iH			= gm.gmBlackBoxY;
		pGlyph->iOrgY		= ( short )( tm.tmAscent - gm.gmptGlyphOrigin.y );
		pGlyph->iCellIncX	= ( short )gm.gmCellIncX;
	}else{
		pGlyph->pBuf		= NULL;
		pGlyph->iW			=
		pGlyph->iH			=
		pGlyph->iOrgY		= 0;
		pGlyph->iCellIncX	= m_iFontW_Space;
	}
	
	if( c > FONT_CHAR_LAST ) m_FontGlyphK[ c ] = pGlyph;
	
	SelectObject( hdc, hFontOld );
	DeleteObject( hFont );
	ReleaseDC( NULL, hdc );			/* �E�C���h�E��HDC��� */
	
	return *pGlyph;
}
