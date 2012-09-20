/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdLog.cpp - CVsdLog class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "dds_lib/dds_lib.h"
#include "../vsd/main.h"
#include "CVsdLog.h"
#include "CScript.h"
#include "CVsdFont.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "CVsdFile.h"
#include "error_code.h"

/*** macros *****************************************************************/

#define GRAVITY			9.80665

/*** �R���X�g���N�^ *********************************************************/

CVsdLog::CVsdLog(){
	m_dFreq	= LOG_FREQ;
	
	m_dLogStartTime	= -1;
	
	m_dLong0	=
	m_dLati0	= 0;
	m_iCnt		= 0;
	
	#define DEF_LOG( name )	m_pLog##name = NULL;
	#include "def_log.h"
}

/*** frame# �ɑΉ����郍�O index �𓾂� *************************************/

double CVsdLog::GetIndex(
	double	dFrame,
	int iVidSt, int iVidEd,
	int iLogSt, int iLogEd,
	int iPrevIdx
){
	if( iVidSt == iVidEd ) return 0;
	return GetIndex(
		( iLogSt + ( iLogEd - iLogSt ) * ( dFrame - iVidSt ) / ( double )( iVidEd - iVidSt )) / SLIDER_TIME,
		iPrevIdx
	);
}

double CVsdLog::GetIndex( double dTime, int iPrevIdx ){
	int idx;
	
	// Time( idx ) <= dTime < Time( idx + 1 )
	// �ƂȂ� idx ��������
	if(
		iPrevIdx < 0 || iPrevIdx >= GetCnt() ||
		Time( iPrevIdx ) > dTime
	){
		// iPrevIdx �����������̂ŁCbinary serch ����
		int iSt = 0;
		int iEd = GetCnt() - 1;
		while( 1 ){
			idx = ( iSt + iEd ) / 2;
			if( iSt == iEd ) break;
			
			if( Time( idx ) > dTime ){
				iEd = idx - 1;
			}else if( Time( idx + 1 ) <= dTime ){
				iSt = idx + 1;
			}else{
				// �q�b�g
				break;
			}
		}
	}else{
		// iPrevIdx �͐���Ȃ̂ŁC������N�_�ɒP���T�[�`����
		idx = iPrevIdx;
		while( Time( idx + 1 ) <= dTime ) ++idx;
	}
	
	// index �̒[�������߂�
	return idx +
		( dTime           - Time( idx )) /
		( Time( idx + 1 ) - Time( idx ));
}

/*** GPS ���O�̃_���v *******************************************************/

#ifdef DEBUG
void CVsdLog::Dump( char *szFileName ){
	FILE *fp = fopen( szFileName, "w" );
	
	BOOL bFirst = TRUE;
	std::map<std::string, VSD_LOG_t *>::iterator it;
	
	// �w�b�_�o��
	for( it = m_Logs.begin(); it != m_Logs.end(); ++it ){
		if( !bFirst ) fputs( "\t", fp ); bFirst = FALSE;
		fputs( it->first.c_str(), fp );
	}
	fputs( "\n", fp );
	
	for( int i = 0; i < GetCnt(); ++i ){
		BOOL bFirst = TRUE;
		for( it = m_Logs.begin(); it != m_Logs.end(); ++it ){
			if( !bFirst ) fputs( "\t", fp ); bFirst = FALSE;
			fprintf( fp, "%f", it->second->Get( i ));
		}
		fputs( "\n", fp );
	}
	fclose( fp );
}
#endif

/*** ���O�� Set *************************************************************/

void CVsdLog::Set( const char *szKey, int iIndex, double dVal ){
	std::string strKey( szKey );
	if( m_Logs.find( strKey ) == m_Logs.end()){
		// �v�f�Ȃ��C���������
		VSD_LOG_t *p;
		m_Logs[ strKey ] = p = new VSD_LOG_t();
		
		// Speed �Ƃ���{�f�[�^�̎Q�Ɨp�|�C���^�ɑ��
		#define DEF_LOG( name )	if( strKey == #name ) m_pLog##name = p;
		#include "def_log.h"
	}
	
	m_Logs[ strKey ]->Set( iIndex, dVal );
	
	if( m_iCnt <= iIndex ) m_iCnt = iIndex + 1;
}

/*** 1���R�[�h�R�s�[ ********************************************************/

void CVsdLog::CopyRecord( int iTo, int iFrom ){
	std::map<std::string, VSD_LOG_t *>::iterator it;
	
	for( it = m_Logs.begin(); it != m_Logs.end(); ++it ){
		VSD_LOG_t *pLog = it->second;
		pLog->Set( iTo, pLog->Get( iFrom ));
	}
}

/*** �Ԍ��ǉ� ***************************************************************/

void CVsdLog::AddWatchDog( void ){
	int iCnt = GetCnt() - 1;
	CopyRecord( 0, 2 );           AddStopRecord( 0, -WATCHDOG_TIME );
	CopyRecord( 1, 2 );           AddStopRecord( 1, Time( 2 ) - 0.5 );
	CopyRecord( iCnt + 1, iCnt ); AddStopRecord( iCnt + 1, Time( iCnt ) + 0.5 );
	CopyRecord( iCnt + 2, iCnt ); AddStopRecord( iCnt + 2, WATCHDOG_TIME );
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogRescan( void ){
	
	if( GetCnt() == 0 ) return 0;			// 2�f�[�^���Ȃ���ΏI��
	
	double	dDistance = 0;
	
	/*** VSD_LOG_t �̕��𑖍����āC���낢��␳ *****************************/
	
	// �ܓx�o�x�����[�g�� �ϊ��萔
	double dLong2Meter = GPSLogGetLength( m_dLong0, m_dLati0, m_dLong0 + 1.0 / 3600, m_dLati0 ) * 3600;
	double dLati2Meter = GPSLogGetLength( m_dLong0, m_dLati0, m_dLong0, m_dLati0 + 1.0 / 3600 ) * 3600;
	
	m_dFreq = 0;
	int		iFreqCnt = 0;
	int		iSameCnt = 0;
	
	// �������Ԃ��A������ꍇ�̎��������ƁC
	for( int i = 1; i < GetCnt(); ++i ){
		if( Time( i - 1 ) == Time( i )){
			// �������������O�������Ƃ����̃J�E���g������
			++iSameCnt;
		}else if( iSameCnt ){
			// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
			++iSameCnt;
			
			// -4 -3 -2 -1 +0
			//  A  A  A  A  B
			//u =  1  2  3
			
			for( int j = 1; j < iSameCnt; ++j ){
				SetTime(
					i - iSameCnt + j,
					Time( i - iSameCnt ) + 
					( Time( i ) - Time( i - iSameCnt )) / iSameCnt * j
				);
			}
			iSameCnt = 0;
		}
	}
	
	#ifdef _OPENMP
		#pragma omp parallel
	#endif
	{
		if( m_pLogX0 && m_pLogY0 ){
			// �ܓx�E�o�x�����[�g��
			#ifdef _OPENMP
				#pragma omp for
			#endif
			for( int i = 0; i < GetCnt(); ++i ){
				SetX0( i, X0( i ) * dLong2Meter );
				SetY0( i, Y0( i ) * dLati2Meter );
			}
			
			// ���s�����Z�o	(���񉻕s�\)
			if( !m_pLogDistance ){
				for( int i = 1; i < GetCnt(); ++i ){
					double x = X0( i - 1 ) - X0( i ); x *= x;
					double y = Y0( i - 1 ) - Y0( i ); y *= y;
					
					dDistance += sqrt( x + y );
					SetDistance( i, dDistance );
				}
			}
			
			// �X�s�[�h����
			if( !m_pLogSpeed ){
				#ifdef _OPENMP
					#pragma omp for
				#endif
				for( int i = 0; i < GetCnt() - 1; ++i ){
					SetSpeed( i,
						( Distance( i + 1 ) - Distance( i ))
						* ( 3600.0 / 1000 ) /
						( Time( i + 1 ) - Time( i ))
					);
				}
				SetSpeed( GetCnt() - 1, 0 );
			}
			
			// G �v�Z
			if( !m_pLogGx ){
				double	dBearingPrev;
				
				#ifdef _OPENMP
					#pragma omp for
				#endif
				for( int i = 0; i < GetCnt() - 1; ++i ){
					// bearing �v�Z
					double dBearing = atan2( Y0( i + 1 ) - Y0( i ), X0( i + 1 ) - X0( i ));
					
					// �� G �v�Z
					if( i >= 2 ){
						// Gx / Gy �����
						SetGy( i,
							( Speed( i + 1 ) - Speed( i ))
							* ( 1 / 3.600 / GRAVITY )
							/ ( Time( i + 1 ) - Time( i ))
						);
						
						// ��G = v��
						double dBearingDelta = dBearing - dBearingPrev;
						if     ( dBearingDelta >  M_PI ) dBearingDelta -= M_PI * 2;
						else if( dBearingDelta < -M_PI ) dBearingDelta += M_PI * 2;
						
						SetGx( i,
							dBearingDelta / GRAVITY
							/ ( Time( i + 1 ) - Time( i ))
							* ( Speed( i ) / 3.600 )
						);
						
						// �}5G �ȏ�́C�폜
						if( Gx( i ) < -3 || Gx( i ) > 3 ){
							SetGx( i, Gx( i - 1 ));
						}
					}
					
					dBearingPrev = dBearing;
					
					// 5km/h �ȏ�̎��̂݁C���O Freq ���v�Z����
					/* ���b��
					if( Speed( i ) >= 5 ){
						m_dFreq += Time( i ) - Time( i - 1 );
						++iFreqCnt;
					}
					*/
				}
				SetGx( GetCnt() - 1, 0 );
				SetGy( GetCnt() - 1, 0 );
				
				// �X���[�W���O
				#define G_SMOOTH_NUM	3
				double	dGx0, dGx1 = 0;
				double	dGy0, dGy1 = 0;
				
				for( int i = ( G_SMOOTH_NUM - 1 ) / 2; i < GetCnt() - G_SMOOTH_NUM / 2; ++i ){
					if( i < 2 || i >= GetCnt() - 2 ) continue;
					
					SetGx( i, dGx0 = (
						( G_SMOOTH_NUM >= 7 ? Gx( i - 3 ) : 0 ) +
						( G_SMOOTH_NUM >= 6 ? Gx( i + 3 ) : 0 ) +
						( G_SMOOTH_NUM >= 5 ? Gx( i - 2 ) : 0 ) +
						( G_SMOOTH_NUM >= 4 ? Gx( i + 2 ) : 0 ) +
						( G_SMOOTH_NUM >= 3 ? Gx( i - 1 ) : 0 ) +
						( G_SMOOTH_NUM >= 2 ? Gx( i + 1 ) : 0 ) +
						Gx( i + 0 )
					) / G_SMOOTH_NUM );
					SetGy( i, dGy0 = (
						( G_SMOOTH_NUM >= 7 ? Gy( i - 3 ) : 0 ) +
						( G_SMOOTH_NUM >= 6 ? Gy( i + 3 ) : 0 ) +
						( G_SMOOTH_NUM >= 5 ? Gy( i - 2 ) : 0 ) +
						( G_SMOOTH_NUM >= 4 ? Gy( i + 2 ) : 0 ) +
						( G_SMOOTH_NUM >= 3 ? Gy( i - 1 ) : 0 ) +
						( G_SMOOTH_NUM >= 2 ? Gy( i + 1 ) : 0 ) +
						Gy( i + 0 )
					) / G_SMOOTH_NUM );
					
					dGx1 = dGx1 * 0.9 + dGx0 * 0.1;
					dGy1 = dGy1 * 0.9 + dGy0 * 0.1;
					/* ���b��
					if( MaxGx() < fabs( dGx1 )) MaxGx() = fabs( dGx1 );
					if( MaxGy() < dGy1 ) MaxGy() = dGy1;
					if( MinGy() > dGy1 ) MinGy() = dGy1;
					*/
				}
			}
		}
  	}
	
	// ���b��
	//m_dFreq = iFreqCnt / m_dFreq;
	m_dFreq = 10;
	
	return GetCnt();
}

/*** �����Z�o **************************************************************/

double CVsdLog::GPSLogGetLength(
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

/*** ���O���[�h by JavaScript **********************************************/

int CVsdLog::ReadGPSLog( const char *szFileName ){
	TCHAR	szCurDir[ MAX_PATH + 1 ];
	TCHAR	szBuf[ BUF_SIZE ];
	
	getcwd( szCurDir, MAX_PATH );	// �J�����g dir
	
	// �}���`�t�@�C���̏ꍇ�C1�ڂ� dir ���Ȃ̂ł����� cd
	char const *p;
	if( p = strchr( szFileName, '/' )){
		strncpy( szBuf, szFileName, p - szFileName );
		*( szBuf + ( p - szFileName )) = '\0';
		chdir( szBuf );
		
		szFileName = p + 1;
	}
	
	{
		// JavaScript �I�u�W�F�N�g������
		CScript Script;
		Script.Initialize();
		if( Script.RunFile( L"log_reader\\nmea.js" ) != ERR_OK ){
			return 0;	// �G���[
		}
		
		while( *szFileName ){
			
			// �t�@�C������ / �ŕ���
			p = szFileName;
			if( !( p = strchr( szFileName, '/' ))) p = strchr( szFileName, '\0' );
			strncpy( szBuf, szFileName, p - szFileName );
			*( szBuf + ( p - szFileName )) = '\0';
			szFileName = *p ? p + 1 : p;	// p == '/' �Ȃ�X�L�b�v
			
			// �X�N���v�g���s
			LPWSTR pStr = NULL;
			StringNew( pStr, szBuf );
			
			Script.Run_s( L"ReadLog", pStr );
			delete [] pStr;
		}
		
		chdir( szCurDir );	// pwd �����ɖ߂�
		
		/*** JS �� Log �ɃA�N�Z�X *******************************************/
		
		{
			#ifdef AVS_PLUGIN
				v8::Isolate::Scope IsolateScope( Script.m_pIsolate );
			#endif
			v8::HandleScope handle_scope;
			v8::Context::Scope context_scope( Script.m_Context );
			
			v8::Local<v8::Array> hArray = v8::Local<v8::Array>::Cast( Script.m_Context->Global()->Get( v8::String::New(( uint16_t *)L"Log" )));
			if( !hArray->IsArray()) return 0;
			
			v8::Local<v8::Array> Keys = hArray->GetPropertyNames();
			
			for( UINT u = 0; u < Keys->Length(); ++u ){
				v8::String::Value str0( Keys->Get( u ));
			}
		}
	}
	
	/************************************************************************/
	
	if( GetCnt()){
		AddWatchDog();
		
		#define DUMP_LOG
		#if defined DEBUG && defined DUMP_LOG
			Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_raw.txt" );
		#endif
		
		// ���O�ăX�L����
		GPSLogRescan();
		
		#if defined DEBUG && defined DUMP_LOG
			Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_upcon.txt" );
		#endif
		
		m_dLogStartTime += 9 * 3600;
	}
	return GetCnt();
}

/*** GPS ���O���[�h ********************************************************/

#if 0
int CVsdLog::ReadGPSLog( const char *szFileName ){
	TCHAR	szCurDir[ MAX_PATH + 1 ];
	TCHAR	szBuf[ BUF_SIZE ];
	
	double	dLati;
	double	dLong;
	double	dSpeed;
	double	dTime;
	gzFile	fp;
	
	UINT	u;
	int		iCnt = WATCHDOG_REC_NUM;
	
	getcwd( szCurDir, MAX_PATH );	// �J�����g dir
	
	// �}���`�t�@�C���̏ꍇ�C1�ڂ� dir ���Ȃ̂ł����� cd
	char const *p;
	if( p = strchr( szFileName, '/' )){
		strncpy( szBuf, szFileName, p - szFileName );
		*( szBuf + ( p - szFileName )) = '\0';
		chdir( szBuf );
		
		szFileName = p + 1;
	}
	
	while( *szFileName ){
		
		// �t�@�C������ / �ŕ���
		p = szFileName;
		if( !( p = strchr( szFileName, '/' ))) p = strchr( szFileName, '\0' );
		strncpy( szBuf, szFileName, p - szFileName );
		*( szBuf + ( p - szFileName )) = '\0';
		szFileName = *p ? p + 1 : p;	// p == '/' �Ȃ�X�L�b�v
		
		if(( fp = gzopen(( char *)szBuf, "rb" )) == NULL ) return 0;
		
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
				// �l�␳
				// 2254460 �� 22:54:46.0
				u = BigEndianI( 0 ) & 0x3FFFFF;
				SetDateTime( iCnt,
					u / 100000 * 3600 +
					u / 1000 % 100 * 60 +
					( u % 1000 ) / 10.0
				);
				
				// ���x
				SetSpeed( iCnt, BigEndianS( 12 ) / 10.0 );
				SetLongitude( iCnt, BigEndianI( 4 ) / 460800.0 );
				SetLatitude(  iCnt, BigEndianI( 8 ) / 460800.0 );
				
				++iCnt;
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
				SetDateTime( iCnt, dTime + ( GetCnt() - 2 ) / 5 );	// 5Hz �Œ�炵��
				
				// ���x
				SetSpeed( iCnt, *( short int *)( szBuf + 0x4 ) / 10.0 );
				SetLongitude( iCnt, *( short int *)( szBuf + 0x0 ) / 460800.0 + dLongBase );
				SetLatitude(  iCnt, *( short int *)( szBuf + 0x2 ) / 460800.0 + dLatiBase );
				
				++iCnt;
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
				"%lg,",		// speed
				// 1	2		3				4		5			6
				&dTime, &dLati, &cNorthSouth, &dLong, &cEastWest, &dSpeed
			);
			
			// $GPRMC �Z���e���X�ȊO�̓X�L�b�v
			if( uParamCnt < 5 ) continue;
			
			// �l�␳
			// 225446.00 �� 22:54:46.00
			dTime =	( int )dTime / 10000 * 3600 +
					( int )dTime / 100 % 100 * 60 +
					fmod( dTime, 100 );
			
			// �ܓx�o�x�̕ϊ�: 4916.452653 �� 49�x16.45��
			dLong = ( int )dLong / 100 + fmod( dLong, 100 ) / 60;
			dLati = ( int )dLati / 100 + fmod( dLati, 100 ) / 60;
			
			// �C�O�Ή�w
			if( cEastWest   == 'W' ) dLong = -dLong;
			if( cNorthSouth == 'S' ) dLati = -dLati;
			
			SetDateTime( iCnt, dTime );
			
			// ���x
			if( uParamCnt >= 6 ) SetSpeed( iCnt, dSpeed * 1.852 ); // knot/h �� km/h
			SetLongitude( iCnt, dLong );
			SetLatitude(  iCnt, dLati );
			
			++iCnt;
		}
		
		gzclose( fp );
	}
	
	chdir( szCurDir );	// pwd �����ɖ߂�
	
	/************************************************************************/
	
	if( GetCnt()){
		AddWatchDog();
		
		#define DUMP_LOG
		#if defined DEBUG && defined DUMP_LOG
			Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_raw.txt" );
		#endif
		
		// ���O�ăX�L����
		GPSLogRescan();
		
		#if defined DEBUG && defined DUMP_LOG
			Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_upcon.txt" );
		#endif
		
		m_dLogStartTime += 9 * 3600;
	}
	return GetCnt();
}
#endif

/*** ���O���[�h *************************************************************/

int CVsdLog::ReadLog( const char *szFileName, CLapLog *&pLapLog ){
	
	TCHAR	szBuf[ BUF_SIZE ];
	gzFile	fp;
	BOOL	bCalibrating = FALSE;
	
	if(( fp = gzopen(( char *)szFileName, "rb" )) == NULL ) return 0;
	
	pLapLog			= NULL;
	
	// ���O���[�h
	
	UINT	uReadCnt;
	double	dGcx = 0;
	double	dGcy = 0;
	
	m_iLogStart = m_iLogStop = 0;
	
	TCHAR	*p;
	
	int	iLogFreqLog		= 0;
	int	iLogFreqTime	= 0;
	
	LAP_t	LapTime;
	
	int iCnt = WATCHDOG_REC_NUM;
	
	while( gzgets( fp, szBuf, BUF_SIZE ) != Z_NULL ){
		if(( p = strstr( szBuf, "LAP" )) != NULL ){ // ���b�v�^�C���L�^��������
			
			if( pLapLog == NULL ){
				pLapLog = new CLapLog();
				pLapLog->m_iLapMode = LAPMODE_MAGNET;
			}
			
			UINT	uLap, uMin, uSec, uMSec;
			
			uReadCnt = sscanf( p, "LAP%d%d:%d.%d", &uLap, &uMin, &uSec, &uMSec );
			
			int iTime = ( uMin * 60 + uSec ) * 1000 + uMSec;
			
			LapTime.uLap	= uLap;
			LapTime.fLogNum	= ( float )iCnt;
			LapTime.iTime	= ( uReadCnt == 4 ) ? iTime : 0;
			pLapLog->m_Lap.push_back( LapTime );
			
			if(
				uReadCnt == 4 &&
				( pLapLog->m_iBestTime == TIME_NONE || pLapLog->m_iBestTime > iTime )
			){
				pLapLog->m_iBestTime	= iTime;
				pLapLog->m_iBestLap	= pLapLog->m_iLapNum - 1;
				
				iLogFreqLog	 += iCnt - ( int )pLapLog->m_Lap[ pLapLog->m_iLapNum - 1 ].fLogNum;
				iLogFreqTime += iTime;
			}
			++pLapLog->m_iLapNum;
		}
		
		if(( p = strstr( szBuf, "GPS" )) != NULL ){ // GPS�L�^��������
			double dLong, dLati;
			
			sscanf( p, "GPS%lg%lg", &dLati, &dLong );
			SetLongitude( iCnt, dLong );
			SetLatitude(  iCnt, dLati );
		}
		
		int		iTacho;
		double	dSpeed;
		double	dDistance;
		double	dGx, dGy;
		
		// ���ʂ� log
		if(( uReadCnt = sscanf( szBuf, "%u%lg%lg%lg%lg",
			&iTacho,
			&dSpeed,
			&dDistance,
			&dGy,
			&dGx
		)) >= 3 ){
			
			SetTacho( iCnt, iTacho );
			SetSpeed( iCnt, dSpeed );
			SetDistance( iCnt, dDistance );
			
			if( uReadCnt >= 5 ){
				if( dGx >= 4 ){	
					// �P�ʂ� G �ɕϊ�
					dGx = -dGx / ACC_1G_Y;
					dGy =  dGy / ACC_1G_Z;
				}
				
				if( iCnt == 0 ){
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
					iCnt &&
					Distance( iCnt - 1 ) == Distance( iCnt ) &&
					( Gy( iCnt - 1 ) - dGy ) >= -0.02 &&
					( Gy( iCnt - 1 ) - dGy ) <=  0.02
				){
					dGcx += dGx / 160;
					dGcy += dGy / 160;
				}
				
				SetGx( iCnt, dGx );
				SetGy( iCnt, dGy );
			}
			
			// ���O�J�n�E�I���F��
			if( dSpeed >= 300 ){
				// ��MaxSpeed �� 300km/h �ɂȂ�̕��u��
				if( !bCalibrating ){
					bCalibrating = TRUE;
					m_iLogStart  = m_iLogStop;
					m_iLogStop   = iCnt;
				}
			}else{
				bCalibrating = FALSE;
			}
			
			SetDateTime( iCnt, ( double )( iCnt - WATCHDOG_REC_NUM ) / LOG_FREQ );
			
			++iCnt;
		}
	}
	
	// �Â� log �� LOG_FREQ �Ƃ͌���Ȃ��̂ŁC�v�Z�ŋ��߂�
	if( iLogFreqTime ){
		m_dFreq = iLogFreqLog / ( iLogFreqTime / 1000.0 );
	}
	
	/************************************************************************/
	
	gzclose( fp );
	
	// Lap log �̔Ԍ�
	if( pLapLog ){
		LapTime.fLogNum	= FLT_MAX;	// �Ԍ�
		LapTime.iTime	= 0;		// �Ԍ�
		pLapLog->m_Lap.push_back( LapTime );
	}
	
	if( GetCnt()){
		AddWatchDog();
	}
	
	return GetCnt();
}

/*** MAP ��]���� ***********************************************************/

void CVsdLog::RotateMap( double dAngle ){
	for( int i = 0; i < GetCnt(); ++i ){
		SetX( i,  cos( dAngle ) * X0( i ) + sin( dAngle ) * Y0( i ));
		SetY( i, -sin( dAngle ) * X0( i ) + cos( dAngle ) * Y0( i ));
	}
}
