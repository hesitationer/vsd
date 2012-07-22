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

CVsdFont::CVsdFont( LOGFONT &logfont ){
	
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
	m_FontGlyph = new tFontGlyph[ '~' - '!' + 1 ];
	
	for( i = 0; i <= '~' - '!'; ++i ){
		// �K�v�z��T�C�Y�擾
		iSize = GetGlyphOutline( hdc, '!' + i, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat );
		if( iSize > 0 ){
			m_FontGlyph[ i ].pBuf = new BYTE[ iSize ];
			
			// �t�H���g�f�[�^�擾
			GetGlyphOutline( hdc, '!' + i, GGO_GRAY8_BITMAP, &gm, iSize, m_FontGlyph[ i ].pBuf, &mat );
			m_FontGlyph[ i ].iW		= gm.gmBlackBoxX;
			m_FontGlyph[ i ].iH		= gm.gmBlackBoxY;
			m_FontGlyph[ i ].iOrgY	= tm.tmAscent - gm.gmptGlyphOrigin.y;
			
			if( i == 'B' - '!' ){	// 'W' ����ԕ����L��
				m_iFontW = gm.gmCellIncX;
			}
		}else{
			m_FontGlyph[ i ].pBuf	= NULL;
			m_FontGlyph[ i ].iW		=
			m_FontGlyph[ i ].iH		=
			m_FontGlyph[ i ].iOrgY	= 0;
		}
	}
	
	m_iFontH = tm.tmHeight;
	
	SelectObject( hdc, hFontOld );
	DeleteObject( hFont );
	ReleaseDC( NULL, hdc );			/* �E�C���h�E��HDC��� */
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdFont::~CVsdFont(){
	int i;

	for( i = 0; i <= '~' - '!'; ++i ){
		delete [] m_FontGlyph[ i ].pBuf;
	}
	delete [] m_FontGlyph;
	m_FontGlyph = NULL;
}
