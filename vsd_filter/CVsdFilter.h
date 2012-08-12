/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFilter.h - CVsdFilter class header
	
*****************************************************************************/

#ifndef _CVsdFilter_h_
#define _CVsdFilter_h_

#define PROG_NAME		"VSDFilter"
#define PROG_NAME_J		"VSD���[�^�[����"
#define PROG_NAME_LONG	"`VSDFilter' vehicle data logger overlay plugin"
#define PROG_VERSION	"v1.10beta1"

#define GPS_FREQ	10
#define MAX_LAP		200

#define G_CX_CNT		30
#define POS_DEFAULT		0x80000000

#ifdef GPS_ONLY
	#define VideoSt			m_piParamS[ PARAM_VSt ]
	#define VideoEd			m_piParamS[ PARAM_VEd ]
	#define LogSt			0
	#define LogEd			0
	#define GPSSt			m_piParamS[ PARAM_GSt ]
	#define GPSEd			m_piParamS[ PARAM_GEd ]
#else
	#define VideoSt			m_piParamT[ PARAM_VSt ]
	#define VideoEd			m_piParamT[ PARAM_VEd ]
	#define LogSt			m_piParamT[ PARAM_LSt ]
	#define LogEd			m_piParamT[ PARAM_LEd ]
	#define GPSSt			m_piParamT[ PARAM_GSt ]
	#define GPSEd			m_piParamT[ PARAM_GEd ]
#endif

// �p�����[�^��ϊ�
#define ConvParam( p, from, to ) ( \
	from##Ed == from##St ? 0 : \
	( double )( to##Ed - to##St ) * (( p ) - from##St ) \
	/ ( from##Ed - from##St ) \
	+ to##St \
)

// VSD log ��D��C�������`�F�b�N�{�b�N�X�ŃI�[�o�[���C�h�ł���
#define SelectLogVsd ( m_CurLog = ( GPSPriority && m_GPSLog || !m_VsdLog ) ? m_GPSLog : m_VsdLog )

// GPS log ��D��
#define SelectLogGPS ( m_CurLog = m_GPSLog ? m_GPSLog : m_VsdLog )

// Laptime �v�Z�p
#define SelectLogForLapTime	( m_CurLog = \
	m_LapLog && ( \
		m_LapLog->m_iLapMode == LAPMODE_MAGNET || \
		m_LapLog->m_iLapMode == LAPMODE_HAND_MAGNET \
	) ? m_VsdLog : m_GPSLog )

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

/*** new type ***************************************************************/

typedef struct {
	short	iLeft, iRight;
} PolygonData_t;

class CVsdFilter {
	
  public:
	CVsdFilter();
	~CVsdFilter();
	
	/*** �摜�I�y���[�V���� *************************************************/
	
	void PutPixel(	// !js_func
		int x, int y, tRABY uColor,
		UINT uFlag	// !default:0
	);
	void         PutPixel( int x, int y, const PIXEL_YCA_ARG yc, UINT uFlag );
	virtual void PutPixel( int x, int y, const PIXEL_YCA_ARG yc ) = 0;
	void         FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc, UINT uFlag );
	virtual void FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc ) = 0;
	virtual UINT PutImage( int x, int y, CVsdImage &img ) = 0;	// !js_func
	
	void DrawLine( int x1, int y1, int x2, int y2, const PIXEL_YCA_ARG yc, UINT uFlag );
	void DrawLine( int x1, int y1, int x2, int y2, tRABY uColor, UINT uFlag );
	void DrawLine(		// !js_func
		int x1, int y1, int x2, int y2,
		int width,		// !arg:5 !default:1
		tRABY uColor,	// !arg:4
		UINT uFlag		// !default:0
	);
	
	void DrawRect(	// !js_func
		int x1, int y1, int x2, int y2,
		tRABY uColor, UINT uFlag
	);
	void DrawCircle(	// !js_func
		int x, int y, int r,
		tRABY uColor,
		UINT uFlag		// !default:0
	);
	void DrawCircle( int x, int y, int a, int b, tRABY uColor, UINT uFlag );
	void DrawArc(
		int x, int y,
		int a, int b,
		double dStart, double dEnd,
		tRABY uColor,
		UINT uFlag
	);
	void DrawArc(
		int x, int y,
		int a, int b,
		int c, int d,
		double dStart, double dEnd,
		tRABY uColor,
		UINT uFlag
	);
	
	int DrawFont0( int x, int y, UCHAR c, CVsdFont &Font, tRABY uColor );
	int DrawFont( int x, int y, UCHAR c, CVsdFont &Font, tRABY uColor, tRABY uColorOutline = color_black );
	void DrawText( // !js_func
		int x, int y, char *szMsg, CVsdFont &Font, tRABY uColor,
		tRABY uColorOutline = color_black	// !default:color_black
	);
	void DrawTextAlign( // !js_func
		int x, int y, UINT uAlign, char *szMsg, CVsdFont &Font, tRABY uColor,
		tRABY uColorOutline = color_black	// !default:color_black
	);
	
	void DrawGraph(
		int x1, int y1, int x2, int y2,
		char *szFormat,
		CVsdFont &Font,
		tRABY uColor,
		CVsdLog& Log,
		double ( *GetDataFunc )( CVsdLog&, int ),
		double dMaxVal
	);
	void DrawGraph(	// !js_func
		int x1, int y1, int x2, int y2,
		CVsdFont &Font
	);
	
	// �|���S���`��
	void InitPolygon( void );			// !js_func
	void DrawPolygon( tRABY uColor );	// !js_func
	void DrawPolygon( const PIXEL_YCA_ARG yc );
	
	UINT BlendColor(
		tRABY uColor0,
		tRABY uColor1,
		double	dAlfa
	);
	
	BOOL DrawVSD( void );
	void DrawGSnake( // !js_func
		int iCx, int iCy, int iR, int iIndicatorR, int iWidth,
		tRABY uColorBall, tRABY uColorLine
	);
	
	void DrawMeterScale(	// !js_func
		int iCx, int iCy, int iR,
		int iLineLen1, int iLineWidth1, tRABY uColorLine1,
		int iLineLen2, int iLineWidth2, tRABY uColorLine2,
		int iLine2Cnt,
		int iMinDeg, int iMaxDeg,
		int iRNum,
		int iMaxVal, int iMaxNumCnt, tRABY uColorNum,
		CVsdFont &Font
	);
	void DrawMap(	// !js_func
		int x1, int y1, int x2, int y2,
		UINT uAlign,
		int iWidth,
		int iIndicatorR,
		tRABY uColorIndicator,
		tRABY uColorG0,
		tRABY uColorGPlus,
		tRABY uColorGMinus
	);
	void DrawLapTime( // !js_func
		int x, int y, UINT uAlign, CVsdFont &Font,
		tRABY uColor, tRABY uColorOutline, tRABY uColorBest, tRABY uColorPlus
	);
	void DrawNeedle( // !js_func
		int x, int y, int r1, int r2,
		int iStart, int iEnd, double dVal,
		tRABY uColor,
		int iWidth // !default:1
	);
	
	// ���b�v�^�C�����
	char *FormatTime( int iTime ); // !js_func
	
	int CurTime( void ){ return m_LapLog ? m_LapLog->m_iCurTime : TIME_NONE; }	// !js_var:CurTime
	int BestLapTime( void ){ return m_LapLog ? m_LapLog->m_iBestTime : TIME_NONE; }	// !js_var:BestLapTime
	int DiffTime( void ){ return m_LapLog ? m_LapLog->m_iDiffTime : TIME_NONE; }	// !js_var:DiffTime
	
	void DispErrorMessage( const char *szMsg );
	
	enum {
		IMG_FILL	= ( 1 << 0 ),
	};
	
	enum {
		ALIGN_LEFT		= 0,
		ALIGN_TOP		= 0,
		ALIGN_HCENTER	= 1,
		ALIGN_RIGHT		= 2,
		ALIGN_VCENTER	= 4,
		ALIGN_BOTTOM	= 8,
	};
	
	// ���z�֐�
	virtual int	GetWidth( void )	= 0;	// !js_var:Width
	virtual int	GetHeight( void )	= 0;	// !js_var:Height
	virtual int	GetFrameMax( void )	= 0;	// !js_var:MaxFrameCnt
	virtual int	GetFrameCnt( void )	= 0;	// !js_var:FrameCnt
	virtual double GetFPS( void )	= 0;
	
	char	*m_szLogFile;
	char	*m_szGPSLogFile;
	char	*m_szSkinFile;
	char	*m_szSkinDir;	// !js_var:SkinDir
	char	*m_szPluginDir;	// !js_var:VsdRootDir
	
	// ���O���[�h�w���p
	int ReadGPSLog( const char *szFileName );
	int ReadLog( const char *szFileName );
	double LapNum2LogNum( CVsdLog *Log, int iLapNum );
	CLapLog *CreateLapTime( int iLapMode );
	CLapLog *CreateLapTimeAuto( void );
	void CalcLapTime( void );
	
	static char *StringNew( char *&szDst, const char *szSrc ){
		if( szDst ) delete [] szDst;
		
		if( szSrc == NULL || *szSrc == '\0' ){
			return szDst = NULL;
		}
		szDst = new char[ strlen( szSrc ) + 1 ];
		return strcpy( szDst, szSrc );
	}
	
	// �X�L�� dir �擾
	char *SetSkinFile( const char *szSkinFile ){
		char szBuf[ MAX_PATH + 1 ];
		
		if( szSkinFile == NULL ) return StringNew( m_szSkinDir, NULL );
		
		// �X�L���t�@�C������ CWD=m_szPluginDir �Ƃ݂Ȃ��t���p�X�ɕϊ�
		GetFullPathWithCDir( szBuf, szSkinFile, m_szPluginDir );
		
		// ���̃f�B���N�g�����𓾂�
		StringNew( m_szSkinFile, szBuf );
		return StringNew( m_szSkinDir, StrTokFile( szBuf, m_szSkinFile, STF_FULL | STF_PATH2 ));
	}
	
	// �v���O�C�� dll dir �擾
	char *SetPluginDir( void ){
		char szBuf[ MAX_PATH + 1 ];
		GetModuleFileName(( HMODULE )m_hInst, szBuf, MAX_PATH );
		char *p = StrTokFile( NULL, szBuf, STF_NODE );
		if( p ) strcpy( p, "vsd_skins\\" );
		return StringNew( m_szPluginDir, szBuf );
	}
	
	int		*m_piParamT;
	int		*m_piParamC;
	int		*m_piParamS;
	
	// �t�H���g
	CVsdFont	*m_pFont;
	
	CVsdLog	*m_VsdLog;
	CVsdLog	*m_GPSLog;
	CVsdLog *m_CurLog;
	CLapLog	*m_LapLog;
	
	BOOL ParseMarkStr( const char *szMark );
	
	BOOL		m_bCalcLapTimeReq;
	
	// JavaScript �p�p�����[�^
	double	m_dSpeed;		// !js_var:Speed
	double	m_dTacho;		// !js_var:Tacho
	double	m_dGx;			// !js_var:Gx
	double	m_dGy;			// !js_var:Gy
	
	int	GetMaxSpeed( void ){ // !js_var:MaxSpeed
		return m_CurLog ? m_CurLog->m_iMaxSpeed : 180;
	}
	int	GetMaxTachk( void ){ // !js_var:MaxTacho
		return m_CurLog ? m_CurLog->m_iMaxTacho : 0;
	}
	
	static HINSTANCE	m_hInst;	// dll handle
	
  protected:
	
	CScript	*m_Script;
	
  private:
	
	// �X�^�[�g���C��@GPS �v�����[�h
	double	m_dStartLineX1;
	double	m_dStartLineY1;
	double	m_dStartLineX2;
	double	m_dStartLineY2;
	
	virtual void SetFrameMark( int iFrame ) = 0;
	virtual int  GetFrameMark( int iFrame ) = 0;
	
	int m_iPosX, m_iPosY;
	
	PolygonData_t	*m_Polygon;
	
	// �𑜓x�ύX���o�p
	int	m_iWidth;
	int m_iHeight;
};
#endif
