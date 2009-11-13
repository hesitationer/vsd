/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFilter.h - CVsdFilter class header
	$Id$
	
*****************************************************************************/

#ifndef _CVsdFilter_h_
#define _CVsdFilter_h_

#define BUF_SIZE	1024
#define MAX_LAP		200
#define GPS_FREQ	10

#ifdef AVS_PLUGIN
	typedef UCHAR	PIXEL_t;
	
	#define RGB2YC( r, g, b ) { \
		(( int )( 0.299 * r + 0.587 * g + 0.114 * b ) >> 4 ), \
		(( int )(-0.169 * r - 0.331 * g + 0.500 * b ) >> 4 ) + 0x80, \
		(( int )( 0.299 * r + 0.587 * g + 0.114 * b ) >> 4 ), \
		(( int )( 0.500 * r - 0.419 * g - 0.081 * b ) >> 4 ) + 0x80 \
	}
#else
	typedef short	PIXEL_t;
	
	#define RGB2YC( r, g, b ) { \
		( int )( 0.299 * r + 0.587 * g + 0.114 * b ), \
		( int )(-0.169 * r - 0.331 * g + 0.500 * b ), \
		( int )( 0.500 * r - 0.419 * g - 0.081 * b ) \
	}
#endif

#define G_CX_CNT		30

#define LINE_WIDTH		( GetWidth() / HIREZO_TH + 1 )

#define GPS_LOG_OFFS	15

#define MAP_LINE1		yc_green
#define MAP_LINE2		yc_yellow
#define MAP_LINE3		yc_red
#define MAP_G_MAX		1.2

#define HIREZO_TH		600			// �n�C���]���[�h���̉����X���b�V�����h
#define POS_DEFAULT		0x80000000

#define BESTLAP_NONE	599999

/*** track / check ID *******************************************************/

enum {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	id,
	#include "def_trackbar.h"
	TRACK_N
};

enum {
	#define DEF_CHECKBOX( id, init, name, conf_name )	id,
	#include "def_checkbox.h"
	CHECK_N
};

enum {
	#define DEF_SHADOW( id, init, conf_name )	id,
	#include "def_shadow.h"
	SHADOW_N
};

enum {
	LAPMODE_HAND,	// �蓮�v�����[�h
	LAPMODE_GPS,	// GPS �����v�����[�h
	LAPMODE_MAGNET,	// ���C�Z���T�[�����v�����[�h
};

/*** new type ***************************************************************/

typedef struct {
	USHORT	uLap;
	int		iLogNum;
	int		iTime;
} LAP_t;

typedef struct {
	short	iLeft, iRight;
} PolygonData_t;

#ifdef AVS_PLUGIN
typedef struct {
	union {
		// yuv �ʎw��
		struct {
			UCHAR	y;
			UCHAR	cb;
			UCHAR	y1;
			UCHAR	cr;
		};
		
		// ycb / ycr �A�N�Z�X
		struct {
			USHORT	ycb;
			USHORT	ycr;
		};
	};
} PIXEL_YC;
#endif

class CVsdFilter {
	
  public:
	CVsdFilter();
	~CVsdFilter();
	
	/*** �摜�I�y���[�V���� *************************************************/
	
	virtual void PutPixel( int x, int y, const PIXEL_YC &yc, UINT uFlag ) = 0;
	
	void DrawLine( int x1, int y1, int x2, int y2, const PIXEL_YC &yc, UINT uFlag );
	void DrawLine( int x1, int y1, int x2, int y2, int width, const PIXEL_YC &yc, UINT uFlag );
	void FillLine( int x1, int y1, int x2,         const PIXEL_YC &yc, UINT uFlag );
	
	void DrawRect( int x1, int y1, int x2, int y2, const PIXEL_YC &yc, UINT uFlag );
	void DrawCircle( int x, int y, int r, const PIXEL_YC &yc, UINT uFlag );
	void DrawCircle( int x, int y, int r, int a, int b, const PIXEL_YC &yc, UINT uFlag );
	
	void DrawFont( int x, int y, UCHAR c, const PIXEL_YC &yc, UINT uFlag );
	void DrawFont( int x, int y, UCHAR c, const PIXEL_YC &yc, const PIXEL_YC &ycEdge, UINT uFlag );
	void DrawString( char *szMsg, const PIXEL_YC &yc, UINT uFlag, int x = POS_DEFAULT, int y = POS_DEFAULT );
	void DrawString( char *szMsg, const PIXEL_YC &yc, const PIXEL_YC &ycEdge, UINT uFlag, int x = POS_DEFAULT, int y = POS_DEFAULT );
	
	// �|���S���`��
	void PolygonClear( void );
	void PolygonDraw( const PIXEL_YC &yc, UINT uFlag );
	
	// �t�H���g
	UCHAR GetBMPPix( int x, int y ){
		return m_pFontData[ ( m_iBMP_H - y - 1 ) * m_iBMP_BytesPerLine + ( x >> 3 ) ] & ( 1 << ( 7 - ( x & 0x7 )));
	}
	
	int GetFontW( void ){ return m_iFontW; }
	int GetFontH( void ){ return m_iFontH; }
	void InitFont( void );
	
	BOOL DrawVSD( void );
	
	enum {
		IMG_ALFA	= ( 1 << 0 ),
		IMG_FILL	= ( 1 << 1 ),
		IMG_POLYGON	= ( 1 << 2 ),
	};
	
	int m_iPosX, m_iPosY;
	int	m_iFontW, m_iFontH, m_iBMP_H, m_iBMP_BytesPerLine;
	
	int	m_iPreW;
	
	int	m_iLapIdx;
	int m_iBestLogNumRunning;
	
	PolygonData_t	*m_Polygon;
	
	// ���z�֐�
	virtual int	GetWidth( void )	= 0;
	virtual int	GetHeight( void )	= 0;
	virtual int	GetFrameMax( void )	= 0;
	virtual int	GetFrameCnt( void )	= 0;
	
  private:
	static const UCHAR m_Font9p[];
	static const UCHAR m_Font18p[];
	
	const UCHAR *m_pFontData;
	
	/*** ���O�I�y���[�V���� *************************************************/
	
  public:
	char *IsConfigParam( const char *szParamName, char *szBuf, int &iVal );
	BOOL ConfigLoad( const char *szFileName );
	BOOL ParseMarkStr( const char *szMark );
	BOOL GPSLogLoad( const char *szFileName );
	
	BOOL ReadLog( const char *szFileName );
	
	double LapNum2LogNum( CVsdLog *Log, int iLapNum );
	
	void DrawSpeedGraph( CVsdLog *Log, const PIXEL_YC &yc );
	
	CVsdLog		*m_VsdLog;
	CVsdLog		*m_GPSLog;
	
	LAP_t		*m_Lap;
	int			m_iLapMode;
	int			m_iLapNum;
	int			m_iBestTime;
	int			m_iBestLap;
	
	int			m_iLogStart;
	int			m_iLogStop;
	
	double		m_dVideoFPS;
	
	int			*m_piParamT;
	int			*m_piParamC;
	int			*m_piParamS;
	
	BOOL		m_bCalcLapTimeReq;
	
	// �X�^�[�g���C��@GPS �v�����[�h
	double	m_dStartLineX1;
	double	m_dStartLineY1;
	double	m_dStartLineX2;
	double	m_dStartLineY2;
	
	static const char *m_szTrackbarName[];
	static const char *m_szCheckboxName[];
	static const char *m_szShadowParamName[];
	
	char	*m_szLogFile;
	char	*m_szGPSLogFile;
	
	// ���z�֐�
	virtual void SetFrameMark( int iFrame ) = 0;
	virtual void CalcLapTime( void ) = 0;
	
	// �t���[���ԍ������߂� CalcLaptimeAuto( int ) ���Ă�
	virtual void CalcLapTimeAuto( void ) = 0;
	void CalcLapTimeAuto( int iFrame );
	
	virtual char *GetVideoFileName( char *szFileName ){ return ""; }
  private:
};
#endif
