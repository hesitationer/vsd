/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter_avs.cpp - vsd_filter for AviSynth
	
*****************************************************************************/

#include "StdAfx.h"

#include "avisynth.h"

#include "dds.h"
#include "dds_lib/dds_lib.h"
#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CScript.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "error_code.h"

#define PROG_NAME	"VSDFilter"
#define VERSION		"v1.07��2"

/****************************************************************************/

enum {
	ARGID_CLIP,
	#define DEF_STR_PARAM( id, var, init, conf_name )			ARGID_ ## id,
	#include "def_str_param.h"
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	ARGID_ ## id,
	#define DEF_TRACKBAR_N( id, init, min, max, name, conf_name )
	#include "def_trackbar.h"
	#define DEF_CHECKBOX( id, init, name, conf_name )			ARGID_ ## id,
	#define DEF_CHECKBOX_N( id, init, name, conf_name )
	#include "def_checkbox.h"
	#define DEF_SHADOW( id, init, conf_name )					ARGID_ ## id,
	#include "def_shadow.h"
	ARGID_MARK,
	ARGID_NUM
};

/****************************************************************************/

class CVsdFilterAvs : public GenericVideoFilter, CVsdFilter {
  public:
	CVsdFilterAvs(
		PClip _child,
		AVSValue args,
		IScriptEnvironment* env
	);
	
	~CVsdFilterAvs();
	PVideoFrame __stdcall GetFrame( int n, IScriptEnvironment* env );
	
	/////////////
	
	int GetIndex( int x, int y ){ return m_iBytesPerLine * y + x * 2; }
	
	void PutPixelInit( void ){
		m_iBufX = -1;
	}
	void PutPixelFlush( void ){
		if( m_iBufX >= 0 ){
			
		}
	}
	
	// ���z�֐�
	void         PutPixel( int iIndex, const PIXEL_YCA_ARG yc, int iAlfa );
	virtual void PutPixel( int x, int y, const PIXEL_YCA_ARG yc );
	virtual void FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc );
	virtual UINT PutImage( int x, int y, CVsdImage &img );
	void PutPixelFlush( int x, int y ){
		PIXEL_YCA	yc;
		
		yc.y	= m_ycaBuf0.y;
		yc.alfa	= m_ycaBuf1.y;
		yc.cb	= ( m_ycaBuf0.cb + m_ycaBuf1.cb ) >> 1;
		yc.cr	= ( m_ycaBuf0.cr + m_ycaBuf1.cr ) >> 1;
		
		*( UINT *)( m_pPlane + GetIndex( x, y )) = yc.ycbcr;
	}
	
	virtual int	GetWidth( void )	{ return m_iWidth; }
	virtual int	GetHeight( void )	{ return m_iHeight; }
	virtual int	GetFrameMax( void )	{ return vi.num_frames; }
	virtual int	GetFrameCnt( void )	{ return m_iFrameCnt; }
	virtual double	GetFPS( void )	{ return ( double )vi.fps_numerator / vi.fps_denominator; }
	
	virtual void SetFrameMark( int iFrame );
	virtual int  GetFrameMark( int iFrame );
	
	// �p�����[�^
	int	m_iWidth, m_iHeight, m_iFrameCnt, m_iFrameMax;
	int m_iBytesPerLine;
	int *m_piMark;
	int m_iMarkCnt;
	
	BYTE	*m_pPlane;
	const char *m_szMark;
	
  private:
	PIXEL_YCA	m_ycaBuf0;
	PIXEL_YCA	m_ycaBuf1;
	int			m_iBufX;
	int			m_iBufY;
};

/*** �R���X�g���N�^�E�f�X�g���N�^ *******************************************/

// param �w��
CVsdFilterAvs::CVsdFilterAvs(
	PClip _child,
	AVSValue args,
	IScriptEnvironment* env
) : GenericVideoFilter( _child ){
	
	const char *p;
	
	if( !vi.IsYUY2()) env->ThrowError( PROG_NAME ": requires YUY2 input.");
	
	// �p�����[�^������
	m_piParamT	= new int[ TRACK_N ];
	m_piParamC	= new int[ CHECK_N ];
	m_piParamS	= new int[ SHADOW_N ];
	m_piMark	= new int[ MAX_LAP ];
	m_iMarkCnt	= 0;
	
	// �p�����[�^�����l
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	m_piParamT[ id ] = init;
	#include "def_trackbar.h"
	
	#define DEF_CHECKBOX( id, init, name, conf_name )	m_piParamC[ id ] = init;
	#include "def_checkbox.h"
	
	#define DEF_SHADOW( id, init, conf_name )	m_piParamS[ id ] = init;
	#include "def_shadow.h"
	
	// �����w��ɂ�菉����
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name ) \
		if( args[ ARGID_ ## id ].Defined()) m_piParamT[ id ] = args[ ARGID_ ## id ].AsInt();
	#define DEF_TRACKBAR_N( id, init, min, max, name, conf_name )
	#include "def_trackbar.h"
	
	#define DEF_CHECKBOX( id, init, name, conf_name ) \
		if( args[ ARGID_ ## id ].Defined()) m_piParamC[ id ] = args[ ARGID_ ## id ].AsInt();
	#define DEF_CHECKBOX_N( id, init, name, conf_name )
	#include "def_checkbox.h"
	
	#define DEF_SHADOW( id, init, conf_name ) \
		if( args[ ARGID_ ## id ].Defined()) m_piParamS[ id ] = args[ ARGID_ ## id ].AsInt();
	#include "def_shadow.h"
	
	// mark= ��������
	if( p = args[ ARGID_MARK ].AsString( NULL )) ParseMarkStr( p );
	
	// ���O���[�h
	#ifndef GPS_ONLY
		if( p = args[ ARGID_STRPARAM_LOGFILE ].AsString( NULL )) if( !ReadLog( p ))
			env->ThrowError( PROG_NAME ": read log \"%s\" failed.", p );
	#endif
	
	// GPS ���O���[�h
	if( p = args[ ARGID_STRPARAM_GPSFILE ].AsString( NULL )) if( !GPSLogLoad( p ))
		env->ThrowError( PROG_NAME ": read GPS log \"%s\" failed.", p );
	
	// �X�L�����[�h
	if( p = args[ ARGID_STRPARAM_SKINFILE ].AsString( NULL )){
		strcpy( m_szSkinFile, p );
	}
}

CVsdFilterAvs::~CVsdFilterAvs(){
	delete [] m_piParamT;
	delete [] m_piParamC;
	delete [] m_piParamS;
	delete [] m_piMark;
}

/*** PutPixel ***************************************************************/

/* �ϊ���
Y  =  0.299R+0.587G+0.114B
Cr =  0.500R-0.419G-0.081B
Cb = -0.169R-0.332G+0.500B

R = Y+1.402Cr 
G = Y-0.714Cr-0.344Cb 
B = Y+1.772Cb 
*/

inline PIXEL_YCA_ARG PutPixelBuf( PIXEL_YCA_ARG yc0, PIXEL_YCA_ARG yc1 ){
	int iAlfa = yc1.alfa;
	if( iAlfa == 0 ){
		return yc1;
	}
	
	iAlfa += iAlfa >> 7;
	
	yc0.y  = (( yc0.y * iAlfa ) >> 8 ) + yc1.y;
	yc0.cr = ((( int )( yc0.cr - 0x80 ) * iAlfa ) >> 8 ) + yc1.cr;
	yc0.cb = ((( int )( yc0.cb - 0x80 ) * iAlfa ) >> 8 ) + yc1.cb;
	
	return yc0;
}

inline void CVsdFilterAvs::PutPixel( int x, int y, const PIXEL_YCA_ARG yc ){
	
	int iIndex = GetIndex( x & ~1, y );
	if( !( x & 1 )){
		// ���� pixel
		if( m_iBufX >= 0 ) PutPixelFlush( m_iBufX, m_iBufY );		// �t���b�V������
		m_ycaBuf0 = m_ycaBuf1 = *( UINT *)( m_pPlane + iIndex );	// m_ycaBuf0/1 �Ƀ��[�h
		m_ycaBuf1.y = m_ycaBuf1.alfa;
		m_ycaBuf0 = PutPixelBuf( m_ycaBuf0, yc );					// yca0 �ɏ���
		m_iBufX = x;
		m_iBufY = y;
	}else{
		// � pixel
		if( m_iBufX != x - 1 || m_iBufY != y ){
			// yca �o�b�t�@�Ɗ֌W��������
			if( m_iBufX >= 0 ) PutPixelFlush( m_iBufX, m_iBufY );		// �t���b�V������
			m_ycaBuf0 = m_ycaBuf1 = *( UINT *)( m_pPlane + iIndex );	// m_ycaBuf0/1 �Ƀ��[�h
			m_ycaBuf1.y = m_ycaBuf1.alfa;
		}
		m_ycaBuf1 = PutPixelBuf( m_ycaBuf1, yc );						// yca1 �ɏ���
		PutPixelFlush( x - 1, y );
		m_iBufX = -1;
	}
}

inline void CVsdFilterAvs::FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc ){
	
	int x;
	if( yc.alfa ){
		for( x = x1; x <= x2; ++x ){
			PutPixel( x, y1, yc );
		}
	}else{
		int iIndex = GetIndex( x1, y1 );
		
		PIXEL_YCA yca = yc;
		yca.alfa = yca.y;
		
		// x1, x2 �����[�� pixel �Ȃ�C���ꂾ����ɏ���
		if( x1 & 1 ){
			PutPixel( x1, y1, yca );
			++x1;
			iIndex += 2;
		}
		for( x = x1; x < x2; x += 2, iIndex += 4 ){
			*( UINT *)( m_pPlane + iIndex ) = yca.ycbcr;
		}
		if( x <= x2 ){
			PutPixel( x, y1, yca );
		}
	}
}

/*** PutImage ***************************************************************/

UINT CVsdFilterAvs::PutImage(
	int x, int y, CVsdImage &img
){
	int xst = ( x >= 0 ) ? 0 : -x;
	int xed = x + img.m_iWidth <= GetWidth() ? img.m_iWidth : GetWidth() - x;
	int yst = ( y >= 0 ) ? 0 : -y;
	int yed = y + img.m_iHeight <= GetHeight() ? img.m_iHeight : GetHeight() - y;
	
	for( int y1 = yst; y1 < yed; ++y1 ){
		
		int	iIndex = GetIndex( x + xst, y + y1 );
		int x1 = xst;
		
		// �擪�̔��[�� 1pixel ����
		if( iIndex & 2 ){
			PIXEL_YCA yc( img.GetPixel0( x1, y1 ));
			PutPixel( x1, y1, yc );
			iIndex += 2;
			++x1;
		}
		for( ; x1 < xed - 1; x1 += 2, iIndex += 4 ){
			PIXEL_YCA yc0( img.GetPixel0( x1    , y1 ));
			PIXEL_YCA yc1( img.GetPixel0( x1 + 1, y1 ));
			
			if( yc0.alfa == 0 && yc1.alfa == 0 ){
				// 2�s�N�Z�������� 100% �s����
				yc0.cr	= ( yc0.cr + yc1.cr ) >> 1;
				yc0.cb	= ( yc0.cb + yc1.cb ) >> 1;
				yc0.alfa= yc1.y;
				
				*( UINT *)( m_pPlane + iIndex ) = yc0.ycbcr;
			}else{
				// �����ł͂Ȃ�����
				PutPixel( x + x1,     y + y1, yc0 );
				PutPixel( x + x1 + 1, y + y1, yc1 );
			}
		}
		// ��[�̔��[�� 1pixel ����
		if( x1 < xed ){
			PIXEL_YCA yc( img.GetPixel0( x1, y1 ));
			PutPixel( x + x1, y + y1, yc );
		}
	}
	
	return ERR_OK;
}

/*** �t���[�����}�[�N *******************************************************/

void CVsdFilterAvs::SetFrameMark( int iFrame ){
	m_piMark[ m_iMarkCnt++ ] = iFrame;
	m_piMark[ m_iMarkCnt   ] = -1;
};

int CVsdFilterAvs::GetFrameMark( int iFrame ){
	
	int	i;
	
	for( i = 0; m_piMark[ i ] < iFrame && m_piMark[ i ] >= 0; ++i );
	
	return m_piMark[ i ];
}

/****************************************************************************/

PVideoFrame __stdcall CVsdFilterAvs::GetFrame( int n, IScriptEnvironment* env ){
	PVideoFrame src = child->GetFrame( n, env );
	env->MakeWritable(&src);
	
	m_iBytesPerLine		= src->GetPitch();
	m_iWidth			= src->GetRowSize()>>1; //case of YUY2
	m_iHeight			= src->GetHeight();
	m_iFrameCnt			= n;
	
	m_pPlane			= src->GetWritePtr();
	
	DrawVSD();
	
	return src;
}

AVSValue __cdecl Create_VSDFilter( AVSValue args, void* user_data, IScriptEnvironment* env ){
	return new CVsdFilterAvs(
		args[0].AsClip(),
		args,
		env
	);
}

extern "C" __declspec( dllexport ) const char* __stdcall AvisynthPluginInit2( IScriptEnvironment* env ){
	
	env->AddFunction( "VSDFilter",
		"c"
		#define DEF_STR_PARAM( id, var, init, conf_name )			"[" conf_name "]s"
		#include "def_str_param.h"
		#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	"[" conf_name "]i"
		#define DEF_TRACKBAR_N( id, init, min, max, name, conf_name )
		#include "def_trackbar.h"
		#define DEF_CHECKBOX( id, init, name, conf_name )			"[" conf_name "]i"
		#define DEF_CHECKBOX_N( id, init, name, conf_name )
		#include "def_checkbox.h"
		#define DEF_SHADOW( id, init, conf_name )					"[" conf_name "]i"
		#include "def_shadow.h"
		"[mark]s",
		Create_VSDFilter, 0
	);
	
	return "`VSDFilter' vehicle data logger overlay plugin";
}
