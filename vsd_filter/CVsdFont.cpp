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
	CreateFont( logfont );
}

CVsdFont::CVsdFont( const char *szFontName, int iSize, int iAttr ){
	LOGFONT	logfont;
	
	logfont.lfHeight			= iSize;						// �����Z���܂��͕����̍���
	logfont.lfWidth				= 0;							// ���ϕ�����
	logfont.lfEscapement		= 0;							// ��������̕�����X���Ƃ̊p�x
	logfont.lfOrientation		= 0;							// �x�[�X���C����X���Ƃ̊p�x
	logfont.lfWeight			= FW_REGULAR;					// �t�H���g�̑���
	logfont.lfItalic			= FALSE;						// �C�^���b�N�̎w��
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

/*** �f�X�g���N�^ ***********************************************************/

CVsdFont::~CVsdFont(){
	int i;

	for( i = 0; i <= '~' - '!'; ++i ){
		delete [] m_FontGlyph[ i ].pBuf;
	}
	delete [] m_FontGlyph;
	m_FontGlyph = NULL;
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
