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
	m_uAttr = uAttr;
	
	LOGFONT	logfont;
	
	logfont.lfHeight			= iSize;						// �����Z���܂��͕����̍���
	logfont.lfWidth				= 0;							// ���ϕ�����
	logfont.lfEscapement		= 0;							// ��������̕�����X���Ƃ̊p�x
	logfont.lfOrientation		= 0;							// �x�[�X���C����X���Ƃ̊p�x
	logfont.lfWeight			= uAttr & ATTR_BOLD ? FW_BOLD : FW_REGULAR;	// �t�H���g�̑���
	logfont.lfItalic			= uAttr & ATTR_ITALIC ? TRUE : FALSE;		// �C�^���b�N�̎w��
	logfont.lfUnderline			= FALSE;						// �����t���w��
	logfont.lfStrikeOut			= FALSE;						// �ł��������t���w��
	logfont.lfCharSet			= DEFAULT_CHARSET;				// �L�����N�^�Z�b�g
	logfont.lfOutPrecision		= OUT_DEFAULT_PRECIS;			// �o�͐��x
	logfont.lfClipPrecision		= CLIP_DEFAULT_PRECIS;			// �N���b�s���O�̐��x
	logfont.lfQuality			= PROOF_QUALITY;				// �o�͕i��
	logfont.lfPitchAndFamily	= FIXED_PITCH | FF_DONTCARE;	// �s�b�`�ƃt�@�~��
	strcpy( logfont.lfFaceName, szFontName );					// �t�H���g��
	
	CreateFont( logfont );
}

/*** �t�H���g�쐬 ***********************************************************/

void CVsdFont::CreateFont( LOGFONT &logfont ){
	
	const MAT2		mat = {{ 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 }};
    GLYPHMETRICS	gm;
    int	i;
	
	// DC, FONT �n���h���擾
	HDC		hdc			= GetDC( NULL );
	HFONT	hFont		= CreateFontIndirect( &logfont );
	HFONT	hFontOld	= ( HFONT )SelectObject( hdc, hFont );
	
	// tmAscent �擾
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
	
	int iSize;
	
	// �v���|�[�V���i���� Space ���擾
	GetGlyphOutline( hdc, ' ', GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat );
	m_iFontW_Space = gm.gmCellIncX;
	
	for( i = FONT_CHAR_FIRST; i <= FONT_CHAR_LAST; ++i ){
		// �K�v�z��T�C�Y�擾
		iSize = GetGlyphOutline( hdc, i, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat );
		
		if( iSize > 0 ){
			FontGlyph( i ).pBuf = new BYTE[ iSize ];
			
			// �t�H���g�f�[�^�擾
			GetGlyphOutline( hdc, i, GGO_GRAY8_BITMAP, &gm, iSize, FontGlyph( i ).pBuf, &mat );
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
