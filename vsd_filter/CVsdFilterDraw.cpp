/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFilter.cpp - CVsdFilter class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "../vsd/main.h"
#include "dds_lib/dds_lib.h"

#ifndef AVS_PLUGIN
	#include "filter.h"
#endif
#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CScript.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "error_code.h"

/*** macros *****************************************************************/

#define SPRINTF_BUF		128

#define INVALID_POS_I	0x7FFFFFFF
#define MAX_LINE_LEN	2000

#define LineTrace		m_piParamT[ TRACK_LineTrace ]
#define DispLap			m_piParamC[ CHECK_LAP ]
#define GSnakeLen		m_piParamT[ TRACK_G_Len ]
#define GScale			( m_piParamS[ SHADOW_G_SCALE ] * ( INVERT_G / 1000.0 ))
#define DispGraph		m_piParamC[ CHECK_GRAPH ]

#ifdef AVS_PLUGIN
	#define DispSyncInfo	0
#else
	#define DispSyncInfo	m_piParamC[ CHECK_SYNCINFO ]
#endif

#ifdef GPS_ONLY
	#define GPSPriority		FALSE
#else
	#define GPSPriority		m_piParamC[ CHECK_GPS_PRIO ]
#endif

// VSD log を優先，ただしチェックボックスでオーバーライドできる
#define SelectLogVsd ( Log = ( GPSPriority && m_GPSLog || !m_VsdLog ) ? m_GPSLog : m_VsdLog )

// GPS log を優先
#define SelectLogGPS ( Log = m_GPSLog ? m_GPSLog : m_VsdLog )

// Laptime 計算用
#define SelectLogForLapTime	( Log = m_iLapMode == LAPMODE_MAGNET || m_iLapMode == LAPMODE_HAND_MAGNET ? m_VsdLog : m_GPSLog )

#define DEFAULT_FONT	"ＭＳ ゴシック"

/*** DrawLine ***************************************************************/

#define ABS( x )			(( x ) < 0 ? -( x ) : ( x ))
#define SWAP( x, y, tmp )	( tmp = x, x = y, y = tmp )

void CVsdFilter::DrawLine( int x1, int y1, int x2, int y2, const PIXEL_YCA_ARG yc, UINT uFlag ){
	
	int i;
	
	if( y1 == y2 ){
		if( x1 > x2 ) SWAP( x1, x2, i );
		FillLine( x1, y1, x2, yc, uFlag );
		return;
	}
	
	int	iXdiff = ABS( x1 - x2 ) + 1;
	int iYdiff = ABS( y1 - y2 ) + 1;
	
	int iXsign = ( x2 > x1 ) ? 1 : -1;
	int iYsign = ( y2 > y1 ) ? 1 : -1;
	
	/* 傾きが1より小さい場合 */
	if( iXdiff > iYdiff ){
		int E = iYdiff - 2 * iXdiff;
		for( i = 0; i < iXdiff; i++ ){
			PutPixel( x1, y1, yc, uFlag );
			x1 += iXsign;
			E += 2 * iYdiff;
			if( E > 0 ){
				y1 += iYsign;
				E -= 2 * iXdiff;
			}
		}
	/* 傾きが1以上の場合 */
	}else{
		int E = iXdiff - 2 * iYdiff;
		for( i = 0; i < iYdiff; i++ ){
			PutPixel( x1, y1, yc, uFlag );
			y1 += iYsign;
			E += 2 * iXdiff;
			if( E > 0 ){
				x1 += iXsign;
				E -= 2 * iYdiff;
			}
		}
	}
}

void CVsdFilter::DrawLine( int x1, int y1, int x2, int y2, tRABY uColor, UINT uFlag ){
	PIXEL_YCA yc( uColor );
	DrawLine( x1, y1, x2, y2, yc, uFlag );
}

void CVsdFilter::DrawLine(
	int x1, int y1, int x2, int y2,
	int width,
	tRABY uColor,
	UINT uFlag
){
	
	PIXEL_YCA yc( uColor );
	
	for( int y = 0; y < width; ++y ) for( int x = 0; x < width; ++x ){
		DrawLine(
			x1 + x - width / 2, y1 + y - width / 2,
			x2 + x - width / 2, y2 + y - width / 2,
			yc, uFlag
		);
	}
}

/*** DrawRect ***************************************************************/

void CVsdFilter::DrawRect(
	int x1, int y1, int x2, int y2,
	tRABY uColor, UINT uFlag
){
	int	y;
	
	if( y1 > y2 ) SWAP( y1, y2, y );
	if( x1 > x2 ) SWAP( x1, x2, y );
	
	if( uFlag & IMG_FILL ){
		#ifdef _OPENMP
			#pragma omp parallel for
		#endif
		for( y = y1; y <= y2; ++y ){
			FillLine( x1, y, x2, uColor, 0 );
		}
	}else{
		DrawLine( x1, y1, x2, y1, uColor, 0 );
		DrawLine( x1, y2, x2, y2, uColor, 0 );
		DrawLine( x1, y1, x1, y2, uColor, 0 );
		DrawLine( x2, y1, x2, y2, uColor, 0 );
	}
}

/*** DrawCircle *************************************************************/

void CVsdFilter::DrawCircle(
	int x, int y, int r,
	tRABY uColor,
	UINT uFlag
){
	
	int	i = r;
	int j = 0;
	int f = -2 * r + 3;
	
	PIXEL_YCA yc( uColor );
	
	// 円を書く
	while( i >= j ){
		PutPixel( x + i, y + j, yc, uFlag ); PutPixel( x + i, y - j, yc, uFlag );
		PutPixel( x - i, y + j, yc, uFlag ); PutPixel( x - i, y - j, yc, uFlag );
		PutPixel( x + j, y + i, yc, uFlag ); PutPixel( x - j, y + i, yc, uFlag );
		PutPixel( x + j, y - i, yc, uFlag ); PutPixel( x - j, y - i, yc, uFlag );
		
		if( f >= 0 ){
			--i;
			f -= 4 * i;
		}
		++j;
		f += 4 * j + 2;
	}
	
	// Polygon 合成
	if( uFlag & IMG_FILL ) DrawPolygon( yc );
}

// http://fussy.web.fc2.com/algo/algo2-2.htm
// の，a = r / A, b = r / B と置いて両辺に ( A * B / r )^2 をかける
void CVsdFilter::DrawCircle( int x, int y, int a, int b, tRABY uColor, UINT uFlag ){
	
	PIXEL_YCA yc( uColor );
	
	if( a == b ){
		DrawCircle( x, y, a, uColor, uFlag );
		return;
	}
	
	int		i	= a;
	int		j	= 0;
	int		a2	= b * b;
	int		b2	= a * a;
	int		d	= a * b * b;
	int		f	= -2 * d + a2 + 2 * b2;
	int		h	= -4 * d + 2 * a2 + b2;
	
	while( i >= 0 ){
		PutPixel( x + i, y + j, yc, uFlag );
		PutPixel( x - i, y + j, yc, uFlag );
		PutPixel( x + i, y - j, yc, uFlag );
		PutPixel( x - i, y - j, yc, uFlag );
		
		if( f >= 0 ){
			--i;
			f -= 4 * a2 * i;
			h -= 4 * a2 * i - 2 * a2;
		}
		if( h < 0 ){
			++j;
			f += 4 * b2 * j + 2 * b2;
			h += 4 * b2 * j;
		}
	}
	
	// Polygon 合成
	if( uFlag & IMG_FILL ) DrawPolygon( yc );
}

void CVsdFilter::DrawArc(
	int x, int y,
	int a, int b,
	double dStart, double dEnd,
	tRABY uColor, UINT uFlag
){
	int		i	= a;
	int		j	= 0;
	int		a2	= b * b;
	int		b2	= a * a;
	int		d	= a * b * b;
	int		f	= -2 * d + a2 + 2 * b2;
	int		h	= -4 * d + 2 * a2 + b2;
	
	int		iStX = ( int )( 1024 * a * abs( cos( ToRAD * dStart )));
	int		iStY = ( int )( 1024 * b * abs( sin( ToRAD * dStart )));
	int		iEdX = ( int )( 1024 * a * abs( cos( ToRAD * dEnd )));
	int		iEdY = ( int )( 1024 * b * abs( sin( ToRAD * dEnd )));
	
	int		iStart	= ( int )dStart;
	int		iEnd	= ( int )dEnd;
	int		iStArea	= ( iStart / ( 360 / 4 )) << 4;
	int		iEdArea	= ( iEnd   / ( 360 / 4 )) << 4;
	
	
	PIXEL_YCA yc( uColor );
	
	int	iAreaCmpS, iAreaCmpE;
	
	while( i >= 0 ){
		// (i,j) が iStar / iEnd の角度よりも大きい / 小さい を計算しておく
		iAreaCmpS = iStX * j - iStY * i;
		iAreaCmpS = iAreaCmpS == 0 ? 0 :
					iAreaCmpS >= 0 ? 1 : -1;
		iAreaCmpS = iStArea + (( iStArea & 0x10 ) ? iAreaCmpS : -iAreaCmpS );
		
		iAreaCmpE = iEdX * j - iEdY * i;
		iAreaCmpE = iAreaCmpE == 0 ? 0 :
					iAreaCmpE >= 0 ? 1 : -1;
		iAreaCmpE = iEdArea + (( iEdArea & 0x10 ) ? iAreaCmpE : -iAreaCmpE );
		
		if( iStart < iEnd ){
			// st && ed
			if( iAreaCmpS <= 0x00 && 0x00 <= iAreaCmpE ) PutPixel( x + i, y + j, yc, uFlag );
			if( iAreaCmpS <= 0x10 && 0x10 <= iAreaCmpE ) PutPixel( x - i, y + j, yc, uFlag );
			if( iAreaCmpS <= 0x20 && 0x20 <= iAreaCmpE ) PutPixel( x - i, y - j, yc, uFlag );
			if( iAreaCmpS <= 0x30 && 0x30 <= iAreaCmpE ) PutPixel( x + i, y - j, yc, uFlag );
		}else{
			// st || ed
			if( iAreaCmpS <= 0x00 || 0x00 <= iAreaCmpE ) PutPixel( x + i, y + j, yc, uFlag );
			if( iAreaCmpS <= 0x10 || 0x10 <= iAreaCmpE ) PutPixel( x - i, y + j, yc, uFlag );
			if( iAreaCmpS <= 0x20 || 0x20 <= iAreaCmpE ) PutPixel( x - i, y - j, yc, uFlag );
			if( iAreaCmpS <= 0x30 || 0x30 <= iAreaCmpE ) PutPixel( x + i, y - j, yc, uFlag );
		}
		
		if( f >= 0 ){
			--i;
			f -= 4 * a2 * i;
			h -= 4 * a2 * i - 2 * a2;
		}
		if( h < 0 ){
			++j;
			f += 4 * b2 * j + 2 * b2;
			h += 4 * b2 * j;
		}
	}
	
	// Polygon 合成
	if( uFlag & IMG_FILL ) DrawPolygon( yc );
}

void CVsdFilter::DrawArc(
	int x, int y,
	int a, int b,
	int c, int d,
	double dStart, double dEnd,
	tRABY uColor, UINT uFlag
){
	int		iStX = ( int )( 1024 * a * abs( cos( ToRAD * dStart )));
	int		iStY = ( int )( 1024 * b * abs( sin( ToRAD * dStart )));
	int		iEdX = ( int )( 1024 * a * abs( cos( ToRAD * dEnd )));
	int		iEdY = ( int )( 1024 * b * abs( sin( ToRAD * dEnd )));
	
	int		iStart	= ( int )dStart;
	int		iEnd	= ( int )dEnd;
	int		iStArea	= ( iStart / ( 360 / 4 )) << 4;
	int		iEdArea	= ( iEnd   / ( 360 / 4 )) << 4;
	
	int	iAreaCmpS, iAreaCmpE;
	
	double		a2		= pow( a + 0.5, 2 );
	double		a2_b2	= a2 / pow( b + 0.5, 2 );
	double		c2		= pow( c + 0.5, 2 );
	double		c2_d2	= c2 / pow( d + 0.5, 2 );
	
	PIXEL_YCA yc( uColor );
	
	for( int j = 0; j <= b; ++j ){
		int iScanS = ( j <= d ) ? ( int )sqrt( c2 - c2_d2 * ( j * j )) : 0;
		int iScanE = ( int )sqrt( a2 - a2_b2 * ( j * j ));
		
		for( int i = iScanS; i <= iScanE; ++i ){
			
			// (i,j) が iStar / iEnd の角度よりも大きい / 小さい を計算しておく
			iAreaCmpS = iStX * j - iStY * i;
			iAreaCmpS = iAreaCmpS == 0 ? 0 :
						iAreaCmpS >= 0 ? 1 : -1;
			iAreaCmpS = iStArea + (( iStArea & 0x10 ) ? iAreaCmpS : -iAreaCmpS );
			
			iAreaCmpE = iEdX * j - iEdY * i;
			iAreaCmpE = iAreaCmpE == 0 ? 0 :
						iAreaCmpE >= 0 ? 1 : -1;
			iAreaCmpE = iEdArea + (( iEdArea & 0x10 ) ? iAreaCmpE : -iAreaCmpE );
			
			if( iStart < iEnd ){
				// st && ed
				if(           iAreaCmpS <= 0x00 && 0x00 <= iAreaCmpE ) PutPixel( x + i, y + j, yc, 0 );
				if( i &&      iAreaCmpS <= 0x10 && 0x10 <= iAreaCmpE ) PutPixel( x - i, y + j, yc, 0 );
				if( i && j && iAreaCmpS <= 0x20 && 0x20 <= iAreaCmpE ) PutPixel( x - i, y - j, yc, 0 );
				if(      j && iAreaCmpS <= 0x30 && 0x30 <= iAreaCmpE ) PutPixel( x + i, y - j, yc, 0 );
			}else{
				// st || ed
				if(           iAreaCmpS <= 0x00 || 0x00 <= iAreaCmpE ) PutPixel( x + i, y + j, yc, 0 );
				if( i &&      iAreaCmpS <= 0x10 || 0x10 <= iAreaCmpE ) PutPixel( x - i, y + j, yc, 0 );
				if( i && j && iAreaCmpS <= 0x20 || 0x20 <= iAreaCmpE ) PutPixel( x - i, y - j, yc, 0 );
				if(      j && iAreaCmpS <= 0x30 || 0x30 <= iAreaCmpE ) PutPixel( x + i, y - j, yc, 0 );
			}
		}
	}
}

/*** DrawFont ***************************************************************/

int CVsdFilter::DrawFont0( int x, int y, UCHAR c, CVsdFont &Font, tRABY uColor ){
	
	// 文字幅を得る
	CFontGlyph &FontGlyph = Font.FontGlyph( c );
	
	int iCellIncX = Font.IsFixed() ? Font.GetWidth() : FontGlyph.iCellIncX;
	int iOrgX = ( iCellIncX - FontGlyph.iW ) / 2;
	
	if( !Font.IsNoAntialias()){
		int iBmpW = ( FontGlyph.iW + 3 ) & ~3;
		
		#ifdef _OPENMP
			#pragma omp parallel for
		#endif
		for( int j = 0; j < FontGlyph.iH; ++j ) for( int i = 0; i < FontGlyph.iW; ++i ){
			int iDensity = FontGlyph.pBuf[ iBmpW * j + i ];	// 0〜64
			
			if( iDensity ){
				if( iDensity == 64 ){
					PutPixel( x + iOrgX + i, y + FontGlyph.iOrgY + j, uColor, 0 );
				}else{
					int ir = ( uColor >> 24 ) - 0x80;
					int ia = ( 0xFF << 6 ) - (( 0xFF - (( uColor >> 16 ) & 0xFF )) * iDensity );
					int ib = (( uColor >>  8 ) & 0xFF ) - 0x80;
					int iy = uColor & 0xFF;
					
					UINT uColorTmp = (
						(( ir * iDensity ) & ( 0xFF << 6 )) << ( 24 - 6 ) |
						(( ia            ) & ( 0xFF << 6 )) << ( 16 - 6 ) |
						(( ib * iDensity ) & ( 0xFF << 6 )) << (  8 - 6 ) |
						(( iy * iDensity ) & ( 0xFF << 6 )) >> (      6 )
					) ^ 0x80008000;
					
					PutPixel( x + iOrgX + i, y + FontGlyph.iOrgY + j, uColorTmp, 0 );
				}
			}
		}
	}else{
		int iBmpW = (( FontGlyph.iW + 31 ) & ~31 ) / 8;
		UINT uBitmap;
		
		#ifdef _OPENMP
			#pragma omp parallel for
		#endif
		for( int j = 0; j < FontGlyph.iH; ++j ) for( int i = 0; i < FontGlyph.iW; ++i ){
			if(( i & 0x7 ) == 0 ) uBitmap = FontGlyph.pBuf[ iBmpW * j + ( i >> 3 ) ];
			
			if( uBitmap & 0x80 ){
				PutPixel( x + iOrgX + i, y + FontGlyph.iOrgY + j, uColor, 0 );
			}
			uBitmap <<= 1;
		}
	}
	
	return iCellIncX;
}

int CVsdFilter::DrawFont( int x, int y, UCHAR c, CVsdFont &Font, tRABY uColor, tRABY uColorOutline ){
	
	// フォントが存在しない文字なら，space の文字幅を返す
	if( !CVsdFont::ExistFont( c ))
		return ( Font.IsFixed()) ? Font.GetWidth() : Font.GetW_Space();
	
	if( Font.IsOutline()){
		DrawFont0( x + 1, y + 0, c, Font, uColorOutline );
		DrawFont0( x - 1, y + 0, c, Font, uColorOutline );
		DrawFont0( x + 0, y + 1, c, Font, uColorOutline );
		DrawFont0( x + 0, y - 1, c, Font, uColorOutline );
	}
	return DrawFont0( x, y, c, Font, uColor );
}

/*** DrawText *************************************************************/

void CVsdFilter::DrawText( int x, int y, char *szMsg, CVsdFont &Font, tRABY uColor, tRABY uColorOutline ){
	
	if( x != POS_DEFAULT ) m_iPosX = x;
	if( y != POS_DEFAULT ) m_iPosY = y;
	
	x = m_iPosX;
	
	for( int i = 0; szMsg[ i ]; ++i ){
		x += DrawFont( x, m_iPosY, szMsg[ i ], Font, uColor, uColorOutline );
	}
	
	m_iPosY += Font.GetHeight();
}

void CVsdFilter::DrawTextAlign( int x, int y, UINT uAlign, char *szMsg, CVsdFont &Font, tRABY uColor, tRABY uColorOutline ){
	
	if( uAlign & ALIGN_HCENTER ){
		x -= Font.GetTextWidth( szMsg ) / 2;
	}else if( uAlign & ALIGN_RIGHT ){
		x -= Font.GetTextWidth( szMsg );
	}
	
	if( uAlign & ALIGN_VCENTER ){
		y -= Font.GetHeight() / 2;
	}else if( uAlign & ALIGN_BOTTOM ){
		y -= Font.GetHeight();
	}
	
	for( int i = 0; szMsg[ i ]; ++i ){
		x += DrawFont( x, y, szMsg[ i ], Font, uColor, uColorOutline );
	}
	
	m_iPosY += Font.GetHeight();
}

/*** put pixel 系 ***********************************************************/

inline void CVsdFilter::PutPixel( int x, int y, tRABY uColor, UINT uFlag ){
	PIXEL_YCA	yc( uColor );
	PutPixel( x, y, yc, uFlag );
}

inline void CVsdFilter::PutPixel( int x, int y, const PIXEL_YCA_ARG yc, UINT uFlag ){
	
	if( !( 0 <= y && y < GetHeight())) return;
	
	if( uFlag & IMG_FILL ){
		// ポリゴン描画
		if( x > m_Polygon[ y ].iRight ){
			m_Polygon[ y ].iRight = ( x >= GetWidth()) ? GetWidth() : x;
		}
		if( x < m_Polygon[ y ].iLeft  ){
			m_Polygon[ y ].iLeft  = ( x < 0 ) ? 0 : x;
		}
	}else if( 0 <= x && x < GetWidth() && yc.alfa < 255 ){
		PutPixel( x, y, yc );
	}
}

inline void CVsdFilter::FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc, UINT uFlag ){
	
	if( uFlag & IMG_FILL ){
		// ポリゴン描画
		if( x1 > x2 ){
			if( x1 > m_Polygon[ y1 ].iRight ) m_Polygon[ y1 ].iRight = x1;
			if( x2 < m_Polygon[ y1 ].iLeft  ) m_Polygon[ y1 ].iLeft  = x2;
		}else{
			if( x2 > m_Polygon[ y1 ].iRight ) m_Polygon[ y1 ].iRight = x2;
			if( x1 < m_Polygon[ y1 ].iLeft  ) m_Polygon[ y1 ].iLeft  = x1;
		}
	}else if( 0 <= y1 && y1 < GetHeight() && yc.alfa < 255 ){
		if( x1 < 0 )         x1 = 0;
		if( x2 > GetWidth()) x2 = GetWidth();
		
		FillLine( x1, y1, x2, yc );
	}
}

/*** ポリゴン描画 ***********************************************************/

inline void CVsdFilter::InitPolygon( void ){
	#ifdef _OPENMP
		#pragma omp parallel for
	#endif
	for( int y = 0; y < GetHeight(); ++y ){
		m_Polygon[ y ].iRight	= 0;		// right
		m_Polygon[ y ].iLeft	= 0x7FFF;	// left
	}
}

inline void CVsdFilter::DrawPolygon( const PIXEL_YCA_ARG yc ){
	#ifdef _OPENMP
		#pragma omp parallel for
	#endif
	for( int y = 0; y < GetHeight(); ++y ) if( m_Polygon[ y ].iLeft <= m_Polygon[ y ].iRight ){
		FillLine( m_Polygon[ y ].iLeft, y, m_Polygon[ y ].iRight, yc );
	}
	
	InitPolygon();
}

void CVsdFilter::DrawPolygon( tRABY uColor ){
	PIXEL_YCA	yc( uColor );
	DrawPolygon( yc );
}

/*** カラーを混ぜる *********************************************************/

inline UINT CVsdFilter::BlendColor(
	tRABY uColor0,
	tRABY uColor1,
	double	dAlfa
){
	if     ( dAlfa < 0.0 ) dAlfa = 0.0;
	else if( dAlfa > 1.0 ) dAlfa = 1.0;
	
	return
		(( UINT )(( uColor1 & 0xFF000000 ) * dAlfa + ( uColor0 & 0xFF000000 ) * ( 1 - dAlfa )) & 0xFF000000 ) +
		(( UINT )(( uColor1 & 0x0000FF00 ) * dAlfa + ( uColor0 & 0x0000FF00 ) * ( 1 - dAlfa )) & 0x0000FF00 ) +
		(( UINT )(( uColor1 & 0x000000FF ) * dAlfa + ( uColor0 & 0x000000FF ) * ( 1 - dAlfa )) & 0x000000FF );
}

/****************************************************************************/
/*** メーター描画 ***********************************************************/
/****************************************************************************/

/*** パラメータ調整用スピードグラフ *****************************************/

#define GRAPH_SCALE	2

static double GetSpeedLog( CVsdLog& Log, int iIndex ){
	return Log.Speed( iIndex );
}

static double GetTachoLog( CVsdLog& Log, int iIndex ){
	return Log.Tacho( iIndex );
}

void CVsdFilter::DrawGraph(
	int x1, int y1, int x2, int y2,
	char *szFormat,
	CVsdFont &Font,
	tRABY uColor,
	CVsdLog& Log,
	double ( *GetDataFunc )( CVsdLog&, int ),
	double dMaxVal
){
	int	iWidth  = x2 - x1 + 1;
	int iHeight = y2 - y1 + 1;
	
	int		iPrevY = INVALID_POS_I;
	int		iCursorPos;
	double	dCursorVal;
	double	dVal;
	
	char	szBuf[ SPRINTF_BUF ];
	
	for( int x = 0; x < iWidth; ++x ){
		int iLogNum = Log.m_iLogNum + ( x - iWidth / 2 ) * GRAPH_SCALE;
		if( iLogNum < 0 || Log.m_iCnt < iLogNum ){
			dVal = 0;
		}else{
			dVal = GetDataFunc( Log, iLogNum );
		}
		
		int iPosY = y2 - ( int )( dVal * iHeight / dMaxVal );
		if( iPrevY != INVALID_POS_I )
			DrawLine( x1 + x - 1, iPrevY, x1 + x, iPosY, 1, uColor, 0 );
		
		iPrevY = iPosY;
		
		if( x == iWidth / 2 ){
			iCursorPos = iPosY;
			dCursorVal = dVal;
		}
	}
	
	int x = x1 + iWidth / 2;
	DrawLine(
		x, iCursorPos,
		x + 10, iCursorPos - 10,
		1, uColor, 0
	);
	
	sprintf( szBuf, szFormat, dCursorVal );
	DrawText(
		x + 10,
		iCursorPos - 10 - Font.GetHeight(),
		szBuf, Font, uColor
	);
}

// スピード・タコグラフ
void CVsdFilter::DrawGraph(
	int x1, int y1, int x2, int y2,
	CVsdFont &Font
){
	if(
		#ifdef GPS_ONLY
			DispGraph
		#else
			DispGraph || DispSyncInfo
		#endif
	){
		if( m_VsdLog ){
			DrawGraph(
				x1, y1, x2, y2,
				"%.0f km/h", Font, color_orange,
				*m_VsdLog,
				GetSpeedLog, m_VsdLog->m_iMaxSpeed
			);
			if( !DispSyncInfo ) DrawGraph(
				x1, y1, x2, y2,
				"%.0f rpm", Font, color_cyan,
				*m_VsdLog,
				GetTachoLog, 7000
			);
		}
		
		if(( !m_VsdLog || DispSyncInfo ) && m_GPSLog ){
			DrawGraph(
				x1, y1, x2, y2,
				"%.0f km/h", Font, color_cyan,
				*m_GPSLog,
				GetSpeedLog, m_VsdLog->m_iMaxSpeed
			);
		}
	}
}

/*** G スネーク描画 *********************************************************/

void CVsdFilter::DrawGSnake(
	int iCx, int iCy, int iR, int iIndicatorR, int iWidth,
	tRABY uColorBall, tRABY uColorLine
){
	int	iGx, iGy;
	int	i;
	
	CVsdLog *Log;
	SelectLogVsd;
	
	if( Log && Log->m_iLogNum < Log->m_iCnt ){
		if( GSnakeLen > 0 ){
			
			int iGxPrev = INVALID_POS_I, iGyPrev;
			
			for( i = -( int )( GSnakeLen * LOG_FREQ / 10.0 ) ; i <= 1 ; ++i ){
				
				if( Log->m_iLogNum + i >= 0 ){
					// i == 1 時は最後の中途半端な LogNum
					iGx = ( int )((( i != 1 ) ? Log->m_Log[ Log->m_iLogNum + i ].Gx() : Log->Gx()) * iR );
					iGy = ( int )((( i != 1 ) ? Log->m_Log[ Log->m_iLogNum + i ].Gy() : Log->Gy()) * iR );
					
					iGx = ( int )( iGx );
					
					if( iGxPrev != INVALID_POS_I ) DrawLine(
						iCx + iGx, iCy - iGy, iCx + iGxPrev, iCy - iGyPrev,
						iWidth, uColorLine, 0
					);
					
					iGxPrev = iGx;
					iGyPrev = iGy;
				}
			}
		}else{
			iGx = ( int )( Log->Gx() * iR );
			iGy = ( int )( Log->Gy() * iR );
		}
	}else{
		iGx = iGy = 0;
	}
	
	// G インジケータ
	DrawCircle(
		iCx + iGx, iCy - iGy, iIndicatorR,
		uColorBall, CVsdFilter::IMG_FILL
	);
}

/*** 走行軌跡表示 ***********************************************************/

void CVsdFilter::DrawMap(
	int x1, int y1, int x2, int y2,
	UINT uAlign,
	int iLineWidth,
	int iIndicatorR,
	tRABY uColorIndicator,
	tRABY uColorG0,
	tRABY uColorGPlus,
	tRABY uColorGMinus
){
	double dGx, dGy;
	int	iGx, iGy;
	int i;
	
	CVsdLog *Log;
	SelectLogGPS;
	
	if( !LineTrace || !Log || !Log->IsDataExist()) return;
	
	int iWidth  = x2 - x1 + 1;
	int iHeight = y2 - y1 + 1;
	double dScaleX = iWidth  / Log->m_dMapSizeX;
	double dScaleY = iHeight / Log->m_dMapSizeY;
	double dScale;
	
	if( dScaleX < dScaleY ){
		// 幅律速なので y1 を再計算
		dScale = dScaleX;
		if( uAlign & ALIGN_HCENTER ){
			y1 = y1 + ( iHeight - ( int )( Log->m_dMapSizeY * dScale )) / 2;
		}else if( uAlign & ALIGN_BOTTOM ){
			y1 = y1 + ( iHeight - ( int )( Log->m_dMapSizeY * dScale ));
		}
	}else{
		// 高さ律速なので x1 を再計算
		dScale = dScaleY;
		if( uAlign & ALIGN_HCENTER ){
			x1 = x1 + ( iWidth - ( int )( Log->m_dMapSizeX * dScale )) / 2;
		}else if( uAlign & ALIGN_BOTTOM ){
			x1 = x1 + ( iWidth - ( int )( Log->m_dMapSizeX * dScale ));
		}
	}
	
	int iGxPrev = INVALID_POS_I, iGyPrev;
	
	int iLineSt = ( int )LapNum2LogNum( Log, m_iLapIdx );
	if( Log->m_iLogNum - iLineSt > ( int )( LineTrace * LOG_FREQ ))
		iLineSt = Log->m_iLogNum - ( int )( LineTrace * LOG_FREQ );
	
	int iLineEd = m_iLapIdx != m_iLapNum - 1
		? ( int )LapNum2LogNum( Log, m_iLapIdx + 1 ) : Log->m_iCnt - 1;
	
	if( iLineEd - Log->m_iLogNum > ( int )( LineTrace * LOG_FREQ ))
		iLineEd = Log->m_iLogNum + ( int )( LineTrace * LOG_FREQ );
	
	for( i = iLineSt; i <= iLineEd ; ++i ){
		if( !_isnan( Log->X( i ))){
			#define GetMapPos( p, a ) ((( p ) - Log->m_dMapOffs ## a ) * dScale )
			iGx = x1 + ( int )GetMapPos( Log->X( i ), X );
			iGy = y1 + ( int )GetMapPos( Log->Y( i ), Y );
			
			if( iGxPrev != INVALID_POS_I ){
				if(
					( iGx - iGxPrev ) * ( iGx - iGxPrev ) +
					( iGy - iGyPrev ) * ( iGy - iGyPrev ) >= ( 25 )
				){
					// Line の色用に G を求める
					
					double dG = Log->Gy( i );
					
					tRABY uColorLine;
					
					if( dG >= 0.0 ){
						uColorLine = BlendColor( uColorG0, uColorGPlus,  dG / Log->m_dMaxG );
					}else{
						uColorLine = BlendColor( uColorG0, uColorGMinus, dG / Log->m_dMinG );
					}
					
					// Line を引く
					DrawLine(
						iGx,     iGy,
						iGxPrev, iGyPrev,
						iLineWidth, uColorLine, 0
					);
					iGxPrev = iGx;
					iGyPrev = iGy;
				}
			}else{
				iGxPrev = iGx;
				iGyPrev = iGy;
			}
		}else{
			iGxPrev = INVALID_POS_I;
		}
	}
	
	// MAP インジケータ (自車)
	dGx = x1 + GetMapPos( Log->X(), X );
	dGy = y1 + GetMapPos( Log->Y(), Y );
	
	if( !_isnan( dGx )) DrawCircle(
		( int )( dGx ), ( int )dGy, iIndicatorR,
		uColorIndicator, CVsdFilter::IMG_FILL
	);
	
	// スタートライン表示
	if( DispSyncInfo && m_iLapMode == LAPMODE_GPS ){
		double dAngle = m_piParamT[ TRACK_MapAngle ] * ( -ToRAD / 10 );
		
		int xs1 = x1 + ( int )((  cos( dAngle ) * m_dStartLineX1 + sin( dAngle ) * m_dStartLineY1 - Log->m_dMapOffsX ) * dScale );
		int ys1 = y1 + ( int )(( -sin( dAngle ) * m_dStartLineX1 + cos( dAngle ) * m_dStartLineY1 - Log->m_dMapOffsY ) * dScale );
		int xs2 = x1 + ( int )((  cos( dAngle ) * m_dStartLineX2 + sin( dAngle ) * m_dStartLineY2 - Log->m_dMapOffsX ) * dScale );
		int ys2 = y1 + ( int )(( -sin( dAngle ) * m_dStartLineX2 + cos( dAngle ) * m_dStartLineY2 - Log->m_dMapOffsY ) * dScale );
		
		DrawLine( xs1, ys1, xs2, ys2, color_blue, 0 );
	}
}

/*** 針描画 *****************************************************************/

void CVsdFilter::DrawNeedle(
	int x, int y, int r1, int r2,
	int iStart, int iEnd, double dVal,
	tRABY uColor,
	int iWidth
){
	iStart %= 360;
	iEnd   %= 360;
	double dAngle = ( iStart + (( iStart > iEnd ? 360 : 0 ) + ( iEnd - iStart )) * dVal ) * ToRAD;
	
	DrawLine(
		( int )( cos( dAngle ) * r1 ) + x,
		( int )( sin( dAngle ) * r1 ) + y,
		( int )( cos( dAngle ) * r2 ) + x,
		( int )( sin( dAngle ) * r2 ) + y,
		iWidth, uColor, 0
	);
}

/*** ラップタイム表示 *******************************************************/

void CVsdFilter::DrawLapTime(
	int x, int y, CVsdFont &Font,
	tRABY uColor, tRABY uColorOutline, tRABY uColorBest, tRABY uColorPlus
){
	
	BOOL	bInLap = FALSE;	// ラップタイム計測中
	int	i;
	char	szBuf[ SPRINTF_BUF ];
	
	if( !DispLap || !m_iLapNum ) return;
	
	CVsdLog *Log;
	SelectLogForLapTime;
	
	// 時間表示
	if( m_iLapIdx >= 0 && m_Lap[ m_iLapIdx + 1 ].iTime != 0 ){
		int iTime;
		if( m_iLapMode != LAPMODE_HAND_VIDEO ){
			// 自動計測時は，タイム / ログ数 から計算
			iTime = ( int )(( Log->m_dLogNum - m_Lap[ m_iLapIdx ].fLogNum ) * 1000 / Log->m_dFreq );
		}else{
			// 手動計測モードのときは，フレーム数から計算
			iTime = ( int )(( GetFrameCnt() - m_Lap[ m_iLapIdx ].fLogNum ) * 1000.0 / GetFPS());
		}
		
		sprintf( szBuf, "Time%2d'%02d.%03d", iTime / 60000, iTime / 1000 % 60, iTime % 1000 );
		DrawText( x, y, szBuf, Font, uColor, uColorOutline );
		bInLap = TRUE;
	}else{
		// まだ開始していない
		DrawText( x, y, "Time -'--.---", Font, uColor, uColorOutline );
	}
	
	/*** ベストとの車間距離表示 - ***/
	if( m_VsdLog || m_GPSLog ){
		if( bInLap ){
			
			SelectLogGPS;
			
			// ベストラップ開始の LogNum
			double dBestLapLogNumStart = LapNum2LogNum( Log, m_iBestLap );
			
			// この周の走行距離を求める
			double dMileage = Log->Mileage() - Log->Mileage( LapNum2LogNum( Log, m_iLapIdx ));
			
			// この周の 1周の走行距離から，現在の走行距離を補正する
			dMileage =
				dMileage
				* ( Log->Mileage( LapNum2LogNum( Log, m_iBestLap + 1 )) - Log->Mileage( dBestLapLogNumStart ))
				/ ( Log->Mileage( LapNum2LogNum( Log, m_iLapIdx  + 1 )) - Log->Mileage( LapNum2LogNum( Log, m_iLapIdx )));
			
			// 最速 Lap の，同一走行距離におけるタイム (=ログ番号,整数) を求める
			// m_iBestLogNumRunning <= 最終的に求める結果 < m_iBestLogNumRunning + 1  となる
			// m_iBestLogNumRunning がおかしかったら，リセット
			if(
				m_iBestLogNumRunning < dBestLapLogNumStart ||
				m_iBestLogNumRunning >= Log->m_iCnt ||
				( Log->Mileage( m_iBestLogNumRunning ) - Log->Mileage( dBestLapLogNumStart )) > dMileage
			) m_iBestLogNumRunning = ( int )dBestLapLogNumStart;
			
			for(
				;
				( Log->Mileage( m_iBestLogNumRunning + 1 ) - Log->Mileage( dBestLapLogNumStart )) <= dMileage &&
				m_iBestLogNumRunning < Log->m_iCnt;
				++m_iBestLogNumRunning
			);
			
			// 最速 Lap の，1/15秒以下の値を求める = A / B
			double dBestLapLogNumRunning =
				( double )m_iBestLogNumRunning +
				// A: 最速ラップは，後これだけ走らないと dMileage と同じではない
				( dMileage - ( Log->Mileage( m_iBestLogNumRunning ) - Log->Mileage( dBestLapLogNumStart ))) /
				// B: 最速ラップは，1/15秒の間にこの距離を走った
				( Log->Mileage( m_iBestLogNumRunning + 1 ) - Log->Mileage( m_iBestLogNumRunning ));
			
			int iDiffTime = ( int )(
				(
					( Log->m_dLogNum - LapNum2LogNum( Log, m_iLapIdx )) -
					( dBestLapLogNumRunning - dBestLapLogNumStart )
				) * 1000.0 / Log->m_dFreq
			);
			
			BOOL bSign = iDiffTime <= 0;
			if( iDiffTime < 0 ) iDiffTime = -iDiffTime;
			
			sprintf(
				szBuf, "    %c%d'%02d.%03d",
				bSign ? '-' : '+',
				iDiffTime / 60000,
				iDiffTime / 1000 % 60,
				iDiffTime % 1000
			);
			DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, Font, bSign ? uColorBest : uColorPlus, uColorOutline );
		}else{
			m_iPosY += Font.GetHeight();
		}
	}
	
	m_iPosY += Font.GetHeight() / 4;
	
	// Best 表示
	sprintf(
		szBuf, "Best%2d'%02d.%03d",
		m_iBestTime / 60000,
		m_iBestTime / 1000 % 60,
		m_iBestTime % 1000
	);
	DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, Font, uColor, uColorOutline );
	
	// Lapタイム表示
	// 3つタイム表示する分の，最後の LapIdx を求める．
	// 通常は m_iLapIdx + 1 だが，m_Lap[ iLapIdxEnd ].iTime == 0 の時は
	// 周回モードでは最後のラップを走り終えた
	// ジムカモードでは 1周走り終えたことを示しているので
	// LapIdx を -1 する
	int iLapIdxEnd = m_iLapIdx + 1;
	if( m_Lap[ iLapIdxEnd ].iTime == 0 ) --iLapIdxEnd;
	
	// iLapIdxEnd から有効なラップタイムが 2個見つかるまで遡る
	int iLapIdxStart = iLapIdxEnd - 1;
	for( i = 0; iLapIdxStart > 0; --iLapIdxStart ){
		if( m_Lap[ iLapIdxStart ].iTime ){
			if( ++i >= 2 ) break;
		}
	}
	
	if( iLapIdxStart >= 0 ){
		for( ; iLapIdxStart <= iLapIdxEnd; ++iLapIdxStart ){
			if( m_Lap[ iLapIdxStart ].iTime != 0 ){
				sprintf(
					szBuf, "%3d%c%2d'%02d.%03d",
					m_Lap[ iLapIdxStart ].uLap,
					( iLapIdxStart == m_iLapIdx + 1 && bInLap ) ? '*' : ' ',
					m_Lap[ iLapIdxStart ].iTime / 60000,
					m_Lap[ iLapIdxStart ].iTime / 1000 % 60,
					m_Lap[ iLapIdxStart ].iTime % 1000
				);
				DrawText(
					POS_DEFAULT, POS_DEFAULT, szBuf, Font,
					m_iBestTime == m_Lap[ iLapIdxStart ].iTime ? uColorBest : uColor,
					uColorOutline
				);
				++i;
			}
		}
	}
}

/*** メーターパネル目盛り ***************************************************/

void CVsdFilter::DrawMeterScale(
	int iCx, int iCy, int iR,
	int iLineLen1, int iLineWidth1, tRABY uColorLine1,
	int iLineLen2, int iLineWidth2, tRABY uColorLine2,
	int iLine2Cnt,
	int iMinDeg, int iMaxDeg,
	int iRNum,
	int iMaxVal, int iMaxNumCnt, tRABY uColorNum,
	CVsdFont &Font
){
	int	i;
	char	szBuf[ SPRINTF_BUF ];
	
	const int iDegRange	= ( iMaxDeg + 360 - iMinDeg ) % 360;
	
	/*** メーターパネル ***/
	
	// iStep は切り上げ
	int	iStep = ( iMaxVal + iMaxNumCnt - 1 ) / iMaxNumCnt;
	
	if( iStep == 0 ){
		iStep = 1;
	}else if( iMaxVal >= 50 ){
		// 50以上では，10単位に切り上げ
		iStep = ( iStep + 9 ) / 10 * 10;
	}
	
	for( i = 0; i <= iMaxVal * iLine2Cnt / iStep; ++i ){
		double dAngle = ( iDegRange * i * iStep / ( double )iLine2Cnt / iMaxVal + iMinDeg ) * ToRAD;
		
		// メーターパネル目盛り
		if( i % iLine2Cnt == 0 ){
			DrawLine(
				( int )( cos( dAngle ) * iR ) + iCx,
				( int )( sin( dAngle ) * iR ) + iCy,
				( int )( cos( dAngle ) * ( iR - iLineLen1 )) + iCx,
				( int )( sin( dAngle ) * ( iR - iLineLen1 )) + iCy,
				iLineWidth1,
				uColorLine1, 0
			);
			
			// メーターパネル目盛り数値
			sprintf( szBuf, "%d", iStep * i / iLine2Cnt );
			DrawTextAlign(
				( int )( cos( dAngle ) * iRNum ) + iCx,
				( int )( sin( dAngle ) * iRNum ) + iCy,
				ALIGN_HCENTER | ALIGN_VCENTER,
				szBuf, Font, uColorNum
			);
		}else{
			// 小目盛り
			DrawLine(
				( int )( cos( dAngle ) * iR ) + iCx,
				( int )( sin( dAngle ) * iR ) + iCy,
				( int )( cos( dAngle ) * ( iR - iLineLen2 )) + iCx,
				( int )( sin( dAngle ) * ( iR - iLineLen2 )) + iCy,
				iLineWidth2,
				uColorLine2, 0
			);
		}
	}
}

/*** エラーメッセージ *******************************************************/

void CVsdFilter::DispErrorMessage( char *szMsg ){
	DrawRect( 0, 0, GetWidth() - 1, GetHeight() - 1, color_black_a, IMG_FILL );
	
	int x = 0; int y = 0;
	
	for( ; *szMsg; ++szMsg ){
		if( *szMsg == '\n' ){
			// 改行
			x = 0;
			y += m_pFont->GetHeight();
		}else{
			x += DrawFont( x, y, *szMsg, *m_pFont, color_red );
		}
	}
}

/*** メーター等描画 *********************************************************/

BOOL CVsdFilter::DrawVSD( void ){
	
	char	szBuf[ SPRINTF_BUF ];
	
	// 解像度変更
	if( m_iWidth != GetWidth() || m_iHeight != GetHeight()){
		m_iWidth  = GetWidth();
		m_iHeight = GetHeight();
		
		// ポリゴン用バッファリサイズ
		if( m_Polygon ) delete [] m_Polygon;
		m_Polygon = new PolygonData_t[ m_iHeight ];
		InitPolygon();
		
		// JavaScript 再起動用に削除
		if( m_Script ){
			delete m_Script;
			m_Script = NULL;
		}
		// フォントサイズ初期化
		if( m_pFont ) delete m_pFont;
		m_pFont = new CVsdFont( DEFAULT_FONT, 18, CVsdFont::ATTR_OUTLINE | CVsdFont::ATTR_NOANTIALIAS );
	}
	
	CVsdLog *Log;
	
	// ログ位置の計算
	if( m_VsdLog ){
		m_VsdLog->m_dLogNum = ConvParam( GetFrameCnt(), Video, Log );
		m_VsdLog->m_iLogNum = ( int )m_VsdLog->m_dLogNum;
	}
	if( m_GPSLog ){
		m_GPSLog->m_dLogNum = ConvParam( GetFrameCnt(), Video, GPS );
		m_GPSLog->m_iLogNum = ( int )m_GPSLog->m_dLogNum;
	}
	
	// ラップタイムの再計算
	if( m_iLapMode != LAPMODE_MAGNET && DispLap && m_bCalcLapTimeReq ){
		m_bCalcLapTimeReq	= FALSE;
		m_iLapMode			= LAPMODE_HAND_VIDEO;
		
		if( m_GPSLog && m_piParamT[ TRACK_SLineWidth ] > 0 ){
			CalcLapTimeAuto();
		}
		if( m_iLapMode != LAPMODE_GPS ){
			CalcLapTime();
		}
	}
	
	SelectLogForLapTime;
	
	// ラップインデックスを求める
	if( m_iLapNum ){
		// VSD/GPS 両方のログがなければ，手動計測での m_Lap[].fLogNum はフレーム# なので
		// m_Lap[].fLogNum と精度をあわせるため，m_dLogNum はいったん float に落とす
		float fLogNum = m_iLapMode != LAPMODE_HAND_VIDEO ? ( float )Log->m_dLogNum : GetFrameCnt();
		
		// カレントポインタがおかしいときは，-1 にリセット
		if(
			m_iLapIdx >= m_iLapNum ||
			m_iLapIdx >= 0 && m_Lap[ m_iLapIdx ].fLogNum > fLogNum
		) m_iLapIdx = -1;
		
		for( ; m_Lap[ m_iLapIdx + 1 ].fLogNum <= fLogNum; ++m_iLapIdx );
	}else{
		m_iLapIdx = -1;
	}
	
	// JavaScript 用ログデータ計算
	SelectLogVsd;
	if( Log && Log->m_iLogNum < Log->m_iCnt ){
		m_dSpeed	= Log->Speed( Log->m_dLogNum );
		m_dTacho	= Log->Tacho( Log->m_dLogNum );
		m_dGx		= Log->Gx( Log->m_dLogNum );
		m_dGy		= Log->Gy( Log->m_dLogNum );
		m_iMaxSpeed	= Log->m_iMaxSpeed;
	}else{
		m_dSpeed	=
		m_dTacho	=
		m_dGx		=
		m_dGy		= 0;
		m_iMaxSpeed	= 180;
	}
	
	// スクリプト実行
	DebugMsgD( ":DrawVSD():Running script... %X\n", GetCurrentThreadId());
	if( !m_Script && *m_szSkinFile ){
		m_Script = new CScript( this );
		if( m_Script->Initialize( m_szSkinFile ) != ERR_OK ){
			m_Script->m_bError = TRUE;
		}
	}
	
	if( m_Script ){
		if( !m_Script->m_bError ){
			if( m_Script->Run( "Draw" ) != ERR_OK ){
				m_Script->m_bError = TRUE;
			}
		}
		if( m_Script->m_bError ){
			DispErrorMessage( m_Script->m_szErrorMsg ? m_Script->m_szErrorMsg : "Unknown error" );
		}
	}else{
		DrawText( 0, 0, "Skin not loaded.", *m_pFont, color_white );
	}
	
	// フレーム表示
	
	#define Float2Time( n )	( int )( n ) / 60, fmod( n, 60 )
	
	if( DispSyncInfo ){
		
		m_iPosX = 0;
		m_iPosY = GetHeight() / 3;
		
		if( m_GPSLog ){
			int i = ( int )(( m_GPSLog->m_dLogStartTime + m_GPSLog->m_dLogNum / LOG_FREQ ) * 100 ) % ( 24 * 3600 * 100 );
			sprintf(
				szBuf, "GPS time: %02d:%02d:%02d.%02d",
				i / 360000,
				i / 6000 % 60,
				i /  100 % 60,
				i        % 100
			);
			DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, *m_pFont, color_white );
		}
		
		#ifndef GPS_ONLY
			DrawText( POS_DEFAULT, POS_DEFAULT, "        start       end     range cur.pos", *m_pFont, color_white );
			
			sprintf(
				szBuf, "Vid%4d:%05.2f%4d:%05.2f%4d:%05.2f%7d",
				Float2Time( VideoSt / GetFPS()),
				Float2Time( VideoEd / GetFPS()),
				Float2Time(( VideoEd - VideoSt ) / GetFPS()),
				GetFrameCnt()
			);
			DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, *m_pFont, color_white );
			
			if( m_VsdLog ){
				sprintf(
					szBuf, "Log%4d:%05.2f%4d:%05.2f%4d:%05.2f%7d",
					Float2Time( LogSt / m_VsdLog->m_dFreq ),
					Float2Time( LogEd / m_VsdLog->m_dFreq ),
					Float2Time(( LogEd - LogSt ) / m_VsdLog->m_dFreq ),
					m_VsdLog->m_iLogNum
				);
				DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, *m_pFont, color_white );
			}
			
			if( m_GPSLog ){
				sprintf(
					szBuf, "GPS%4d:%05.2f%4d:%05.2f%4d:%05.2f%7d",
					Float2Time( GPSSt / m_GPSLog->m_dFreq ),
					Float2Time( GPSEd / m_GPSLog->m_dFreq ),
					Float2Time(( GPSEd - GPSSt ) / m_GPSLog->m_dFreq ),
					m_GPSLog->m_iLogNum
				);
				DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, *m_pFont, color_white );
			}
		#endif	// !GPS_ONLY
	}
	
	return TRUE;
}
