/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter_avs.cpp - vsd_filter for AviSynth
	$Id$
	
*****************************************************************************/

#include <windows.h>
#include "avisynth.h"

#include "dds.h"
#include "dds_lib/dds_lib.h"
#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CVsdFilter.h"

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
	
	// ���z�֐�
	virtual void PutPixel( int x, int y, const PIXEL_YCA &yc, UINT uFlag );
	virtual void FillLine( int x1, int y1, int x2, const PIXEL_YCA &yc, UINT uFlag );
	
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
	
	// FONT �w��
	if( p = args[ ARGID_STRPARAM_FONT ].AsString( NULL )) strcpy( m_logfont.lfFaceName, p );
	m_logfont.lfWeight	= m_piParamS[ SHADOW_FONT_ATTR ] & 0x1 ? FW_BOLD : FW_REGULAR;
	m_logfont.lfItalic	= m_piParamS[ SHADOW_FONT_ATTR ] & 0x2 ? 1 : 0;
	
	// ���O���[�h
	#ifndef GPS_ONLY
		if( p = args[ ARGID_STRPARAM_LOGFILE ].AsString( NULL )) if( !ReadLog( p ))
			env->ThrowError( PROG_NAME ": read log \"%s\" failed.", p );
	#endif
	
	// GPS ���O���[�h
	if( p = args[ ARGID_STRPARAM_GPSFILE ].AsString( NULL )) if( !GPSLogLoad( p ))
		env->ThrowError( PROG_NAME ": read GPS log \"%s\" failed.", p );
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

void CVsdFilterAvs::PutPixel( int x, int y, const PIXEL_YCA &yc, UINT uFlag ){
	
	if( uFlag & IMG_POLYGON ){
		// �|���S���`��
		if( x > m_Polygon[ y ].iRight ) m_Polygon[ y ].iRight = x;
		if( x < m_Polygon[ y ].iLeft  ) m_Polygon[ y ].iLeft  = x;
	}else{
		if( 0 <= x && x < GetWidth() && 0 <= y && y < GetHeight() ){
			int	iIndex	= GetIndex( x, y );
			
			if( yc.alfa ){
				int iAlfa = ( int )yc.alfa;
				
				m_pPlane[ iIndex + 0 ] = ( PIXEL_t )(
					yc.y + ((  m_pPlane[ iIndex + 0 ] * iAlfa ) >> 8 )
				);
				m_pPlane[ iIndex + 1 ] = ( PIXEL_t )(
					(( x & 1 )? yc.cr : yc.cb ) + (( m_pPlane[ iIndex + 1 ] * iAlfa ) >> 8 )
				);
			}else{
				*( USHORT *)( m_pPlane + iIndex ) = ( x & 1 ) ? yc.ycr : yc.ycb;
				m_pPlane[ iIndex ] = yc.y;
			}
		}
	}
}

void CVsdFilterAvs::FillLine( int x1, int y1, int x2, const PIXEL_YCA &yc, UINT uFlag ){
	
	if( uFlag & IMG_POLYGON ){
		// �|���S���`��
		if( x1 > x2 ){
			if( x1 > m_Polygon[ y1 ].iRight ) m_Polygon[ y1 ].iRight = x1;
			if( x2 < m_Polygon[ y1 ].iLeft  ) m_Polygon[ y1 ].iLeft  = x2;
		}else{
			if( x2 > m_Polygon[ y1 ].iRight ) m_Polygon[ y1 ].iRight = x2;
			if( x1 < m_Polygon[ y1 ].iLeft  ) m_Polygon[ y1 ].iLeft  = x1;
		}
	}else if( 0 <= y1 && y1 < GetHeight()){
		if( x1 < 0 )         x1 = 0;
		if( x2 > GetWidth()) x2 = GetWidth();
		
		int iIndex = GetIndex( x1, y1 );
		
		if( yc.alfa ){
			int iAlfa = ( int )yc.alfa;
			
			for( int x = x1; x <= x2; ++x, iIndex += 2 ){
				m_pPlane[ iIndex + 0 ] = ( PIXEL_t )(
					yc.y + ((  m_pPlane[ iIndex + 0 ] * iAlfa ) >> 8 )
				);
				m_pPlane[ iIndex + 1 ] = ( PIXEL_t )(
					(( x & 1 )? yc.cr : yc.cb ) + (( m_pPlane[ iIndex + 1 ] * iAlfa ) >> 8 )
				);
			}
		}else{
			// x1, x2 �����[�� pixel �Ȃ�C���ꂾ����ɏ���
			if( x1 & 1 ){
				*( USHORT *)( m_pPlane + iIndex ) = yc.ycr;
				++x1;
				iIndex += 2;
			}
			if( !( x2 & 1 )){
				*( USHORT *)( m_pPlane + GetIndex( x2, y1 )) = yc.ycb;
				--x2;
			}
			for( int x = x1; x <= x2; x += 2, iIndex += 4 ){
				*( UINT *)( m_pPlane + iIndex ) = yc.ycbcr;
			}
		}
	}
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
