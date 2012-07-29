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

/*** macros *****************************************************************/

#define MAX_POLY_HEIGHT	2000		// polygon �p���C����
#define SLineWidth		( m_piParamT[ TRACK_SLineWidth ] / 10.0 )

/*** static member **********************************************************/

/*** tarckbar / checkbox conf_name �� ***/

const char *CVsdFilter::m_szTrackbarName[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name ) conf_name,
	#include "def_trackbar.h"
};

const char *CVsdFilter::m_szCheckboxName[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name ) conf_name,
	#include "def_checkbox.h"
};

const char *CVsdFilter::m_szShadowParamName[] = {
	#define DEF_SHADOW( id, init, conf_name ) conf_name,
	#include "def_shadow.h"
};

/*** �R���X�g���N�^ *********************************************************/

CVsdFilter::CVsdFilter (){
	
	m_VsdLog 			= NULL;
	m_GPSLog 			= NULL;
	
	m_Lap	 			= new LAP_t[ MAX_LAP ];
	m_iLapNum			= 0;
	m_Lap[ 0 ].fLogNum	= FLT_MAX;	// �Ԍ�
	m_Lap[ 0 ].iTime	= 0;		// �Ԍ�
	
	m_iBestTime			= BESTLAP_NONE;
	m_iBestLap			= 0;
	m_iLapMode			= LAPMODE_HAND_VIDEO;
	
	m_iLapIdx			= -1;
	m_iBestLogNumRunning= 0;
	
	m_bCalcLapTimeReq	= FALSE;
	
	m_szLogFile			= new char[ MAX_PATH + 1 ];
	m_szGPSLogFile		= new char[ BUF_SIZE ];
	
	// DrawPolygon �p�o�b�t�@
	m_Polygon			= new PolygonData_t[ MAX_POLY_HEIGHT ];
	
	m_pFont				= NULL;
	
	m_logfont.lfHeight			= 0;							// �����Z���܂��͕����̍���
	m_logfont.lfWidth			= 0;							// ���ϕ�����
	m_logfont.lfEscapement		= 0;							// ��������̕�����X���Ƃ̊p�x
	m_logfont.lfOrientation		= 0;							// �x�[�X���C����X���Ƃ̊p�x
	m_logfont.lfWeight			= FW_REGULAR;					// �t�H���g�̑���
	m_logfont.lfItalic			= FALSE;						// �C�^���b�N�̎w��
	m_logfont.lfUnderline		= FALSE;						// �����t���w��
	m_logfont.lfStrikeOut		= FALSE;						// �ł��������t���w��
	m_logfont.lfCharSet			= DEFAULT_CHARSET;				// �L�����N�^�Z�b�g
	m_logfont.lfOutPrecision	= OUT_DEFAULT_PRECIS;			// �o�͐��x
	m_logfont.lfClipPrecision	= CLIP_DEFAULT_PRECIS;			// �N���b�s���O�̐��x
	m_logfont.lfQuality			= PROOF_QUALITY;				// �o�͕i��
	m_logfont.lfPitchAndFamily	= FIXED_PITCH | FF_DONTCARE;	// �s�b�`�ƃt�@�~��
	strcpy( m_logfont.lfFaceName, "�l�r �S�V�b�N" );			// �t�H���g��
	
	// str param �ɏ����l�ݒ�
	#define DEF_STR_PARAM( id, var, init, conf_name ) strcpy( var, init );
	#include "def_str_param.h"
	
	m_Script	= NULL;
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdFilter::~CVsdFilter (){
	delete m_VsdLog;
	delete m_GPSLog;
	delete [] m_Lap;
	delete [] m_szLogFile;
	delete [] m_szGPSLogFile;
	delete [] m_Polygon;
	delete m_pFont;
	delete m_Script;
}

/*** �ݒ胍�[�h *************************************************************/

char *CVsdFilter::IsConfigParam( const char *szParamName, char *szBuf, int &iVal ){
	
	int	iLen;
	
	while( isspace( *szBuf )) ++szBuf;
	
	if(
		strncmp( szBuf, szParamName, iLen = strlen( szParamName )) == 0 &&
		szBuf[ iLen ] == '='
	){
		iVal = atoi( szBuf + iLen + 1 );
		return szBuf + iLen + 1;
	}
	
	return NULL;
}

char *CVsdFilter::IsConfigParamStr( const char *szParamName, char *szBuf, char *szDst ){
	
	int		iLen;
	char	*p;
	
	while( isspace( *szBuf )) ++szBuf;
	
	if(
		strncmp( szBuf, szParamName, iLen = strlen( szParamName )) == 0 &&
		szBuf[ iLen ] == '='
	){
		szBuf += iLen + 1;	// " ���w���Ă���͂�
		
		// ������擪
		if( p = strchr( szBuf, '"' )){
			szBuf = p + 1;
		}
		
		strcpy( szDst, szBuf );
		
		// ������I�[
		if(( p = strchr( szDst, '"' )) || ( p = strchr( szDst, ',' ))){
			*p = '\0';
		}
		
		return szDst;
	}
	
	return NULL;
}

BOOL CVsdFilter::ParseMarkStr( const char *szMark ){
	
	do{
		while( *szMark && !isdigit( *szMark )) ++szMark;	// �����܂ŃX�L�b�v
		if( !*szMark ) break;
		
		SetFrameMark( atoi( szMark ));
		if( szMark = strchr( szMark, ',' )) ++szMark;	// ���̃p�����[�^
	}while( szMark );
	
	m_bCalcLapTimeReq = TRUE;
	return TRUE;
}

BOOL CVsdFilter::ConfigLoad( const char *szFileName ){
	
	int 	i, iVal;
	FILE	*fp;
	char	szBuf[ BUF_SIZE ];
	
	if(( fp = fopen( szFileName, "r" )) != NULL ){
		m_bCalcLapTimeReq = TRUE;
		
		while( fgets( szBuf, BUF_SIZE, fp )){
			if( char *p = IsConfigParam( "mark", szBuf, iVal )){
				// ���b�v�^�C���}�[�N
				ParseMarkStr( p + 1 );
			}
			
			// str param �̃��[�h
			#define DEF_STR_PARAM( id, var, init, conf_name ) else if( IsConfigParamStr( conf_name, szBuf, var ));
			#include "def_str_param.h"
			
			else{
				// Mark �ȊO�̃p�����[�^
				for( i = 0; i < TRACK_N; ++i ){
					if(
						m_szTrackbarName[ i ] &&
						IsConfigParam( m_szTrackbarName[ i ], szBuf, iVal )
					){
						m_piParamT[ i ] = iVal;
						goto Next;
					}
				}
				
				for( i = 0; i < CHECK_N; ++i ){
					if(
						m_szCheckboxName[ i ] &&
						IsConfigParam( m_szCheckboxName[ i ], szBuf, iVal )
					){
						m_piParamC[ i ] = iVal;
						goto Next;
					}
				}
				
				for( i = 0; i < SHADOW_N; ++i ){
					if( IsConfigParam( m_szShadowParamName[ i ], szBuf, iVal )){
						m_piParamS[ i ] = iVal;
						goto Next;
					}
				}
			}
		  Next: ;
		}
		fclose( fp );
	}
	return TRUE;
}

/*** GPS ���O���[�h ********************************************************/

double CVsdFilter::GPSLogGetLength(
	double dLong0, double dLati0,
	double dLong1, double dLati1
){
	// �q���x�j�̌��� http://yamadarake.jp/trdi/report000001.html
	const double a	= 6378137.000;
	const double b	= 6356752.314245;
	const double e2	= ( a * a - b * b ) / ( a * a );
	
	double dx	= ( dLong1 - dLong0 ) * ToRAD;
	double dy	= ( dLati1 - dLati0 ) * ToRAD;
	double uy	= ( dLati0 + dLati1 ) / 2 * ToRAD;
	double W	= sqrt( 1 - e2 * sin( uy ) * sin( uy ));
	double M	= a * ( 1 - e2 ) / pow( W, 3 );
	double N	= a / W;
	
	return	sqrt( dy * dy * M * M + pow( dx * N * cos( uy ), 2 ));
}

BOOL CVsdFilter::GPSLogLoad( const char *szFileName ){
	
	UINT	uGPSCnt = 0;
	TCHAR	szBuf[ BUF_SIZE ];
	
	double	dLati, dLati0 = 0;
	double	dLong, dLong0 = 0;
	double	dLati2Meter, dLong2Meter;
	double	dSpeed;
	double	dBearing;
	double	dTime, dTime0;
	gzFile	fp;
	
	UINT	u;
	UINT	uSameCnt = 0;
	
	if( m_GPSLog ){
		delete m_GPSLog;
		m_GPSLog = NULL;
	}
	
	GPS_LOG_t	*GPSLog = new GPS_LOG_t[ ( int )( MAX_VSD_LOG * GPS_FREQ / LOG_FREQ ) ];
	
	while( *szFileName ){
		
		// �t�@�C������ / �ŕ���
		char const *p = szFileName;
		if( !( p = strchr( szFileName, '/' ))) p = strchr( szFileName, '\0' );
		strncpy( szBuf, szFileName, p - szFileName );
		*( szBuf + ( p - szFileName )) = '\0';
		szFileName = *p ? p + 1 : p;	// p == '/' �Ȃ�X�L�b�v
		
		if(( fp = gzopen(( char *)szBuf, "rb" )) == NULL ) return FALSE;
		
		/*** dp3 ****************************************************************/
		
		if( IsExt(( char *)szBuf, "dp3" )){
			
			gzseek( fp, 0x100, SEEK_CUR );
			
			#define BigEndianI( p )	( \
				( *(( UCHAR *)szBuf + p + 0 ) << 24 ) | \
				( *(( UCHAR *)szBuf + p + 1 ) << 16 ) | \
				( *(( UCHAR *)szBuf + p + 2 ) <<  8 ) | \
				( *(( UCHAR *)szBuf + p + 3 )       ))
			
			#define BigEndianS( p )	( \
				( *(( UCHAR *)szBuf + p + 0 ) <<  8 ) | \
				( *(( UCHAR *)szBuf + p + 1 )       ))
			
			while( gzread( fp, szBuf, 16 )){
				
				u = BigEndianI( 0 ) & 0x3FFFFF;
				// �l�␳
				// 2254460 �� 22:54:46.0
				dTime =	u / 100000 * 3600 +
						u / 1000 % 100 * 60 +
						( u % 1000 ) / 10.0;
				
				dLati = BigEndianI( 8 ) / 460800.0;
				dLong = BigEndianI( 4 ) / 460800.0;
				
				if( uGPSCnt == 0 ){
					dLati0 = dLati;
					dLong0 = dLong;
					dTime0 = dTime;
					
					dLong2Meter = GPSLogGetLength( dLong, dLati, dLong + 1.0 / 3600, dLati ) * 3600;
					dLati2Meter = GPSLogGetLength( dLong, dLati, dLong, dLati + 1.0 / 3600 ) * 3600;
				}
				
				if( dTime < dTime0 ) dTime += 24 * 3600;
				dTime -= dTime0;
				
				// �P�ʂ�␳
				// �ܓx�E�o�x�����[�g��
				GPSLog[ uGPSCnt ].fX = ( float )(( dLong - dLong0 ) * dLong2Meter );
				GPSLog[ uGPSCnt ].fY = ( float )(( dLati0 - dLati ) * dLati2Meter );
				
				// ���x�E�������x�N�g�����W
				GPSLog[ uGPSCnt ].fSpeed	= ( float )( BigEndianS( 12 ) / 10.0 );
				GPSLog[ uGPSCnt ].fBearing	= ( float )BigEndianS( 14 );
				GPSLog[ uGPSCnt ].fTime 	= ( float )dTime;
				
				if( uGPSCnt >=2 ){
					if( GPSLog[ uGPSCnt - 1 ].fTime == GPSLog[ uGPSCnt ].fTime ){
						// �������������O�������Ƃ����̃J�E���g������
						++uSameCnt;
					}else if( uSameCnt ){
						// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
						++uSameCnt;
						
						for( u = 1; u < uSameCnt; ++ u ){
							GPSLog[ uGPSCnt - uSameCnt + u ].fTime +=
								( GPSLog[ uGPSCnt ].fTime - GPSLog[ uGPSCnt - uSameCnt ].fTime )
								/ uSameCnt * u;
						}
						uSameCnt = 0;
					}
				}
				uGPSCnt++;
			}
		}
		
		/*** dp3x ***************************************************************/
		
		if( IsExt(( char *)szBuf, "dp3x" )){
			
			// ���_�擾
			gzread( fp, szBuf, 0x78 );
			double dLatiBase = *( int *)( szBuf + 0x54 ) / 460800.0;
			double dLongBase = *( int *)( szBuf + 0x50 ) / 460800.0;
			
			// ���Ԏ擾 UTC * 1000
			dTime = fmod(( double )*( __int64 *)( szBuf + 0x48 ) / 1000, 3600 * 24 )
				+ 9 * 3600; // �Ȃ��� UTC-9 �̎��ԂȂ̂ŁC�␳
			
			while( gzread( fp, szBuf, 18 )){
				
				dLati = *( short int *)( szBuf + 0x2 ) / 460800.0 + dLatiBase;
				dLong = *( short int *)( szBuf + 0x0 ) / 460800.0 + dLongBase;
				
				if( uGPSCnt == 0 ){
					dLati0 = dLati;
					dLong0 = dLong;
					dTime0 = dTime;
					
					dLong2Meter = GPSLogGetLength( dLong, dLati, dLong + 1.0 / 3600, dLati ) * 3600;
					dLati2Meter = GPSLogGetLength( dLong, dLati, dLong, dLati + 1.0 / 3600 ) * 3600;
				}
				
				// �P�ʂ�␳
				// �ܓx�E�o�x�����[�g��
				GPSLog[ uGPSCnt ].fX = ( float )(( dLong - dLong0 ) * dLong2Meter );
				GPSLog[ uGPSCnt ].fY = ( float )(( dLati0 - dLati ) * dLati2Meter );
				
				// ���x�E�������x�N�g�����W
				GPSLog[ uGPSCnt ].fSpeed	= ( float )( *( short int *)( szBuf + 0x4 ) / 10.0 );
				
				GPSLog[ uGPSCnt ].fBearing	= FLT_MAX;
				GPSLog[ uGPSCnt ].fTime 	= ( float )( dTime - dTime0 );
				
				uGPSCnt++;
				dTime += 0.2;
			}
		}
		
		/*** nmea ***************************************************************/
		
		else while( gzgets( fp, szBuf, BUF_SIZE ) != Z_NULL ){
			
			char	cNorthSouth;
			char	cEastWest;
			UINT	uParamCnt;
			
			uParamCnt = sscanf( szBuf,
				"$GPRMC,"
				"%lg,%*c,"	// time
				"%lg,%c,"	// lat
				"%lg,%c,"	// long
				"%lg,%lg,",	// speed, bearing
				// 1	2		3				4		5			6		7
				&dTime, &dLati, &cNorthSouth, &dLong, &cEastWest, &dSpeed, &dBearing
			);
			
			// $GPRMC �Z���e���X�ȊO�̓X�L�b�v
			if( uParamCnt < 5 ) continue;
			
			// �C�O�Ή�w
			if( cNorthSouth == 'S' ) dLati = -dLati;
			if( cEastWest   == 'W' ) dLong = -dLong;
			
			// �l�␳
			// 225446.00 �� 22:54:46.00
			dTime =	( int )dTime / 10000 * 3600 +
					( int )dTime / 100 % 100 * 60 +
					fmod( dTime, 100 );
			
			// 4916.452653 �� 49�x16.45��
			dLati =	( int )dLati / 100 + fmod( dLati, 100 ) / 60;
			dLong =	( int )dLong / 100 + fmod( dLong, 100 ) / 60;
			
			if( uGPSCnt == 0 ){
				dLati0 = dLati;
				dLong0 = dLong;
				dTime0 = dTime;
				
				dLong2Meter = GPSLogGetLength( dLong, dLati, dLong + 1.0 / 3600, dLati ) * 3600;
				dLati2Meter = GPSLogGetLength( dLong, dLati, dLong, dLati + 1.0 / 3600 ) * 3600;
			}
			
			if( dTime < dTime0 ) dTime += 24 * 3600;
			dTime -= dTime0;
			
			// �P�ʂ�␳
			// �ܓx�E�o�x�����[�g��
			GPSLog[ uGPSCnt ].fX = ( float )(( dLong - dLong0 ) * dLong2Meter );
			GPSLog[ uGPSCnt ].fY = ( float )(( dLati0 - dLati ) * dLati2Meter );
			
			// ���x�E�������x�N�g�����W
			GPSLog[ uGPSCnt ].fSpeed	= uParamCnt < 6 ? FLT_MAX : ( float )( dSpeed * 1.852 ); // knot/h �� km/h
			GPSLog[ uGPSCnt ].fBearing	= uParamCnt < 7 ? FLT_MAX : ( float )dBearing;
			GPSLog[ uGPSCnt ].fTime		= ( float )dTime;
			
			if( uGPSCnt >=2 ){
				if( GPSLog[ uGPSCnt - 1 ].fTime == GPSLog[ uGPSCnt ].fTime ){
					// �������������O�������Ƃ����̃J�E���g������
					++uSameCnt;
				}else if( uSameCnt ){
					// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
					++uSameCnt;
					
					for( u = 1; u < uSameCnt; ++ u ){
						GPSLog[ uGPSCnt - uSameCnt + u ].fTime +=
							( GPSLog[ uGPSCnt ].fTime - GPSLog[ uGPSCnt - uSameCnt ].fTime )
							/ uSameCnt * u;
					}
					uSameCnt = 0;
				}
			}
			
			// 20km/h �ȉ��� 3�b�ȏ� log �̊Ԋu���J���Ƃ��C0km/h �� log ��⊮����
			if(
				uGPSCnt >= 2 &&
				GPSLog[ uGPSCnt ].fTime - GPSLog[ uGPSCnt - 1 ].fTime >= 3 &&
				GPSLog[ uGPSCnt ].fSpeed <= 20
			){
				// -1 +0 +1 +2
				// A  B
				//  ��
				// A  A' B' B
				
				// �f�[�^ B �̃R�s�[
				GPSLog[ uGPSCnt + 1 ] =
				GPSLog[ uGPSCnt + 2 ] = GPSLog[ uGPSCnt ];
				
				// �f�[�^ A �̃R�s�[
				GPSLog[ uGPSCnt ] = GPSLog[ uGPSCnt - 1 ];
				
				// �X�s�[�h�� 0 ��
				GPSLog[ uGPSCnt ].fSpeed = GPSLog[ uGPSCnt + 1 ].fSpeed = 0;
				
				// ���Ԓ���
				float fDiff = GPSLog[ uGPSCnt - 1 ].fTime - GPSLog[ uGPSCnt - 2 ].fTime;
				GPSLog[ uGPSCnt     ].fTime += fDiff;
				GPSLog[ uGPSCnt + 1 ].fTime -= fDiff;
				
				uGPSCnt += 2;
			}
			
			uGPSCnt++;
		}
		
		gzclose( fp );
	}
	
	/************************************************************************/
	
	// �A�b�v�R���o�[�g�p�o�b�t�@�m�ہE������
	m_GPSLog = new CVsdLog;
	m_GPSLog->GPSLogUpConvert( GPSLog, uGPSCnt, TRUE );
	m_GPSLog->RotateMap( m_piParamT[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	
	DebugCmd( {
		FILE *fpp = fopen( "D:\\DDS\\vsd\\vsd_filter\\z_gps_raw.txt", "w" );
		for( u = 0; u < uGPSCnt; ++u ){
			fprintf( fpp, "%g\t%g\t%g\t%g\t%g\t%g\t%g\n",
				GPSLog[ u ].fX,
				GPSLog[ u ].fY,
				GPSLog[ u ].fSpeed,
				GPSLog[ u ].fBearing,
				GPSLog[ u ].fGx,
				GPSLog[ u ].fGy,
				GPSLog[ u ].fTime
			);
		}
		fclose( fpp );
	} )
	
	delete [] GPSLog;
	
	DebugCmd( {
		FILE *fpp = fopen( "D:\\DDS\\vsd\\vsd_filter\\z_upcon_gps.txt", "w" );
		for( u = 0; u < ( UINT )m_GPSLog->m_iCnt; ++u ){
			fprintf( fpp, "%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\n",
				m_GPSLog->m_Log[ u ].fSpeed,
				m_GPSLog->m_Log[ u ].fTacho,
				m_GPSLog->m_Log[ u ].fMileage,
				m_GPSLog->m_Log[ u ].fX,
				m_GPSLog->m_Log[ u ].fX0,
				m_GPSLog->m_Log[ u ].fY,
				m_GPSLog->m_Log[ u ].fY0,
				m_GPSLog->m_Log[ u ].fGx,
				m_GPSLog->m_Log[ u ].fGy
			);
		}
		fclose( fpp );
	} )
	
	DeleteIfZero( m_GPSLog );
	if( m_GPSLog ) m_GPSLog->m_dLogStartTime = dTime0 + 9 * 3600;
	
	return m_GPSLog != NULL;
}

/*** ���O���[�h *************************************************************/

BOOL CVsdFilter::ReadLog( const char *szFileName ){
	
	TCHAR	szBuf[ BUF_SIZE ];
	gzFile	fp;
	BOOL	bCalibrating = FALSE;
	
	if( m_VsdLog ){
		delete m_VsdLog;
		m_VsdLog = NULL;
	}
	
	if(( fp = gzopen(( char *)szFileName, "rb" )) == NULL ) return FALSE;
	
	// GPS ���O�p
	UINT		uGPSCnt = 0;
	GPS_LOG_t	*GPSLog = new GPS_LOG_t[ ( int )( MAX_VSD_LOG / LOG_FREQ ) ];
	
	// ������
	m_iLapNum		= 0;
	m_iBestTime		= BESTLAP_NONE;
	m_iBestLap		= 0;
	m_iLapIdx		= -1;
	m_iBestLogNumRunning	= 0;
	
	m_VsdLog = new CVsdLog;
	
	// ���O���[�h
	
	UINT	uReadCnt, uLap, uMin, uSec, uMSec;
	UINT	uLogNum = 0;
	double	dGcx = 0;
	double	dGcy = 0;
	double	dGx, dGy;
	
	double	dLati;
	double	dLong;
	double	dLati0 = 0;
	double	dLong0 = 0;
	double	dLati2Meter, dLong2Meter;
	double	dSpeed;
	double	dBearing;
	
	m_iLogStart = m_iLogStop = 0;
	
	TCHAR	*p;
	
	int	iLogFreqLog		= 0;
	int	iLogFreqTime	= 0;
	
	while( gzgets( fp, szBuf, BUF_SIZE ) != Z_NULL ){
		if(( p = strstr( szBuf, "LAP" )) != NULL ){ // ���b�v�^�C���L�^��������
			m_iLapMode = LAPMODE_MAGNET;
			
			uReadCnt = sscanf( p, "LAP%d%d:%d.%d", &uLap, &uMin, &uSec, &uMSec );
			
			int iTime = ( uMin * 60 + uSec ) * 1000 + uMSec;
			
			m_Lap[ m_iLapNum ].uLap		= uLap;
			m_Lap[ m_iLapNum ].fLogNum	= ( float )uLogNum;
			m_Lap[ m_iLapNum ].iTime	= ( uReadCnt == 4 ) ? iTime : 0;
			
			if(
				uReadCnt == 4 &&
				( m_iBestTime == BESTLAP_NONE || m_iBestTime > iTime )
			){
				m_iBestTime	= iTime;
				m_iBestLap	= m_iLapNum - 1;
				
				iLogFreqLog	 += uLogNum - ( int )m_Lap[ m_iLapNum - 1 ].fLogNum;
				iLogFreqTime += iTime;
			}
			++m_iLapNum;
		}
		
		if(( p = strstr( szBuf, "GPS" )) != NULL ){ // GPS�L�^��������
			sscanf( p, "GPS%lg%lg%lg%lg", &dLati, &dLong, &dSpeed, &dBearing );
			
			if( uGPSCnt == 0 ){
				dLati0 = dLati;
				dLong0 = dLong;
				
				dLong2Meter = GPSLogGetLength( dLong, dLati, dLong + 1.0 / 3600, dLati ) * 3600;
				dLati2Meter = GPSLogGetLength( dLong, dLati, dLong, dLati + 1.0 / 3600 ) * 3600;
			}
			
			// �P�ʂ�␳
			// �ܓx�E�o�x�����[�g��
			GPSLog[ uGPSCnt ].fX = ( float )(( dLong - dLong0 ) * dLong2Meter );
			GPSLog[ uGPSCnt ].fY = ( float )(( dLati0 - dLati ) * dLati2Meter );
			GPSLog[ uGPSCnt ].fSpeed	= ( float )dSpeed;
			GPSLog[ uGPSCnt ].fBearing	= ( float )dBearing;
			
			// LOG_FREQ �� 15Hz ����������̃��O�ł͊ԈႢ�����C���䕪���������Q�Ȃ�?
			GPSLog[ uGPSCnt++ ].fTime = (( int )uLogNum - GPS_LOG_OFFS ) / ( float )LOG_FREQ;
		}
		
		// ���ʂ� log
		if(( uReadCnt = sscanf( szBuf, "%g%g%g%lg%lg",
			&m_VsdLog->m_Log[ uLogNum ].fTacho,
			&m_VsdLog->m_Log[ uLogNum ].fSpeed,
			&m_VsdLog->m_Log[ uLogNum ].fMileage,
			&dGy,
			&dGx
		)) >= 3 ){
			if( uReadCnt < 5 && uLogNum ){
				// G�f�[�^���Ȃ��Ƃ��́Cspeed���狁�߂適�p�~
				dGx = 0;
				dGy = 0;
				//dGy = ( m_VsdLog->m_Log[ uLogNum ].fSpeed - m_VsdLog->m_Log[ uLogNum - 1 ].fSpeed ) * ( 1000.0 / 3600 / 9.8 * LOG_FREQ );
			}else{
				if( dGx >= 4 ){	
					// �P�ʂ� G �ɕϊ�
					dGx = -dGx / ACC_1G_Y;
					dGy =  dGy / ACC_1G_Z;
				}
				
				if( uLogNum == 0 ){
					// G �Z���^�[�̏����l
					dGcx = dGx;
					dGcy = dGy;
				}
				
				// G �Z���^�[����
				dGx -= dGcx;
				dGy -= dGcy;
				
				// �Î~���Ă���Ǝv�����Ƃ��́CG �Z���^�[��␳����
				// ���s���� == 0 && �}0.02G
				if(
					uLogNum &&
					m_VsdLog->m_Log[ uLogNum - 1 ].fMileage == m_VsdLog->m_Log[ uLogNum ].fMileage &&
					( m_VsdLog->m_Log[ uLogNum - 1 ].fGy - dGy ) >= -0.02 &&
					( m_VsdLog->m_Log[ uLogNum - 1 ].fGy - dGy ) <=  0.02
				){
					dGcx += dGx / 160;
					dGcy += dGy / 160;
				}
			}
			
			m_VsdLog->m_Log[ uLogNum ].fGx = ( float )dGx;
			m_VsdLog->m_Log[ uLogNum ].fGy = ( float )dGy;
			
			// ���O�J�n�E�I���F��
			if( m_VsdLog->m_Log[ uLogNum ].fSpeed >= 300 ){
				if( !bCalibrating ){
					bCalibrating = TRUE;
					m_iLogStart  = m_iLogStop;
					m_iLogStop   = uLogNum;
				}
			}else{
				bCalibrating = FALSE;
				
				if( m_VsdLog->m_iMaxSpeed < m_VsdLog->m_Log[ uLogNum ].fSpeed )
					m_VsdLog->m_iMaxSpeed = ( int )ceil( m_VsdLog->m_Log[ uLogNum ].fSpeed / 10 ) * 10;
			}
			
			// ���[�^�[�␳
			m_VsdLog->m_Log[ uLogNum ].fTacho = ( float )(
				m_VsdLog->m_Log[ uLogNum ].fTacho * m_piParamS[ METER_ADJUST ] / 1000.0
			);
			m_VsdLog->m_Log[ uLogNum ].fSpeed = ( float )(
				m_VsdLog->m_Log[ uLogNum ].fSpeed * m_piParamS[ METER_ADJUST ] / 1000.0
			);
			++uLogNum;
		}
	}
	m_VsdLog->m_iCnt = uLogNum;
	
	// �Â� log �� LOG_FREQ �Ƃ͌���Ȃ��̂ŁC�v�Z�ŋ��߂�
	if( iLogFreqTime ){
		m_VsdLog->m_dFreq = iLogFreqLog / ( iLogFreqTime / 1000.0 );
	}
	
	/*** GPS ���O����O�Ղ����߂� *******************************************/
	
	if( uGPSCnt ){
		m_VsdLog->GPSLogUpConvert( GPSLog, uGPSCnt );
		m_VsdLog->RotateMap( m_piParamT[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	}
	
	delete [] GPSLog;
	
	/************************************************************************/
	
	gzclose( fp );
	
	m_Lap[ m_iLapNum ].fLogNum	= FLT_MAX;	// �Ԍ�
	m_Lap[ m_iLapNum ].iTime	= 0;		// �Ԍ�
	
	DeleteIfZero( m_VsdLog );
	
	return TRUE;
}

/*** ���b�v�ԍ� �� ���O�ԍ� �ϊ� ********************************************/

double CVsdFilter::LapNum2LogNum( CVsdLog *Log, int iLapNum ){
	
	double a;
	
	// iLapNum �����������Ƃ��� 0 ��Ԃ��Ƃ�
	if( iLapNum < 0 ) return 0;
	
	if( m_iLapMode == LAPMODE_MAGNET || m_iLapMode == LAPMODE_HAND_MAGNET ){
		// iLogNum �� VSD ���O�ԍ�
		if( Log == m_VsdLog ) return m_Lap[ iLapNum ].fLogNum;
		if( LogSt == LogEd )  return 0;
		a = ( m_Lap[ iLapNum ].fLogNum - LogSt ) / ( LogEd - LogSt );
		
	}else if( m_iLapMode == LAPMODE_GPS || m_iLapMode == LAPMODE_HAND_GPS ){
		// iLogNum �� GPS ���O�ԍ�
		if( Log == m_GPSLog ) return m_Lap[ iLapNum ].fLogNum;
		if( GPSSt == GPSEd )  return 0;
		a = ( m_Lap[ iLapNum ].fLogNum - GPSSt ) / ( GPSEd - GPSSt );
		
	}else{
		// iLogNum �̓r�f�I�t���[���ԍ�
		if( VideoSt == VideoEd ) return 0;
		a = ( m_Lap[ iLapNum ].fLogNum - VideoSt ) / ( VideoEd - VideoSt );
	}
	
	return Log == m_VsdLog ?
		a * ( LogEd - LogSt ) + LogSt :
		a * ( GPSEd - GPSSt ) + GPSSt;
}

/*** ���b�v�^�C���Čv�Z (�蓮) **********************************************/

void CVsdFilter::CalcLapTime( void ){
	
	int		iTime, iPrevTime;
	int		iFrame = 0;
	double	dLogNum;
	
	m_iLapNum	= 0;
	m_iBestTime	= BESTLAP_NONE;
	
	if( m_VsdLog ){
		m_iLapMode = LAPMODE_HAND_MAGNET;
	}else if( m_GPSLog ){
		m_iLapMode = LAPMODE_HAND_GPS;
	}else{
		m_iLapMode = LAPMODE_HAND_VIDEO;
	}
	
	while(( iFrame = GetFrameMark( iFrame )) >= 0 ){
		
		if( m_iLapMode == LAPMODE_HAND_MAGNET ){
			dLogNum	= ConvParam( iFrame, Video, Log );
			iTime	= ( int )( dLogNum / m_VsdLog->m_dFreq * 1000 );
		}else if( m_iLapMode == LAPMODE_HAND_GPS ){
			dLogNum	= ConvParam( iFrame, Video, GPS );
			iTime	= ( int )( dLogNum / LOG_FREQ * 1000 );
		}else{
			iTime	= ( int )( iFrame * 1000.0 / GetFPS());
		}
		
		m_Lap[ m_iLapNum ].fLogNum	= m_iLapMode == LAPMODE_HAND_VIDEO ? iFrame : ( float )dLogNum;
		
		if( m_piParamT[ TRACK_SLineWidth ] < 0 ){
			// �W���J�[�i���[�h
			m_Lap[ m_iLapNum ].uLap		= ( m_iLapNum / 2 ) + 1;
			m_Lap[ m_iLapNum ].iTime	= ( m_iLapNum & 1 ) ? iTime - iPrevTime : 0;
		}else{
			m_Lap[ m_iLapNum ].uLap		= m_iLapNum;
			m_Lap[ m_iLapNum ].iTime	= m_iLapNum ? iTime - iPrevTime : 0;
		}
		
		if(
			m_iLapNum &&
			m_Lap[ m_iLapNum ].iTime &&
			( m_iBestTime == BESTLAP_NONE || m_iBestTime > m_Lap[ m_iLapNum ].iTime )
		){
			m_iBestTime	= m_Lap[ m_iLapNum ].iTime;
			m_iBestLap	= m_iLapNum - 1;
		}
		
		iPrevTime = iTime;
		++m_iLapNum;
		++iFrame;
	}
	m_Lap[ m_iLapNum ].fLogNum	= FLT_MAX;	// �Ԍ�
	m_Lap[ m_iLapNum ].iTime	= 0;		// �Ԍ�
}

/*** ���b�v�^�C���Čv�Z (GPS auto) ******************************************/

void CVsdFilter::CalcLapTimeAuto( void ){
	
	int iFrame;
	
	if(( iFrame = GetFrameMark( 0 )) < 0 ) return;
	
	m_iLapMode = LAPMODE_GPS;
	
	/*** �X�^�[�g���C���̈ʒu���擾 ***/
	// iFrame �ɑΉ����� GPS ���O�ԍ��擾
	double dLogNum = ConvParam( iFrame, Video, GPS );
	
	int iLogNum = ( int )dLogNum;
	
	// iLogNum �` iLogNum + 1 �̕��ʂ��Z�o
	
	double dAngle = atan2(
		( m_GPSLog->m_Log[ iLogNum + 1 ].fY0 - m_GPSLog->m_Log[ iLogNum ].fY0 ),
		( m_GPSLog->m_Log[ iLogNum + 1 ].fX0 - m_GPSLog->m_Log[ iLogNum ].fX0 )
	);
	
	#define x1 m_dStartLineX1
	#define y1 m_dStartLineY1
	#define x2 m_dStartLineX2
	#define y2 m_dStartLineY2
	#define x3 m_GPSLog->m_Log[ i ].fX0
	#define y3 m_GPSLog->m_Log[ i ].fY0
	#define x4 m_GPSLog->m_Log[ i + 1 ].fX0
	#define y4 m_GPSLog->m_Log[ i + 1 ].fY0
	
	// ���z���d�ǂ̈ʒu�����߂�
	x2 = m_GPSLog->X0( dLogNum );	// �X�^�[�g�n�_
	y2 = m_GPSLog->Y0( dLogNum );
	
	x1 = x2 + cos( dAngle + 90 * ToRAD ) * SLineWidth / 2;
	y1 = y2 + sin( dAngle + 90 * ToRAD ) * SLineWidth / 2;
	x2 = x2 + cos( dAngle - 90 * ToRAD ) * SLineWidth / 2;
	y2 = y2 + sin( dAngle - 90 * ToRAD ) * SLineWidth / 2;
	
	/*****/
	
	m_iLapNum	= 0;
	m_iBestTime	= BESTLAP_NONE;
	
	int iTime, iPrevTime;
	int	iLapNum = 0;
	
	for( int i = 0; i < m_GPSLog->m_iCnt - 1; ++i ){
		
		/*** ��������C��_���� ***/
		double s1, s2, a;
		
		// ��_���X�^�[�g���C�������ォ�̔���
		s1 = ( x4 - x3 ) * ( y1 - y3 ) - ( x1 - x3 ) * ( y4 - y3 );
		s2 = ( x4 - x3 ) * ( y3 - y2 ) - ( x3 - x2 ) * ( y4 - y3 );
		a = ( s1 + s2 == 0 ) ? -1 : s1 / ( s1 + s2 );
		if( !( 0 <= a && a <= 1 )) continue;
		
		// ��_�� iLogNum �` +1 �����ォ�̔���
		s1 = ( x2 - x1 ) * ( y3 - y1 ) - ( y2 - y1 ) * ( x3 - x1 );
		s2 = ( x2 - x1 ) * ( y1 - y4 ) - ( y2 - y1 ) * ( x1 - x4 );
		a = ( s1 + s2 == 0 ) ? -1 : s1 / ( s1 + s2 );
		if( !( 0 <= a && a < 1 )) continue;
		
		// �i�s�����̔���CdAngle �}45�x
		double dAngle2 = dAngle - atan2(
			( m_GPSLog->m_Log[ i + 1 ].fY0 - m_GPSLog->m_Log[ i ].fY0 ),
			( m_GPSLog->m_Log[ i + 1 ].fX0 - m_GPSLog->m_Log[ i ].fX0 )
		);
		if     ( dAngle2 < -180 * ToRAD ) dAngle2 += 360 * ToRAD;
		else if( dAngle2 >  180 * ToRAD ) dAngle2 -= 360 * ToRAD;
		if( dAngle2 < -45 * ToRAD || dAngle2 > 45 * ToRAD ) continue;
		
		#undef x1
		#undef y1
		#undef x2
		#undef y2
		#undef x3
		#undef y3
		#undef x4
		#undef y4
		
		// ���[�� LogNum
		dLogNum = i + a;
		iTime = ( int )( dLogNum / LOG_FREQ * 1000 );
		iPrevTime;
		
		if( m_piParamS[ SHADOW_LAP_START ] - 1 <= iLapNum && iLapNum <= m_piParamS[ SHADOW_LAP_END ] ){
			m_Lap[ m_iLapNum ].uLap		= m_iLapNum;
			m_Lap[ m_iLapNum ].fLogNum	= ( float )dLogNum;
			m_Lap[ m_iLapNum ].iTime	= m_iLapNum ? iTime - iPrevTime : 0;
			
			if(
				m_iLapNum &&
				( m_iBestTime == BESTLAP_NONE || m_iBestTime > m_Lap[ m_iLapNum ].iTime )
			){
				m_iBestTime	= m_Lap[ m_iLapNum ].iTime;
				m_iBestLap	= m_iLapNum - 1;
			}
			
			iPrevTime = iTime;
			++m_iLapNum;
		}
		++iLapNum;
	}
	m_Lap[ m_iLapNum ].fLogNum	= FLT_MAX;	// �Ԍ�
	m_Lap[ m_iLapNum ].iTime	= 0;		// �Ԍ�
}
