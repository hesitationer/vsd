/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdImage.cpp - Image file operation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "pixel.h"
#include "CVsdImage.h"

/*** �R���X�g���N�^�E�f�X�g���N�^ *******************************************/

CVsdImage::CVsdImage(){
	m_pRGBA_Buf	= NULL;
	m_pPixelBuf	= NULL;
	
	m_iWidth  = 0;	// �摜�̕�
	m_iHeight = 0;	//  �V  ����
}

CVsdImage::~CVsdImage(){
	delete [] m_pRGBA_Buf;
	delete [] m_pPixelBuf;
}

/*** �C���[�W�̃��[�h *******************************************************/

BOOL CVsdImage::Load( const char *szFileName ){
	// �����R�[�h�����C�h������ɕϊ�
	// �y���Ӂz�{���͂��̉ӏ��͕�����o�b�t�@���̍l���̑��ɕ�����I�[�R�[�h������������Z�L���A�ȑΉ����D�܂����ł��B
	wchar_t	path[ MAX_PATH ];
	size_t	pathLength = 0;
	
	if(
		mbstowcs_s(
			&pathLength,	// [out]	�ϊ����ꂽ������
			&path[0],		// [out]	�ϊ����ꂽ���C�h��������i�[����o�b�t�@�̃A�h���X(�ϊ���)
			MAX_PATH,		// [in]	 �o�͑��̃o�b�t�@�̃T�C�Y(�P��:������)
			szFileName,		// [in]	 �}���`�o�C�g������̃A�h���X(�ϊ���)
			_TRUNCATE		// [in]	 �o�͐�Ɋi�[���郏�C�h�����̍ő吔
		) != 0
	){
		return FALSE;
	}
	
	// GDI+�I�u�W�F�N�g�i�摜�W�J�ɕK�v�j
	Gdiplus::GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR						gdiplusToken;
	
	//---- GDI+�̏����ݒ�
	if( Gdiplus::GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL ) != Gdiplus::Ok ){
		return FALSE;
	}
	
	//-------------------------------------------------------------
	// �摜�̓ǂݍ���
	//-------------------------------------------------------------
	BOOL	result = TRUE;
	m_iWidth  = 0;	// �摜�̕�
	m_iHeight = 0;	//  �V  ����
	
	//--- �摜�t�@�C�����J��
	//  �y�Ή��摜�`���z  BMP, JPEG, PNG, GIF, TIFF, WMF, EMF
	Gdiplus::Bitmap*	pBitmap = Gdiplus::Bitmap::FromFile( path );
	if( pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok ){
		//---- �摜�T�C�Y���̗̈�m��
		m_iWidth  = pBitmap->GetWidth();
		m_iHeight = pBitmap->GetHeight();
		
		// �����[�h����摜�̃������̉𑜓x��ύX/�ݒ�i���̈ʒu�ɔC�ӂɋL�q���ĉ������j
		m_pRGBA_Buf = new PIXEL_RGBA[ m_iWidth * m_iHeight ];
		
		//---- �摜�C���[�W�̓ǂݍ���
		for( int y = 0; y < m_iHeight; ++y ){
			for( int x = 0; x < m_iWidth; ++x ){
				Gdiplus::Color srcColor;
				pBitmap->GetPixel( x, y, &srcColor );
				
				m_pRGBA_Buf[ x + y * m_iWidth ].r = srcColor.GetR();
				m_pRGBA_Buf[ x + y * m_iWidth ].g = srcColor.GetG();
				m_pRGBA_Buf[ x + y * m_iWidth ].b = srcColor.GetB();
				m_pRGBA_Buf[ x + y * m_iWidth ].a = 255 - srcColor.GetA();
			}
		}
	}
	
	delete pBitmap;
	
	//---- GDI+�̉��
	Gdiplus::GdiplusShutdown( gdiplusToken );
	
	return result;
}

/*** PIXEL_RGBA -> PIXEL_YCA �ϊ� *******************************************/

BOOL CVsdImage::ConvRGBA2YCA( void ){
	
	if( m_pPixelBuf || !m_pRGBA_Buf ) return FALSE;
	
	// �������m��
	if( !( m_pPixelBuf = new PIXEL_YCA[ m_iWidth * m_iHeight ] )){
		return FALSE;
	}
	
	for( int y = 0; y < m_iHeight; ++y ){
		for( int x = 0; x < m_iWidth; ++x ){
			int iIdx = x + y * m_iWidth;
			Color2YCA( m_pPixelBuf[ iIdx ], m_pRGBA_Buf[ iIdx ].argb );
		}
	}
	
	delete [] m_pRGBA_Buf;
	m_pRGBA_Buf = NULL;
	
	return TRUE;
}
