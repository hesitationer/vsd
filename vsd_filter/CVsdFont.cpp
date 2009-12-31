/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFont.cpp - CVsdFont class implementation
	$Id$
	
*****************************************************************************/

#include <windows.h>
#include "dds.h"
#include "CVsdFont.h"

/*** macros *****************************************************************/

/*** static member **********************************************************/

/*** �R���X�g���N�^ *********************************************************/

CVsdFont::CVsdFont( LOGFONT &logfont ){
	
	BITMAPINFO	biBMP;
	HDC			hdc;
    HFONT		hFont, hFontOld;
    int			i;
	
	ZeroMemory( &biBMP, sizeof( biBMP ));
	
	/* 256*256�s�N�Z���A32�r�b�gDIB�pBITMAPINFO�ݒ� */
	biBMP.bmiHeader.biSize		= sizeof( BITMAPINFOHEADER );
	biBMP.bmiHeader.biBitCount	= 32;
	biBMP.bmiHeader.biPlanes	= 1;
	biBMP.bmiHeader.biWidth		= 8;
	biBMP.bmiHeader.biHeight	= -8;
	
	/* ��DIBSection�쐬 */
	HBITMAP hbmpTemp = CreateDIBSection( NULL, &biBMP, DIB_RGB_COLORS, ( void ** )( &m_pFontData ), NULL, 0 );
	
	/* �E�C���h�E��HDC�擾 */
	HWND hwnd = GetDesktopWindow();	// AviSynth �� hwnd �������̂ŁC�Ă��Ƃ� (^^;
	hdc = GetDC( hwnd );
	
	/* DIBSection�p�������f�o�C�X�R���e�L�X�g�쐬 */
	m_hdcBMP = CreateCompatibleDC( hdc );
	
	/* �E�C���h�E��HDC��� */
	ReleaseDC( hwnd, hdc );
	
	/* DIBSection��HBITMAP���������f�o�C�X�R���e�L�X�g�ɑI�� */
	m_hbmpOld = ( HBITMAP )SelectObject( m_hdcBMP, hbmpTemp );
	
	hFont = CreateFontIndirect( &logfont );
	
	hFontOld = ( HFONT )SelectObject( m_hdcBMP, hFont );
	
	// �t�H���g�T�C�Y��񏉊���
	SIZE size;
	GetTextExtentPoint32( m_hdcBMP, "B", 1, &size );	// W ����ԕ����L��
	
	m_iFontH = size.cy;
	m_iFontW = size.cx;
	m_iBMP_W = m_iFontW * 16 * 2;
	
	/* 256*256�s�N�Z���A32�r�b�gDIB�pBITMAPINFO�ݒ� */
	biBMP.bmiHeader.biWidth		= m_iBMP_W;
	biBMP.bmiHeader.biHeight	= -m_iFontH * 7;
	
	/* DIBSection�쐬 */
	m_hbmpBMP = CreateDIBSection( NULL, &biBMP, DIB_RGB_COLORS, ( void ** )( &m_pFontData ), NULL, 0 );
	
	/* DIBSection��HBITMAP���������f�o�C�X�R���e�L�X�g�ɑI�� */
	SelectObject( m_hdcBMP, m_hbmpBMP );
	
	/* ��DIBSection���폜 */
	DeleteObject( hbmpTemp );
	
	// BMP �𔒂ɏ�����
	for( i = 0; i < m_iBMP_W * m_iFontH * 7; ++i ){
		m_pFontData[ i ] = 0xFFFFFF;
	}
	
	// ��������ׂ�
	char szChar[ 1 ];
	
	for( i = 0x20; i < 0x80; ++i ){
		*szChar = i;
		GetTextExtentPoint32( m_hdcBMP, szChar, 1, &size );
		
		TextOut(
			m_hdcBMP,
			( i % 16 ) * m_iFontW * 2 + ( m_iFontW * 2 - size.cx ) / 2,
			(( i - 0x20 ) / 16 ) * m_iFontH, szChar, 1
		);
	}
	
	SelectObject( m_hdcBMP, hFontOld );
	DeleteObject( hFont );
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdFont::~CVsdFont(){
	/* DIBSection���������f�o�C�X�R���e�L�X�g�̑I������O�� */
	SelectObject( m_hdcBMP, m_hbmpOld );
	
	/* DIBSection���폜 */
	DeleteObject( m_hbmpBMP );
	
	/* �������f�o�C�X�R���e�L�X�g���폜 */
	DeleteDC( m_hdcBMP );
}
