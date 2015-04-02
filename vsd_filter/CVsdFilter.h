/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFilter.h - CVsdFilter class header
	
*****************************************************************************/

#pragma once

#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CScript.h"
#include "CVsdImage.h"
#include "pixel.h"

#ifdef AVS_PLUGIN
	#include "avisynth.h"
#else
	#include "filter.h"
#endif

/****************************************************************************/

#define PROG_NAME		"VSDFilter"
#define PROG_NAME_J		"VSDメーター合成"
#define PROG_NAME_J_W	L"VSDメーター合成"
#define PROG_NAME_LONG	"`VSDFilter' vehicle data logger overlay plugin"

#define G_CX_CNT		30
#define POS_DEFAULT		0x80000000

#ifdef PUBLIC_MODE
	#define VideoSt			m_piParamS[ PARAM_VSt ]
	#define VideoEd			m_piParamS[ PARAM_VEd ]
	#define VsdSt			m_piParamS[ PARAM_LSt ]
	#define VsdEd			m_piParamS[ PARAM_LEd ]
	#define GPSSt			m_piParamS[ PARAM_GSt ]
	#define GPSEd			m_piParamS[ PARAM_GEd ]
	#define DEFAULT_SKIN	"standard_l.js"
#else
	#define VideoSt			m_piParamT[ PARAM_VSt ]
	#define VideoEd			m_piParamT[ PARAM_VEd ]
	#define VsdSt			m_piParamT[ PARAM_LSt ]
	#define VsdEd			m_piParamT[ PARAM_LEd ]
	#define GPSSt			m_piParamT[ PARAM_GSt ]
	#define GPSEd			m_piParamT[ PARAM_GEd ]
	#define DEFAULT_SKIN	"standard_r.js"
#endif

#define PLUGIN_DIR		"vsd_plugins"
#define LOG_READER_DIR	"_log_reader"

// パラメータを変換
#define GetLogIndex( frame, to, prev ) \
	m_##to##Log->GetIndex( frame, &VideoSt, &to##St, prev )

// VSD log を優先，ただしチェックボックスでオーバーライドできる
#define SelectLogVsd ( m_CurLog = ( GPSPriority && m_GPSLog || !m_VsdLog ) ? m_GPSLog : m_VsdLog )

// GPS log を優先
#define SelectLogGPS ( m_CurLog = m_GPSLog ? m_GPSLog : m_VsdLog )

// Laptime 計算用
#define SelectLogForLapTime	( m_CurLog = \
	m_LapLog && m_LapLog->m_iLapSrc == LAPSRC_VSD \
	? m_VsdLog : m_GPSLog )

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

/* 辺の定義 */
struct Edge {
	double	dx, dy;	// 座標
	int		x, y;	// 始点座標
	USHORT	Flag;
};

class CVsdFilter
	#ifdef AVS_PLUGIN
		: public GenericVideoFilter
	#endif
{
  public:
	~CVsdFilter();
	void Constructor( void );
	void Destructor( void );
	
	/*** 画像オペレーション *************************************************/
	
	void PutPixel(	// !js_func
		int x, int y, PIXEL_YCA_ARG yc,
		UINT uFlag	// !default:0
	);
	void PutPixel( int x, int y, PIXEL_YCA_ARG yc );
	void FillLine( int x1, int y1, int x2, PIXEL_YCA_ARG yc, UINT uPattern = 0xFFFFFFFF );
	
	UINT PutImage(	// !js_func
		int x, int y, CVsdImage &img,
		UINT uAlign = 0,		// !default:0
		int iImgX = 0,			// !default:0
		int iImgY = 0,			// !default:0
		int iImgW = INT_MIN,	// !default:INT_MIN
		int iImgH = INT_MIN		// !default:INT_MIN
	);
	// 座標，width 等補正後
	UINT PutImage0(
		int x, int y, CVsdImage &img,
	int ix_st, int iy_st, int ix_ed, int iy_ed
	);
	
	void DrawLine( int x1, int y1, int x2, int y2, PIXEL_YCA_ARG yc, UINT uPattern = 0xFFFFFFFF );
	void DrawLine(		// !js_func
		int x1, int y1, int x2, int y2,
		int width,		// !arg:5 !default:1
		PIXEL_YCA_ARG yc,	// !arg:4
		UINT uPattern = 0xFFFFFFFF	// !default:0xFFFFFFFF
	);
	
	void DrawRect(	// !js_func
		int x1, int y1, int x2, int y2,
		PIXEL_YCA_ARG yc,
		UINT uFlag	// !default:0
	);
	void DrawCircle(	// !js_func
		int x, int y, int r,
		PIXEL_YCA_ARG yc,
		UINT uFlag		// !default:0
	);
	void DrawCircle( int x, int y, int a, int b, PIXEL_YCA_ARG yc, UINT uFlag );
	void DrawArc(
		int x, int y,
		int a, int b,
		double dStart, double dEnd,
		PIXEL_YCA_ARG yc,
		UINT uFlag
	);
	void DrawArc(
		int x, int y,
		int a, int b,
		int c, int d,
		double dStart, double dEnd,
		PIXEL_YCA_ARG yc
	);
	
	int DrawFont0( int x, int y, WCHAR c, CVsdFont &Font, PIXEL_YCA_ARG yc );
	int DrawFont( int x, int y, WCHAR c, CVsdFont &Font, PIXEL_YCA_ARG yc, PIXEL_YCA_ARG ycOutline = color_black );
	void DrawText( // !js_func
		int x, int y, LPCWSTR szMsg, CVsdFont &Font,
		PIXEL_YCA_ARG yc,						// !default:color_white
		PIXEL_YCA_ARG ycOutline = color_black	// !default:color_black
	);
	void DrawTextAlign( // !js_func
		int x, int y, UINT uAlign, LPCWSTR szMsg, CVsdFont &Font,
		PIXEL_YCA_ARG yc,						// !default:color_white
		PIXEL_YCA_ARG ycOutline = color_black	// !default:color_black
	);
	
	void DrawGraphSingle(	// !js_func
		int x1, int y1, int x2, int y2,
		char *szKey,
		LPCWSTR szFormat,
		CVsdFont &Font,
		PIXEL_YCA_ARG yc
	);
	
	void DrawGraphSub(
		int x1, int y1, int x2, int y2,
		LPCWSTR szFormat,
		CVsdFont &Font,
		PIXEL_YCA_ARG yc,
		CVsdLog& Log,
		CLog	&Data
	);
	
	enum {
		GRAPH_HTILE	= 1 << 30,
		GRAPH_VTILE	= 1 << 31,
	};
	
	void DrawSyncGraph(	// !js_func
		int x1, int y1, int x2, int y2,
		CVsdFont &Font
	);
	
	void DrawSyncInfo( // !js_func
		int x, int y, CVsdFont &Font,
		UINT uAlign = 0	// !default:0
	);
	
	// ポリゴン描写
	void InitPolygon( void );
	void FillPolygon( PIXEL_YCA_ARG yc );
	void DrawPolygon( // !js_func
		v8Array pixs,
		PIXEL_YCA_ARG yc,
		UINT uFlag	// !default:0
	);
	
	void DrawPolygon( UINT uEdgeCnt, Edge *EdgeList, PIXEL_YCA_ARG yc );
	void DrawPolygon( UINT uEdgeCnt, Edge *EdgeList, int iMinY, int iMaxY, PIXEL_YCA_ARG yc );
	
	UINT BlendColor(
		PIXEL_YCA_ARG yc0,
		PIXEL_YCA_ARG yc1,
		double	dAlfa
	);
	
	BOOL DrawVSD( void );
	void DrawGSnake( // !js_func
		int iCx, int iCy, int iR, int iIndicatorR, int iWidth,
		PIXEL_YCA_ARG ycBall, PIXEL_YCA_ARG ycLine,
		double dLength	// !default:3
	);
	
	void DrawRoundMeterScaleSub(	// !js_func
		int iCx, int iCy, int iR,
		int iLineLen1, int iLineWidth1, PIXEL_YCA_ARG ycLine1, int iLine1Cnt,
		int iLineLen2, int iLineWidth2, PIXEL_YCA_ARG ycLine2, int iLine2Cnt,
		int iMinDeg, int iMaxDeg,
		int iMinVal, int iMaxVal,
		int iRNum, PIXEL_YCA_ARG ycNum, CVsdFont &Font
	);
	
	void DrawLinearMeterScaleSub(	// !js_func
		UINT uFlag,
		int iX, int iY, int iWidth,
		int iLineLen1, int iLineWidth1, PIXEL_YCA_ARG ycLine1, int iLine1Cnt,
		int iLineLen2, int iLineWidth2, PIXEL_YCA_ARG ycLine2, int iLine2Cnt,
		int iMinVal, int iMaxVal,
		int iNumPos, PIXEL_YCA_ARG ycNum, CVsdFont &Font
	);
	
	void DrawMap(	// !js_func
		int x1, int y1, int x2, int y2,
		UINT uFlag,
		int iWidth,
		int iIndicatorR,
		PIXEL_YCA_ARG ycIndicator,
		PIXEL_YCA_ARG ycG0,
		PIXEL_YCA_ARG ycGPlus,
		PIXEL_YCA_ARG ycGMinus,
		int	iLength = INVALID_INT	// !default:INVALID_INT
	);
	void DrawMapPosition(	// !js_func
		int x1, int y1, int x2, int y2,	UINT uFlag,
		int iLineWidth, PIXEL_YCA_ARG yc,
		CVsdFont &Font,
		PIXEL_YCA_ARG ycFont,					// !default:color_white
		PIXEL_YCA_ARG ycOutline = color_black	// !default:color_black
	);
	void DrawLapTime(	// !js_func
		int x, int y, UINT uAlign, CVsdFont &Font,
		PIXEL_YCA_ARG yc,			// !default:color_white
		PIXEL_YCA_ARG ycBest,		// !default:color_cyan
		PIXEL_YCA_ARG ycPlus,		// !default:color_red
		PIXEL_YCA_ARG ycOutline		// !default:color_black
	);
	void DrawLapTimeLog(	// !js_func
		int x, int y, UINT uAlign, int iNum, CVsdFont &Font,
		PIXEL_YCA_ARG yc,			// !default:color_white
		PIXEL_YCA_ARG ycBest,		// !default:color_cyan
		PIXEL_YCA_ARG ycOutline		// !default:color_black
	);
	void DrawRaceLapTime(	// !js_func
		int x, int y, UINT uAlign, int iNum, CVsdFont &Font,
		PIXEL_YCA_ARG yc,			// !default:color_white
		PIXEL_YCA_ARG ycOutline		// !default:color_black
	);
	void DrawNeedle( // !js_func
		int x, int y, int r1, int r2,
		int iStart, int iEnd, double dVal,
		PIXEL_YCA_ARG yc,
		int iWidth // !default:1
	);
	
	// ラップタイム情報
	LPCWSTR FormatTime( int iTime ); // !js_func
	
	int CurTime( void ){ return m_LapLog ? m_LapLog->m_iCurTime : TIME_NONE; }	// !js_var:ElapsedTime
	int BestLapTime( void ){ return m_LapLog ? m_LapLog->m_iBestTime : TIME_NONE; }	// !js_var:BestLapTime
	int DiffTime( void ){ return m_LapLog ? m_LapLog->m_iDiffTime : TIME_NONE; }	// !js_var:DiffTime
	int LapTime( void ){	// !js_var:LapTime
		if( !m_LapLog || m_LapLog->m_iLapIdx < 0 ) return TIME_NONE;
		if( m_LapLog->m_Lap[ m_LapLog->m_iLapIdx + 1 ].iTime ){
			return m_LapLog->m_Lap[ m_LapLog->m_iLapIdx + 1 ].iTime;
		}
		return m_LapLog->m_Lap[ m_LapLog->m_iLapIdx ].iTime;
	}
	int LapCnt( void ){ // !js_var:LapCnt
		if( !m_LapLog ) return 0;
		return m_LapLog->m_Lap[ m_LapLog->m_iLapIdx + 1 ].uLap;
	}
	int MaxLapCnt( void ){ // !js_var:MaxLapCnt
		return m_LapLog ? m_LapLog->m_Lap[ m_LapLog->m_iLapNum - 1 ].uLap : 0;
	}
	
	void DispErrorMessage( LPCWSTR szMsg );
	
	enum {
		IMG_FILL		= ( 1 << 0 ),
		IMG_NOCLOSE		= ( 1 << 1 ),
	};
	
	enum {
		ALIGN_LEFT		= 0,
		ALIGN_TOP		= 0,
		ALIGN_HCENTER	= 1 << 0,
		ALIGN_RIGHT		= 1 << 1,
		ALIGN_VCENTER	= 1 << 2,
		ALIGN_BOTTOM	= 1 << 3,
		DRAW_MAP_START	= 1 << 4,	// DrawMap 専用フラグ
		LMS_VERTICAL	= 1 << 4,	// DrawLinearMeterScale 専用フラグ
	};
	
	// ダイアログ設定リード
	
	int LineTrace( void ){ return m_piParamT[ TRACK_LineTrace ]; }	// !js_var:Config_map_length
	int DispLap( void ){ return m_piParamC[ CHECK_LAP ]; }			// !js_var:Config_lap_time
	int DispGraph( void ){ return m_piParamC[ CHECK_GRAPH ]; }		// !js_var:Config_graph
	
	char	*m_szLogFile;
	char	*m_szLogFileReader;
	char	*m_szGPSLogFile;
	char	*m_szGPSLogFileReader;
	char	*m_szLapChart;
	char	*m_szSkinFile;
	char	*m_szSkinDirA;
	LPWSTR	m_szSkinDirW;	// !js_const:SkinDir
	char	*m_szPluginDirA;
	LPWSTR	m_szPluginDirW;	// !js_const:VsdRootDir
	
	// リビジョン番号
	static const int	m_iRevision;	// !js_const:Revision
	
	// ログリードヘルパ
	int ReadLog( CVsdLog *&pLog, const char *szFileName, const char *szReaderFunc );
	double LapNum2LogNum( CVsdLog *Log, int iLapNum );
	CLapLog *CreateLapTimeHand( int iLapSrc );
	CLapLog *CreateLapTimeAuto( void );
	void CalcLapTime( void );
	
	// ラップチャートリーダ
	int LapChartRead( const char *szFileName );
	
	// スキン dir 取得
	char *SetSkinFile( const char *szSkinFile ){
		char szBuf[ MAX_PATH + 1 ];
		
		if( szSkinFile == NULL ){
			StringNew( m_szSkinDirW, ( LPCWSTR )NULL );
			return StringNew( m_szSkinDirA, NULL );
		}
		
		StringNew( m_szSkinFile, szSkinFile );
		
		// スキンファイル名を CWD=m_szPluginDirA とみなしフルパスに変換
		GetFullPathWithCDir( szBuf, szSkinFile, m_szPluginDirA );
		
		// ↑のディレクトリ名を得る
		StringNew( m_szSkinDirA, StrTokFile( szBuf, szBuf, STF_FULL | STF_PATH2 ));
		StringNew( m_szSkinDirW, m_szSkinDirA );
		return m_szSkinDirA;
	}
	
	// プラグイン dll dir 取得
	char *SetPluginDir( void ){
		char szBuf[ MAX_PATH + 1 ];
		GetModuleFileName(( HMODULE )m_hInst, szBuf, MAX_PATH );
		char *p = StrTokFile( NULL, szBuf, STF_NODE );
		if( p ) strcpy( p, PLUGIN_DIR "\\" );
		StringNew( m_szPluginDirA, szBuf );
		StringNew( m_szPluginDirW, m_szPluginDirA );
		return m_szPluginDirA;
	}
	
	int		*m_piParamT;
	int		*m_piParamC;
	int		*m_piParamS;
	
	// フォント
	CVsdFont	*m_pFont;
	
	CVsdLog	*m_VsdLog;
	CVsdLog	*m_GPSLog;
	CVsdLog *m_CurLog;
	CLapLog	*m_LapLog;
	
	BOOL ParseMarkStr( const char *szMark );
	
	BOOL		m_bCalcLapTimeReq;
	
	// JavaScript 用パラメータ
	
	#define DEF_LOG( name ) double Get##name( void ){ \
		if( m_VsdLog && m_VsdLog->m_pLog##name ) return m_VsdLog->name(); \
		return m_GPSLog->name(); \
	}
	#include "def_log.h"
	
	double DateTime( void ){	// !js_var:DateTime
		if( m_VsdLog ) return m_VsdLog->DateTime();
		if( m_GPSLog ) return m_GPSLog->DateTime();
		return NaN;
	}
	
	double GetValue( char *szKey ){
		double dRet;
		if( m_VsdLog && !_isnan( dRet = m_VsdLog->Get( szKey, m_VsdLog->m_dLogNum )))
			return dRet;
		if( m_GPSLog && !_isnan( dRet = m_GPSLog->Get( szKey, m_GPSLog->m_dLogNum )))
			return dRet;
		
		return NaN;
	}
	
	void InitJS( v8::Local<v8::FunctionTemplate> tmpl );
	void InitJS_Sub( CVsdLog *pLog, v8::Local<v8::FunctionTemplate> tmpl );
	
	static HINSTANCE	m_hInst;	// dll handle
	
	void DeleteScript( void ){
		if( m_Script ){
			m_Script->Dispose();
			delete m_Script;
			m_Script = NULL;
		}
	}
	
	static void OpenConsole( void );
	static void Print( LPCWSTR szMsg );
	static void Print( const char *szFormat, ... );
	
	BOOL	m_bConsoleOn;
	
  protected:
	
	CScript	*m_Script;
	int m_iTextPosX, m_iTextPosY;
	
  private:
	// GDI+オブジェクト（画像展開に必要）
	Gdiplus::GdiplusStartupInput	m_gdiplusStartupInput;
	ULONG_PTR						m_gdiplusToken;
	
	// スタートライン@GPS 計測モード
	double	m_dStartLineX1;
	double	m_dStartLineY1;
	double	m_dStartLineX2;
	double	m_dStartLineY2;
	
	void SetFrameMark( int iFrame );
	int  GetFrameMark( int iFrame );
	
	PolygonData_t	*m_Polygon;
	int m_iPolygonMinY;
	int m_iPolygonMaxY;
	
	// 解像度変更検出用
	int	m_iWidth;
	int m_iHeight;
	
#ifdef AVS_PLUGIN
	/*** AviSynth ***********************************************************/
  public:
	CVsdFilter(
		PClip _child,
		AVSValue args,
		IScriptEnvironment* env
	);
	
	PVideoFrame __stdcall GetFrame( int n, IScriptEnvironment* env );
	
	/////////////
	
	int GetIndex( int x, int y ){ return m_iBytesPerLine * y + x * 2; }
	
	// 仮想関数
	void PutPixel( int iIndex, PIXEL_YCA_ARG yc, int iAlfa );
	
	int	GetWidth( void )	{ return m_SrcFrame->GetRowSize()>>1; }	// !js_var:Width
	int	GetHeight( void )	{ return m_SrcFrame->GetHeight(); }	// !js_var:Height
	int	GetFrameMax( void )	{ return vi.num_frames; }	// !js_var:MaxFrameCnt
	int	GetFrameCnt( void )	{ return m_iFrameCnt; }	// !js_var:FrameCnt
	double	GetFPS( void )	{ return ( double )vi.fps_numerator / vi.fps_denominator; }
	
	// パラメータ
	int	m_iFrameCnt, m_iFrameMax;
	int m_iBytesPerLine;
	std::vector<int> m_iMark;
	int m_iMarkCnt;
	
	BYTE	*m_pPlane;
	const char *m_szMark;
	IScriptEnvironment* m_env;
	PVideoFrame	m_SrcFrame;
#else
	/*** AviUtl *************************************************************/
	
  public:
	void				*editp;
	FILTER				*filter;
	FILE_INFO			*fileinfo;
	FILTER_PROC_INFO	*fpip;
	
	CVsdFilter( FILTER *filter, void *editp );
	
	int GetIndex( int x, int y ){ return fpip->max_w * y + x; }
	BOOL ConfigSave( const char *szFileName );
	
	BOOL ReadVsdLog( HWND hwnd );
	BOOL ReadGPSLog( HWND hwnd );
	
	char *IsConfigParamStr( const char *szParamName, char *szBuf, char *&szDst );
	char *IsConfigParam( const char *szParamName, char *szBuf, int &iVal );
	BOOL ConfigLoad( const char *szFileName );
	void SetSkinName( char *szSkinFile, HWND hwnd );
	
	// ログリーダ用
	BOOL CreateFilter( void );
	BOOL FileOpenDialog( char *&szOut, char *&szReaderFunc );
	char *m_szLogFilter;
	std::vector<std::string> m_vecReaderFunc;
	
	// 仮想関数
	int	GetWidth( void ){ return fpip ? fpip->w : 0; }
	int	GetHeight( void ){ return fpip ? fpip->h : 0; }
	int	GetFrameMax( void ){ return fileinfo->frame_n; }
	int	GetFrameCnt( void ){ return fpip->frame; }
	double	GetFPS( void ){ return ( double )fileinfo->video_rate / fileinfo->video_scale; }
	
	// 同期情報表示
	WCHAR *DrawSyncInfoFormatTime(
		WCHAR	*pBuf, UINT uBufSize, INT64 &iBaseTime, int iTime
	);
	void DrawSyncInfoSub(
		int x, int y, CVsdFont &Font,
		LPCWSTR	szCaption,
		INT64	&iBaseTime,
		int		iCurTime,
		int		iStartTime,
		int		iEndTime
	);
	
	// 1スライダ調整用パラメータ
	int	m_iAdjustPointNum;
	int	m_iAdjustPointVid[ 2 ];
	int	m_iAdjustPointVsd[ 2 ];
	int	m_iAdjustPointGPS[ 2 ];
	
	static const char *m_szTrackbarName[];
	static const char *m_szCheckboxName[];
	static const char *m_szShadowParamName[];
	
	void GetFileCreationTime( void );
#ifdef PUBLIC_MODE
	void AutoSync( CVsdLog *pLog, int *piParam );
#endif
	
  private:
	INT64	m_iVideoStartTime;
#endif
};

BOOL ListTree( LPTSTR szPath, LPCTSTR szFile, BOOL ( *CallbackFunc )( LPCTSTR, LPCTSTR, void * ), void *pParam );

class CPushDir {
  public:
	CPushDir( char *szPath ){
		int iBufSize = GetCurrentDirectory( 0, NULL );
		m_szCurDir = new char[ iBufSize ];
		GetCurrentDirectory( iBufSize, m_szCurDir );
		SetCurrentDirectory( szPath );
	}
	
	~CPushDir(){
		SetCurrentDirectory( m_szCurDir );
		delete [] m_szCurDir;
	}
	
  private:
	char	*m_szCurDir;
};
