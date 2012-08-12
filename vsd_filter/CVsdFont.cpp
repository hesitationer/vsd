/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFont.cpp - CVsdFont class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "CVsdFont.h"

/*** macros *****************************************************************/

/*** static member **********************************************************/

/*** �R���X�g���N�^ *********************************************************/

CVsdFont::CVsdFont( const char *szFontName, int iSize, UINT uAttr ){
	CreateFont( szFontName, iSize, uAttr );
}

CVsdFont::CVsdFont( LPCWSTR szFontName, int iSize, UINT uAttr ){
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

void CVsdFont::CreateFont( void ){
	
	const MAT2		mat = {{ 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 }};
    GLYPHMETRICS	gm;
	
	// DC, FONT �n���h���擾
	HDC		hdc			= GetDC( NULL );
	HFONT	hFont		= CreateFontIndirect( &m_LogFont );
	HFONT	hFontOld	= ( HFONT )SelectObject( hdc, hFont );
	
	// tmAscent �擾
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
	
	int	iBitmapDepth = IsNoAntialias() ? GGO_BITMAP : GGO_GRAY8_BITMAP;
	
	// �v���|�[�V���i���� Space ���擾
	GetGlyphOutline( hdc, ' ', iBitmapDepth, &gm, 0, NULL, &mat );
	m_iFontW_Space = gm.gmCellIncX;
	
	for( int i = FONT_CHAR_FIRST; i <= FONT_CHAR_LAST; ++i ){
		// �K�v�z��T�C�Y�擾
		int iSize = GetGlyphOutline( hdc, i, iBitmapDepth, &gm, 0, NULL, &mat );
		
		if( iSize > 0 ){
			FontGlyph( i ).pBuf = new BYTE[ iSize ];
			
			// �t�H���g�f�[�^�擾
			GetGlyphOutline( hdc, i, iBitmapDepth, &gm, iSize, FontGlyph( i ).pBuf, &mat );
			FontGlyph( i ).iW			= gm.gmBlackBoxX;
			FontGlyph( i ).iH			= gm.gmBlackBoxY;
			FontGlyph( i ).iOrgY		= tm.tmAscent - gm.gmptGlyphOrigin.y;
			FontGlyph( i ).iCellIncX	= gm.gmCellIncX;
		}else{
			FontGlyph( i ).pBuf			= NULL;
			FontGlyph( i ).iW			=
			FontGlyph( i ).iH			=
			FontGlyph( i ).iOrgY		= 0;
			FontGlyph( i ).iCellIncX	= m_iFontW_Space;
		}
	}
	
	m_iFontW = FontGlyph( 'B' ).iCellIncX;	// 'W' ����ԕ����L��
	m_iFontH = tm.tmHeight;
	
	SelectObject( hdc, hFontOld );
	DeleteObject( hFont );
	ReleaseDC( NULL, hdc );			/* �E�C���h�E��HDC��� */
}
