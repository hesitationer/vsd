/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter.cpp - VSD filter for AviUti
	$Id$
	
*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "dds.h"
#include "../vsd/main.h"
#include "filter.h"
#include "dds_lib/dds_lib.h"
#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CVsdFilter.h"

/*** macros *****************************************************************/

#ifdef GPS_ONLY
	#define CONFIG_EXT		"cfg"
#else
	#define CONFIG_EXT		"avs"
#endif

#define	FILE_LOG_EXT	"log file (*.log)\0*.log; *.gz\0AllFile (*.*)\0*.*\0"
#define	FILE_GPS_EXT	"GPS file (*.nme* *.dp3)\0*.nme*; *.dp3; *.gz\0AllFile (*.*)\0*.*\0"
#define	FILE_CFG_EXT	"Config File (*." CONFIG_EXT ")\0*." CONFIG_EXT "\0AllFile (*.*)\0*.*\0"

/*** new type ***************************************************************/

/*** const ******************************************************************/

enum {
	ID_BUTT_SET_VSt = 0xFF00,
	ID_BUTT_SET_VEd,
#ifndef GPS_ONLY
	ID_BUTT_SET_LSt,
	ID_BUTT_SET_LEd,
#endif
	ID_BUTT_SET_GSt,
	ID_BUTT_SET_GEd,
#ifndef GPS_ONLY
	ID_EDIT_LOAD_LOG,
	ID_BUTT_LOAD_LOG,
#endif
	ID_EDIT_LOAD_GPS,
	ID_BUTT_LOAD_GPS,
	ID_EDIT_SEL_FONT,
	ID_BUTT_SEL_FONT,
	ID_BUTT_LOAD_CFG,
	ID_BUTT_SAVE_CFG,
};

#define POS_TH_SLIDER			220
#define POS_TH_EDIT				299
#define POS_ADD_SLIDER			300
#define POS_ADD_EDIT			16
#define POS_SET_BUTT_SIZE		30

#define POS_FILE_CAPTION_SIZE	55
#define POS_FILE_NAME_SIZE		350
#define POS_FILE_BUTT_SIZE		40
#define POS_FILE_CAPTION_POS	( rectClient.right - ( POS_FILE_NAME_SIZE + POS_FILE_BUTT_SIZE + POS_FILE_CAPTION_SIZE ))
#define POS_FILE_HEIGHT			18
#define POS_FILE_HEIGHT_MARGIN	2

#ifdef GPS_ONLY
	#define POS_FILE_NUM	3
#else
	#define POS_FILE_NUM	4
#endif

/*** gloval var *************************************************************/

HINSTANCE		g_hInst 	= NULL;

/****************************************************************************/
//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------

// トラックバーの名前
TCHAR	*track_name[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	name,
	#include "def_trackbar.h"
};
// トラックバーの初期値
int		track_default[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	init,
	#include "def_trackbar.h"
};
// トラックバーの下限値
int		track_s[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	min,
	#include "def_trackbar.h"
};
// トラックバーの上限値
int		track_e[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	max,
	#include "def_trackbar.h"
};

// チェックボックスの名前
TCHAR	*check_name[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name )	name,
	#include "def_checkbox.h"
};

// チェックボックスの初期値 (値は0か1)
int		check_default[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name )	init,
	#include "def_checkbox.h"
};

// シャドゥの初期値
int		shadow_default[] = {
	#define DEF_SHADOW( id, init, conf_name )	init,
	#include "def_shadow.h"
};

int		shadow_param[] = {
	#define DEF_SHADOW( id, init, conf_name )	init,
	#include "def_shadow.h"
};

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_MAIN_MESSAGE,
								//	フィルタのフラグ
								//	FILTER_FLAG_ALWAYS_ACTIVE		: フィルタを常にアクティブにします
								//	FILTER_FLAG_CONFIG_POPUP		: 設定をポップアップメニューにします
								//	FILTER_FLAG_CONFIG_CHECK		: 設定をチェックボックスメニューにします
								//	FILTER_FLAG_CONFIG_RADIO		: 設定をラジオボタンメニューにします
								//	FILTER_FLAG_EX_DATA				: 拡張データを保存出来るようにします。
								//	FILTER_FLAG_PRIORITY_HIGHEST	: フィルタのプライオリティを常に最上位にします
								//	FILTER_FLAG_PRIORITY_LOWEST		: フィルタのプライオリティを常に最下位にします
								//	FILTER_FLAG_WINDOW_THICKFRAME	: サイズ変更可能なウィンドウを作ります
								//	FILTER_FLAG_WINDOW_SIZE			: 設定ウィンドウのサイズを指定出来るようにします
								//	FILTER_FLAG_DISP_FILTER			: 表示フィルタにします
								//	FILTER_FLAG_EX_INFORMATION		: フィルタの拡張情報を設定できるようにします
								//	FILTER_FLAG_NO_CONFIG			: 設定ウィンドウを表示しないようにします
								//	FILTER_FLAG_AUDIO_FILTER		: オーディオフィルタにします
								//	FILTER_FLAG_RADIO_BUTTON		: チェックボックスをラジオボタンにします
								//	FILTER_FLAG_WINDOW_HSCROLL		: 水平スクロールバーを持つウィンドウを作ります
								//	FILTER_FLAG_WINDOW_VSCROLL		: 垂直スクロールバーを持つウィンドウを作ります
								//	FILTER_FLAG_IMPORT				: インポートメニューを作ります
								//	FILTER_FLAG_EXPORT				: エクスポートメニューを作ります
	0,0,						//	設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
	"VSDメーター合成",			//	フィルタの名前
	TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
	track_name,					//	トラックバーの名前郡へのポインタ
	track_default,				//	トラックバーの初期値郡へのポインタ
	track_s, track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0〜256)
	CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
	check_name,					//	チェックボックスの名前郡へのポインタ
	check_default,				//	チェックボックスの初期値郡へのポインタ
	func_proc,					//	フィルタ処理関数へのポインタ (NULLなら呼ばれません)
	NULL, //func_init,			//	開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL, //func_exit,			//	終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_update,				//	設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_WndProc,				//	設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,NULL,					//	システムで使いますので使用しないでください
	NULL,						//  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
	NULL,						//  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
	NULL,						//	セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	NULL,						//	セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void ){
	return &filter;
}

//---------------------------------------------------------------------
//		初期化
//---------------------------------------------------------------------

/*
BOOL func_init( FILTER *fp ){
	return TRUE;
}
*/

//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------

/*
BOOL func_exit( FILTER *fp ){
	return TRUE;
}
*/

//---------------------------------------------------------------------
//		フィルタ処理関数
//---------------------------------------------------------------------

/*** CVsdFilterAvu クラス ***************************************************/

class CVsdFilterAvu : public CVsdFilter {
	
  public:
	void	*editp;
	FILTER	*filter;
	FILTER_PROC_INFO *fpip;
	
	int GetIndex( int x, int y ){ return fpip->max_w * y + x; }
	BOOL ConfigSave( const char *szFileName );
	
#ifndef GPS_ONLY
	BOOL ReadLog( const char *szFileName, HWND hwnd );
#endif
	BOOL GPSLogLoad( const char *szFileName, HWND hwnd );
	
	// 仮想関数
	virtual void PutPixel( int x, int y, const PIXEL_YC &yc, UINT uFlag );
	
	virtual int	GetWidth( void ){ return fpip->w ; }
	virtual int	GetHeight( void ){ return fpip->h ; }
	virtual int	GetFrameMax( void ){ return fpip->frame_n ; }
	virtual int	GetFrameCnt( void ){ return fpip->frame ; }
	
	virtual void SetFrameMark( int iFrame );
	virtual int  GetFrameMark( int iFrame );
	
	virtual char *GetVideoFileName( char *szFileName );
};

CVsdFilterAvu	*g_Vsd;

/*** PutPixel ***************************************************************/

/* 変換式
Y  =  0.299R+0.587G+0.114B
Cr =  0.500R-0.419G-0.081B
Cb = -0.169R-0.332G+0.500B

R = Y+1.402Cr 
G = Y-0.714Cr-0.344Cb 
B = Y+1.772Cb 
*/

/*
inline void CVsdFilter::PutPixel( int x, int y, short iY, short iCr, short iCb ){
	int	iIndex = GetIndex( x, y );
	
	fpip->ycp_edit[ iIndex ].y  = iY;
	fpip->ycp_edit[ iIndex ].cr = iCr;
	fpip->ycp_edit[ iIndex ].cb = iCb;
}
*/

void CVsdFilterAvu::PutPixel( int x, int y, const PIXEL_YC &yc, UINT uFlag ){
	
	if( uFlag & IMG_POLYGON ){
		// ポリゴン描画
		if( x > m_Polygon[ y ].iRight ) m_Polygon[ y ].iRight = x;
		if( x < m_Polygon[ y ].iLeft  ) m_Polygon[ y ].iLeft  = x;
	}else{
		PIXEL_YC	*ycp = fpip->ycp_edit;
		
		if( uFlag & IMG_ALFA && yc.y == -1 ) return;
		
		if( 0 <= x && x < fpip->max_w && 0 <= y && y < fpip->max_h ){
			if( uFlag & IMG_ALFA ){
				int	iIndex = GetIndex( x, y );
				
				ycp[ iIndex ].y  = ( yc.y  + ycp[ iIndex ].y  ) / 2;
				ycp[ iIndex ].cr = ( yc.cr + ycp[ iIndex ].cr ) / 2;
				ycp[ iIndex ].cb = ( yc.cb + ycp[ iIndex ].cb ) / 2;
			}else{
				ycp[ GetIndex( x, y ) ] = yc;
			}
		}
	}
}

/*** フレームをマーク *******************************************************/

void CVsdFilterAvu::SetFrameMark( int iFrame ){
	FRAME_STATUS	fsp;
	
	filter->exfunc->get_frame_status( editp, iFrame, &fsp );
	fsp.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
	filter->exfunc->set_frame_status( editp, iFrame, &fsp );
}

int CVsdFilterAvu::GetFrameMark( int iFrame ){
	
	FRAME_STATUS	fsp;
	
	for( ; iFrame < GetFrameMax(); ++iFrame ){
		filter->exfunc->get_frame_status( editp, iFrame, &fsp );
		
		if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
			return iFrame;
		}
	}
	return -1;
}

/*** 編集ビデオファイル名取得 ***********************************************/

char *CVsdFilterAvu::GetVideoFileName( char *szFileName ){
	FILE_INFO	fi;
	filter->exfunc->get_file_info( editp, &fi );
	return strcpy( szFileName, fi.name );
}

/*** config セーブ **********************************************************/

BOOL CVsdFilterAvu::ConfigSave( const char *szFileName ){
	FILE	*fp;
	int		i;
	
	if(( fp = fopen( szFileName, "w" )) == NULL ) return FALSE;
	
	char szBuf[ BUF_SIZE ];
	
	FILE_INFO fi;
	filter->exfunc->get_file_info( editp, &fi );
	
	fprintf( fp,
		"DirectShowSource( \"%s\", pixel_type=\"YUY2\", fps=%d.0/%d )\n"
		"VSDFilter",
		GetVideoFileName( szBuf ),
		fi.video_rate, fi.video_scale
	);
	
	char cSep = '(';
	
	// str param に初期値設定
	#define DEF_STR_PARAM( id, var, init, conf_name ) \
		if( strcmp( var, init ) != 0 ){ \
			fprintf( fp, "%c \\\n\t" conf_name "=\"%s\"", cSep, var ); \
			cSep = ','; \
		}
	#include "def_str_param.h"
	
	for( i = 0; i < TRACK_N; ++i ){
		if(
			m_szTrackbarName[ i ] == NULL ||
			i > TRACK_GEd && m_piParamT[ i ] == track_default[ i ]
		) continue;
		
		fprintf(
			fp, "%c \\\n\t%s=%d", cSep, m_szTrackbarName[ i ], m_piParamT[ i ]
		);
		cSep = ',';
	}
	
	for( i = 0; i < CHECK_N; ++i ){
		if(
			m_szCheckboxName[ i ] == NULL ||
			m_piParamC[ i ] == check_default[ i ]
		) continue;
		
		fprintf(
			fp, ", \\\n\t%s=%d", m_szCheckboxName[ i ], m_piParamC[ i ]
		);
	}
	
	for( i = 0; i < SHADOW_N; ++i ){
		if(
			m_piParamS[ i ] == shadow_default[ i ]
		) continue;
		
		fprintf(
			fp, ", \\\n\t%s=%d", m_szShadowParamName[ i ], m_piParamS[ i ]
		);
	}
	
	// 手動ラップ計測マーク出力
	if( m_iLapMode != LAPMODE_MAGNET && m_iLapNum ){
		FRAME_STATUS	fsp;
		FILE_INFO		fip;
		BOOL			bFirst = TRUE;
		
		filter->exfunc->get_file_info( editp, &fip );
		
		// マークされているフレーム# を求める  GetFrameMax() はなぜか×
		for( i = 0; i < fip.frame_n; ++i ){
			filter->exfunc->get_frame_status( editp, i, &fsp );
			if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
				fprintf( fp, "%s%u", bFirst ? ", \\\n\tmark=\"" : ",", i );
				bFirst = FALSE;
			}
		}
		fputc( '"', fp );
	}
	
	fprintf( fp, " \\\n)\n" );
	
	fclose( fp );
	return TRUE;
}

/*** ログリード *************************************************************/

#ifndef GPS_ONLY
BOOL CVsdFilterAvu::ReadLog( const char *szFileName, HWND hwnd ){
	
	char szMsg[ BUF_SIZE ];
	
	if( !CVsdFilter::ReadLog( szFileName )){
		sprintf( szMsg, "ファイルがロードできません\n%s", szFileName );
		MessageBox( NULL,
			szMsg,
			"VSD",
			MB_OK | MB_ICONWARNING
		);
		return FALSE;
	}
	
	strcpy( m_szLogFile, szFileName );
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_LOG ), szFileName );
	
	// trackbar 設定
	track_e[ TRACK_LSt ] =
	track_e[ TRACK_LEd ] = g_Vsd->m_VsdLog->m_iCnt;
	
	return TRUE;
}
#endif

/*** GPS ログリード ********************************************************/

BOOL CVsdFilterAvu::GPSLogLoad( const char *szFileName, HWND hwnd ){
	
	char szMsg[ BUF_SIZE ];
	
	if( !CVsdFilter::GPSLogLoad( szFileName )){
		sprintf( szMsg, "ファイルがロードできません\n%s", szFileName );
		MessageBox( NULL,
			szMsg,
			"VSD",
			MB_OK | MB_ICONWARNING
		);
		return FALSE;
	}
	
	strcpy( m_szGPSLogFile, szFileName );
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_GPS ), szFileName );
	
	// trackbar 設定
	track_e[ TRACK_GSt ] =
	track_e[ TRACK_GEd ] = g_Vsd->m_GPSLog->m_iCnt;
	
	return TRUE;
}

/*** func_proc **************************************************************/

BOOL func_proc( FILTER *fp, FILTER_PROC_INFO *fpip ){
//
//	fp->track[n]			: トラックバーの数値
//	fp->check[n]			: チェックボックスの数値
//	fpip->w 				: 実際の画像の横幅
//	fpip->h 				: 実際の画像の縦幅
//	fpip->w					: 画像領域の横幅
//	fpip->h					: 画像領域の縦幅
//	fpip->ycp_edit			: 画像領域へのポインタ
//	fpip->ycp_temp			: テンポラリ領域へのポインタ
//	fpip->ycp_edit[n].y		: 画素(輝度    )データ (     0 〜 4095 )
//	fpip->ycp_edit[n].cb	: 画素(色差(青))データ ( -2048 〜 2047 )
//	fpip->ycp_edit[n].cr	: 画素(色差(赤))データ ( -2048 〜 2047 )
//
//  画素データは範囲外に出ていることがあります。
//  また範囲内に収めなくてもかまいません。
//
//	画像サイズを変えたいときは fpip->w や fpip->h を変えます。
//
//	テンポラリ領域に処理した画像を格納したいときは
//	fpip->ycp_edit と fpip->ycp_temp を入れ替えます。
//
	
	if( !g_Vsd ) return 0;
	// クラスに変換
	g_Vsd->filter	= fp;
	g_Vsd->fpip		= fpip;
	
	return g_Vsd->DrawVSD();
}

/*** ダイアログサイズ拡張とパーツ追加 ***************************************/

void CreateSubControl(
	HWND hwnd, int &iID, HFONT hfont, int iX, int &iY,
	char *szCap, char *szEdit, char *szButt
) {
	
	HWND hwndChild;
	
	hwndChild = CreateWindow(
		"STATIC", szCap, WS_CHILD | WS_VISIBLE,
		iX, iY,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"EDIT", szEdit, WS_CHILD | WS_VISIBLE | ES_READONLY,
		iX + POS_FILE_CAPTION_SIZE, iY,
		POS_FILE_NAME_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )iID, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", szButt, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		iX + POS_FILE_CAPTION_SIZE + POS_FILE_NAME_SIZE, iY,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( iID + 1 ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	iID += 2;
	iY += POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN;
}

void ExtendDialog( HWND hwnd ){
	
	union {
		struct {
			POINT	topleft, bottomright;
		} points;
		RECT	rect;
	} rect;
	RECT	rectClient;
	
	HWND	hwndChild	= NULL;
	HFONT	hfont		= NULL;
	
	// クライアント領域のサイズ get 後にリサイズ
	GetWindowRect( hwnd, &rectClient );
	MoveWindow( hwnd,
		rectClient.left, rectClient.top,
		rectClient.right  - rectClient.left + POS_ADD_SLIDER + POS_ADD_EDIT + POS_SET_BUTT_SIZE,
		rectClient.bottom - rectClient.top,
		TRUE
	);
	
	GetClientRect( hwnd, &rectClient );
	
	while( 1 ){
		/*** 子パーツのサイズ変更 ***/
		hwndChild = FindWindowEx( hwnd, hwndChild, NULL, NULL );
		if( !hwndChild ) break;
		
		// screen -> client 座標に変換
		GetWindowRect( hwndChild, &rect.rect );
		ScreenToClient( hwnd, &rect.points.topleft );
		ScreenToClient( hwnd, &rect.points.bottomright );
		
		// ダイアログ左側を延ばす，EDIT ボックスのサイズを伸ばす
		if( rect.rect.right >= POS_TH_EDIT   ) rect.rect.right += POS_ADD_EDIT;
		if( rect.rect.left  >= POS_TH_SLIDER ) rect.rect.left  += POS_ADD_SLIDER;
		if( rect.rect.right >= POS_TH_SLIDER ) rect.rect.right += POS_ADD_SLIDER;
		
		// 実際にリサイズ
		MoveWindow( hwndChild,
			rect.rect.left,
			rect.rect.top,
			rect.rect.right  - rect.rect.left,
			rect.rect.bottom - rect.rect.top,
			TRUE
		);
		
		if( !hfont ) hfont = ( HFONT )SendMessage( hwndChild, WM_GETFONT, 0, 0 );
	}
	
	// 位置取得ボタン
	int i;
	for( i = 0; i <= ( ID_BUTT_SET_GEd - ID_BUTT_SET_VSt ); ++i ){
		hwndChild = CreateWindow(
			"BUTTON", "set", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			rectClient.right - POS_SET_BUTT_SIZE, 14 + i * 24,
			POS_SET_BUTT_SIZE, 16,
			hwnd, ( HMENU )( ID_BUTT_SET_VSt + i ), 0, NULL
		);
		SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	}
	
	// ログ名・フォント名
	i += ID_BUTT_SET_VSt;
	int y = rectClient.bottom - ( POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN ) * POS_FILE_NUM + POS_FILE_HEIGHT_MARGIN;
	
#ifndef GPS_ONLY
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y,	"VSDログ",	"",				"開く" );
#endif
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y,	"GPSログ",	"",				"開く" );
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y,	"フォント",	"(default)",	"選択" );
	
	// cfg load/save ボタン
	hwndChild = CreateWindow(
		"STATIC", "cfgファイル", WS_CHILD | WS_VISIBLE,
		rectClient.right - ( POS_FILE_BUTT_SIZE * 2 + POS_FILE_CAPTION_SIZE ), y,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", "開く", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectClient.right - ( POS_FILE_BUTT_SIZE * 2 ), y,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( i++ ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", "保存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectClient.right - POS_FILE_BUTT_SIZE, y,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( i++ ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
}

/*** WndProc ****************************************************************/

BOOL func_WndProc( HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *filter ){
	
	TCHAR	szBuf[ MAX_PATH ];
	TCHAR	szBuf2[ MAX_PATH ];
	int		iFrame;
	
	//	TRUEを返すと全体が再描画される
	
	if( message == WM_FILTER_INIT ) ExtendDialog( hwnd );
	
	//	編集中でなければ何もしない
	if( filter->exfunc->is_editing( editp ) != TRUE ) return FALSE;
	
	switch( message ) {
	  case WM_FILTER_FILE_OPEN:
		
		g_Vsd = new CVsdFilterAvu;
		
		g_Vsd->m_piParamT	= filter->track;
		g_Vsd->m_piParamC	= filter->check;
		g_Vsd->m_piParamS	= shadow_param;
		g_Vsd->filter		= filter;
		g_Vsd->editp		= editp;
		
		// fps 取得
		FILE_INFO fi;
		filter->exfunc->get_file_info( editp, &fi );
		g_Vsd->m_dVideoFPS  = ( double )fi.video_rate / fi.video_scale;
		
		g_Vsd->m_bCalcLapTimeReq = TRUE;
		
		// trackbar 設定
		track_e[ TRACK_VSt ] =
		track_e[ TRACK_VEd ] = filter->exfunc->get_frame_n( editp );
		
		// 設定再描画
		filter->exfunc->filter_window_update( filter );
		
	  Case WM_FILTER_FILE_CLOSE:
		delete g_Vsd;
		g_Vsd = NULL;
		
	  Case WM_FILTER_MAIN_KEY_DOWN:
		switch( wparam ){
			
		  case 'M':
			// マーク
			FRAME_STATUS	fsp;
			iFrame = filter->exfunc->get_frame( editp );
			
			filter->exfunc->get_frame_status( editp, iFrame, &fsp );
			fsp.edit_flag ^= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
			filter->exfunc->set_frame_status( editp, iFrame, &fsp );
			
			g_Vsd->m_bCalcLapTimeReq = TRUE;
			return TRUE;
		}
		
	  Case WM_COMMAND:
		if( ID_BUTT_SET_VSt <= wparam && wparam <= ID_BUTT_SET_GEd ){
			// フレーム数セット
			switch( wparam ){
				case ID_BUTT_SET_VSt:	filter->track[ TRACK_VSt ] = filter->exfunc->get_frame( editp );
				Case ID_BUTT_SET_VEd:	filter->track[ TRACK_VEd ] = filter->exfunc->get_frame( editp );
			#ifndef GPS_ONLY
				Case ID_BUTT_SET_LSt:	if( g_Vsd->m_VsdLog ) filter->track[ TRACK_LSt ] = g_Vsd->m_VsdLog->m_iLogNum;
				Case ID_BUTT_SET_LEd:	if( g_Vsd->m_VsdLog ) filter->track[ TRACK_LEd ] = g_Vsd->m_VsdLog->m_iLogNum;
			#endif
				Case ID_BUTT_SET_GSt:	if( g_Vsd->m_GPSLog ) filter->track[ TRACK_GSt ] = g_Vsd->m_GPSLog->m_iLogNum;
				Case ID_BUTT_SET_GEd:	if( g_Vsd->m_GPSLog ) filter->track[ TRACK_GEd ] = g_Vsd->m_GPSLog->m_iLogNum;
			}
			// 設定再描画
			filter->exfunc->filter_window_update( filter );
			
		}else switch( wparam ){
		  case ID_BUTT_LOAD_CFG:	// .avs ロード
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_CFG_EXT, NULL ) &&
				g_Vsd->ConfigLoad( szBuf )
			){
				// ログリード
			#ifndef GPS_ONLY
				if( *g_Vsd->m_szLogFile ){
					g_Vsd->ReadLog(
						GetFullPathWithCDir( szBuf2, g_Vsd->m_szLogFile, szBuf ),
						hwnd
					);
				}
			#endif
				if( *g_Vsd->m_szGPSLogFile ){
					g_Vsd->GPSLogLoad(
						GetFullPathWithCDir( szBuf2, g_Vsd->m_szGPSLogFile, szBuf ),
						hwnd
					);
				}
				
				// 設定再描画
				filter->exfunc->filter_window_update( filter );
				
				#ifndef GPS_ONLY
					// log pos 自動認識の更新
					func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
				#endif
			}
			
		  Case ID_BUTT_SAVE_CFG:
			if( filter->exfunc->dlg_get_save_name( szBuf, FILE_CFG_EXT, NULL ))
				return g_Vsd->ConfigSave( szBuf );
			
		#ifndef GPS_ONLY // {
		  Case ID_BUTT_LOAD_LOG:	// .log ロード
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_LOG_EXT, NULL ) &&
				g_Vsd->ReadLog( szBuf, hwnd )
			){
				// 設定再描画
				filter->exfunc->filter_window_update( filter );
				
				#ifndef GPS_ONLY
					// log pos 自動認識の更新
					func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
				#endif
			}
		#endif // }
			
		  Case ID_BUTT_LOAD_GPS:	// GPS ログロード
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_GPS_EXT, NULL ) &&
				g_Vsd->GPSLogLoad( szBuf, hwnd )
			){
				// 設定再描画
				filter->exfunc->filter_window_update( filter );
			}
			
		  Case ID_BUTT_SEL_FONT:	// フォント選択
			CHOOSEFONT	cf;
			cf.lStructSize = sizeof( CHOOSEFONT );
			cf.hwndOwner = hwnd;
			cf.lpLogFont = &( g_Vsd->m_logfont );
			cf.Flags = CF_SCREENFONTS | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
			if( ChooseFont( &cf )){
				// フォント強制更新
				g_Vsd->m_piParamS[ SHADOW_FONT_SIZE ] = -g_Vsd->m_logfont.lfHeight;
				delete g_Vsd->m_pFont;
				g_Vsd->m_pFont = NULL;
				
				SetWindowText( GetDlgItem( hwnd, ID_EDIT_SEL_FONT ), g_Vsd->m_logfont.lfFaceName );
			}
			
		  Default:
			return FALSE;
		}
		return TRUE;
	}
	
	return FALSE;
}

/*** スライダバー連動 *******************************************************/

BOOL func_update( FILTER *fp, int status ){
	static	BOOL	bReEnter = FALSE;
	
	if( bReEnter ) return TRUE;
	
	bReEnter = TRUE;
	
	if(
		g_Vsd && (
			status >= FILTER_UPDATE_STATUS_TRACK + TRACK_VSt &&
			status <= FILTER_UPDATE_STATUS_TRACK + TRACK_GSt ||
			status == FILTER_UPDATE_STATUS_TRACK + TRACK_SLineWidth
		)
	) g_Vsd->m_bCalcLapTimeReq = TRUE;
	
	#ifndef GPS_ONLY
		if(
			status == ( FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS ) &&
			fp->check[ CHECK_LOGPOS ]
		){
			fp->track[ TRACK_LSt ] = g_Vsd->m_iLogStart;
			fp->track[ TRACK_LEd ] = g_Vsd->m_iLogStop;
			
			// 設定再描画
			fp->exfunc->filter_window_update( fp );
		}
	#endif
	
	// マップ回転
	if( status == ( FILTER_UPDATE_STATUS_TRACK + TRACK_MapAngle )){
		if( g_Vsd->m_VsdLog )
			g_Vsd->m_VsdLog->RotateMap( fp->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
		if( g_Vsd->m_GPSLog )
			g_Vsd->m_GPSLog->RotateMap( fp->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	}
	
	bReEnter = FALSE;
	
	return TRUE;
}

/****************************************************************************/

BOOL WINAPI DllMain(
	HINSTANCE	hinstDLL,	// handle to DLL module
	DWORD		fdwReason,	// reason for calling function
	LPVOID		lpvReserved	// reserved
){
	if( fdwReason == DLL_PROCESS_ATTACH ){
		g_hInst = hinstDLL;
	}
	return TRUE;
}
