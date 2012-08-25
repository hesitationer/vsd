/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdLog.cpp - CVsdLog class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "dds_lib/dds_lib.h"
#include "../vsd/main.h"
#include "CVsdLog.h"

/*** macros *****************************************************************/

#define GRAVITY			9.80665

/*** �R���X�g���N�^ *********************************************************/

CVsdLog::CVsdLog(){
	m_iCnt	= 0;
	m_dFreq	= LOG_FREQ;
	
	m_dMaxGx =
	m_dMaxGy =
	m_dMinGy = 0;
	
	m_iMaxSpeed		= 0;
	m_iMaxTacho		= 0;
	m_dLogStartTime	= -1;
	
	m_uSameCnt		= 0;
	
	m_dLong0 =
	m_dLati0 =
	m_dLong2Meter =
	m_dLati2Meter = 0;
}

/*** fraem# �ɑΉ����郍�O index �𓾂� *************************************/

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
	
	if( dTime < 0 ) return -1;
	if( dTime >= Time( m_iCnt - 1 )) return m_iCnt;
	
	// Time( idx ) <= dTime < Time( idx + 1 )
	// �ƂȂ� idx ��������
	if(
		iPrevIdx < 0 || iPrevIdx >= m_iCnt ||
		Time( iPrevIdx ) > dTime
	){
		// iPrevIdx �����������̂ŁCbinary serch ����
		int iSt = 0;
		int iEd = m_iCnt - 1;
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
		( dTime                   - Time( idx )) /
		( Time( idx + 1 ) - Time( idx ));
}

/*** GPS ���O�̃_���v *******************************************************/

#ifdef DEBUG
void CVsdLog::Dump( char *szFileName ){
	FILE *fp = fopen( szFileName, "w" );
	fputs( "Time\tSpeed\tTacho\tDistance\tX\tY\tX0\tY0\tGx\tGy\tBearing\n", fp );
	
	for( int i = 0; i < m_iCnt; ++i ){
		fprintf( fp, "%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\n",
			Time( i ),
			Speed( i ),
			Tacho( i ),
			Distance( i ),
			X( i ),
			Y( i ),
			X0( i ),
			Y0( i ),
			Gx( i ),
			Gy( i ),
			Bearing( i )
		);
	}
	fclose( fp );
}
#endif

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogUpConvert( void ){
	
	int	i;
	
	if( m_iCnt < 2 ) return 0;			// 2�f�[�^���Ȃ���ΏI��
	
	VSD_LOG_t	VsdLogTmp;
	VsdLogTmp.SetTime( FLT_MAX );
	m_Log.push_back( VsdLogTmp );		// �Ԍ�
	
	double	dDistance = 0;
	double	dBearing;
	
	/*** VSD_LOG_t �̕��𑖍����āC���낢��␳ *****************************/
	
	m_dFreq = 0;
	int iFreqCnt = 0;
	
	for( i = 1; i < m_iCnt - 1; ++i ){
		// speed ���Ȃ��ꍇ�̕␳
		if( 0 /* �������b�聚���� */ ){
			m_Log[ i ].SetSpeed(
				sqrt(
					pow( X0( i + 1 ) - X0( i - 1 ), 2 ) +
					pow( Y0( i + 1 ) - Y0( i - 1 ), 2 )
				) / ( Time( i + 1 ) - Time( i - 1 ))
				* ( 3600.0 / 1000 )
			);
		}
		
		// bearing ���Ȃ��ꍇ�̕␳
		m_Log[ i ].SetBearing(
			fmod(
				atan2(
					Y0( i + 1 ) - Y0( i - 1 ),
					X0( i + 1 ) - X0( i - 1 )
				) / ToRAD + ( 360 * 2 - 90 ),
				360
			)
		);
		
		// 5km/h �ȏ�̎��̂݁C���O Freq ���v�Z����
		if( Speed( i ) >= 5 ){
			m_dFreq += Time( i ) - Time( i - 1 );
			++iFreqCnt;
		}
	}
	
	m_dFreq = iFreqCnt / m_dFreq;
	
	m_Log[ 0          ].SetSpeed  ( Speed( 1 ));
	m_Log[ 0          ].SetBearing( Bearing( 1 ));
	m_Log[ m_iCnt - 1 ].SetSpeed  ( Speed( m_iCnt - 2 ));
	m_Log[ m_iCnt - 1 ].SetBearing( Bearing( m_iCnt - 2 ));
	
	for( i = 1; i < m_iCnt - 1; ++i ){
		// Gx / Gy �����
		m_Log[ i ].SetGy(
			( Speed( i + 1 ) - Speed( i - 1 ))
			* ( 1 / 3.600 / GRAVITY )
			/ ( Time( i + 1 ) - Time( i - 1 ))
		);
		if( Gy( i ) > 10 ){
			int a= 0;
		}
		// ��G = v��
		dBearing = Bearing( i + 1 ) - Bearing( i - 1 );
		if     ( dBearing >  180 ) dBearing -= 360;
		else if( dBearing < -180 ) dBearing += 360;
		
		m_Log[ i ].SetGx(
			dBearing * ( ToRAD / GRAVITY )
			/ ( Time( i + 1 ) - Time( i - 1 ))
			* ( Speed( i ) / 3.600 )
		);
		
		// �}5G �ȏ�́C�폜
		if( Gx( i ) < -3 || Gx( i ) > 3 ){
			m_Log[ i ].SetGx( Gx( i - 1 ));
		}
	}
	
	m_Log[ 0 ].SetGx(			Gx( 1 ));
	m_Log[ 0 ].SetGy(			Gy( 1 ));
	m_Log[ m_iCnt - 1 ].SetGx(	Gx( m_iCnt - 2 ));
	m_Log[ m_iCnt - 1 ].SetGy(	Gy( m_iCnt - 2 ));
	
	/************************************************************************/
	
	for( i = 0; i < m_iCnt; ++i ){
		if( m_iMaxSpeed < Speed( i ))
			m_iMaxSpeed = ( int )ceil( Speed( i ));
		
		if( i ) dDistance += sqrt(
			pow( X0( i - 1 ) - X0( i ), 2 ) +
			pow( Y0( i - 1 ) - Y0( i ), 2 )
		);
		
		m_Log[ i ].SetDistance( dDistance );
		m_Log[ i ].SetTacho( 0 );
	}
	
	// �X���[�W���O
	#define G_SMOOTH_NUM	2
	double	dGx0, dGx1 = 0;
	double	dGy0, dGy1 = 0;
	
	for( i = ( G_SMOOTH_NUM - 1 ) / 2; i < m_iCnt - G_SMOOTH_NUM / 2; ++i ){
		m_Log[ i ].SetGx( dGx0 = (
			( G_SMOOTH_NUM >= 5 ? Gx( i - 2 ) : 0 ) +
			( G_SMOOTH_NUM >= 4 ? Gx( i + 2 ) : 0 ) +
			( G_SMOOTH_NUM >= 3 ? Gx( i - 1 ) : 0 ) +
			( G_SMOOTH_NUM >= 2 ? Gx( i + 1 ) : 0 ) +
			Gx( i + 0 )
		) / G_SMOOTH_NUM );
		m_Log[ i ].SetGy( dGy0 = (
			( G_SMOOTH_NUM >= 5 ? Gy( i - 2 ) : 0 ) +
			( G_SMOOTH_NUM >= 4 ? Gy( i + 2 ) : 0 ) +
			( G_SMOOTH_NUM >= 3 ? Gy( i - 1 ) : 0 ) +
			( G_SMOOTH_NUM >= 2 ? Gy( i + 1 ) : 0 ) +
			Gy( i + 0 )
		) / G_SMOOTH_NUM );
		
		dGx1 = dGx1 * 0.9 + dGx0 * 0.1;
		dGy1 = dGy1 * 0.9 + dGy0 * 0.1;
		if( m_dMaxGx < fabs( dGx1 )) m_dMaxGx = fabs( dGx1 );
		if( m_dMaxGy < dGy1 ) m_dMaxGy = dGy1;
		if( m_dMinGy > dGy1 ) m_dMinGy = dGy1;
	}
	
	return m_iCnt;
}

/*** GPS ���O���[�h ********************************************************/

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

#define	getcwd	_getcwd
#define	chdir	_chdir

void CVsdLog::PushRecord( VSD_LOG_t& VsdLogTmp, double dLong, double dLati ){
	
	if( m_iCnt == 0 ){
		m_dLogStartTime = VsdLogTmp.Time();
		
		// ���������̔Ԍ�
		m_Log.push_back( VsdLogTmp );
		m_Log[ 0 ].SetTime( FLT_MIN );
		++m_iCnt;
	}
	
	if( dLong == 0 ){
		// GPS �f�[�^���Ȃ��̂ŁC���O�� x, y �f�[�^�R�s�[
		VsdLogTmp.SetX0( X0( m_iCnt - 1 ));
		VsdLogTmp.SetY0( Y0( m_iCnt - 1 ));
	}else{
		// �ꔭ�ڂ� GPS �f�[�^�C��������_�ɂ���
		if( m_dLong0 == 0 ){
			m_dLong0 = dLong;
			m_dLati0 = dLati;
			m_dLong2Meter = GPSLogGetLength( dLong, dLati, dLong + 1.0 / 3600, dLati ) * 3600;
			m_dLati2Meter = GPSLogGetLength( dLong, dLati, dLong, dLati + 1.0 / 3600 ) * 3600;
		}
		
		// �P�ʂ�␳
		// �ܓx�E�o�x�����[�g��
		VsdLogTmp.SetX0(( dLong - m_dLong0 ) * m_dLong2Meter );
		VsdLogTmp.SetY0(( m_dLati0 - dLati ) * m_dLati2Meter );
		
		if( VsdLogTmp.Time() < m_dLogStartTime ) VsdLogTmp.SetTime( VsdLogTmp.Time() + 24 * 3600 );
		VsdLogTmp.SetTime( VsdLogTmp.Time() - m_dLogStartTime );
	}
	
	DebugCmd(
		VsdLogTmp.SetX( dLong - m_dLong0 );
		VsdLogTmp.SetY( dLati - m_dLati0 );
	)
	
	m_Log.push_back( VsdLogTmp );
	
	if( m_iCnt >= 2 ){
		if( Time( m_iCnt - 1 ) == VsdLogTmp.Time()){
			// �������������O�������Ƃ����̃J�E���g������
			++m_uSameCnt;
		}else if( m_uSameCnt ){
			// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
			++m_uSameCnt;
			
			for( UINT u = 1; u < m_uSameCnt; ++ u ){
				m_Log[ m_iCnt - m_uSameCnt + u ].SetTime(
					Time(( int )( m_iCnt - m_uSameCnt + u )) +
					( VsdLogTmp.Time() - Time(( int )( m_iCnt - m_uSameCnt )))
					/ m_uSameCnt * u
				);
			}
			m_uSameCnt = 0;
		}
		
		// 20km/h �ȉ��� 3�b�ȏ� log �̊Ԋu���J���Ƃ��C0km/h �� log ��⊮����
		if(
			VsdLogTmp.Time() - Time( m_iCnt - 1 ) >= 3 &&
			VsdLogTmp.Speed() <= 20
		){
			// -1 +0 +1 +2
			// A  B
			//  ��
			// A  A' B' B
			
			// [+0] �Ƀf�[�^ A->A' �̃R�s�[
			m_Log[ m_iCnt ] = m_Log[ m_iCnt - 1 ];
			
			// �f�[�^ B �̃R�s�[
			m_Log.push_back( VsdLogTmp );	// [+1]
			m_Log.push_back( VsdLogTmp );	// [+2]
			
			// �X�s�[�h�� 0 ��
			m_Log[ m_iCnt ].SetSpeed( 0 );
			m_Log[ m_iCnt ].SetGx( 0 );
			m_Log[ m_iCnt ].SetGy( 0 );
			m_Log[ m_iCnt + 1 ].SetSpeed( 0 );
			m_Log[ m_iCnt + 1 ].SetGx( 0 );
			m_Log[ m_iCnt + 1 ].SetGy( 0 );
			
			// ���Ԓ���
			m_Log[ m_iCnt     ].SetTime( Time( m_iCnt )     + 0.5 ); // �K���� 0.5�b
			m_Log[ m_iCnt + 1 ].SetTime( Time( m_iCnt + 1 ) - 0.5 );
			
			m_iCnt += 2;
		}
	}
	
	m_iCnt++;
}

int CVsdLog::ReadGPSLog( const char *szFileName ){
	TCHAR	szCurDir[ MAX_PATH ];
	TCHAR	szBuf[ BUF_SIZE ];
	
	double	dLati;
	double	dLong;
	double	dSpeed;
	double	dTime;
	gzFile	fp;
	
	UINT	u;
	
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
				
				VSD_LOG_t	VsdLogTmp;
				
				// �l�␳
				// 2254460 �� 22:54:46.0
				u = BigEndianI( 0 ) & 0x3FFFFF;
				VsdLogTmp.SetTime(
					u / 100000 * 3600 +
					u / 1000 % 100 * 60 +
					( u % 1000 ) / 10.0
				);
				
				// ���x
				VsdLogTmp.SetSpeed( BigEndianS( 12 ) / 10.0 );
				
				PushRecord(
					VsdLogTmp,
					BigEndianI( 4 ) / 460800.0,
					BigEndianI( 8 ) / 460800.0
				);
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
				
				VSD_LOG_t	VsdLogTmp;
				
				VsdLogTmp.SetTime( dTime + m_iCnt / 5 );	// 5Hz �Œ�炵��
				
				// ���x
				VsdLogTmp.SetSpeed( *( short int *)( szBuf + 0x4 ) / 10.0 );
				
				PushRecord(
					VsdLogTmp,
					*( short int *)( szBuf + 0x0 ) / 460800.0 + dLongBase,
					*( short int *)( szBuf + 0x2 ) / 460800.0 + dLatiBase
				);
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
			
			VSD_LOG_t	VsdLogTmp;
			
			VsdLogTmp.SetTime( dTime );
			
			// ���x
			if( uParamCnt >= 6 ) VsdLogTmp.SetSpeed( dSpeed * 1.852 ); // knot/h �� km/h
			
			PushRecord( VsdLogTmp, dLong, dLati );
		}
		
		gzclose( fp );
	}
	
	chdir( szCurDir );	// pwd �����ɖ߂�
	
	/************************************************************************/
	
	//#define DUMP_LOG
	#if defined DEBUG && defined DUMP_LOG
		Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_raw.txt" );
	#endif
	
	// �A�b�v�R���o�[�g�p�o�b�t�@�m�ہE������
	GPSLogUpConvert();
	
	#if defined DEBUG && defined DUMP_LOG
		Dump( "D:\\DDS\\vsd\\vsd_filter\\z_gpslog_upcon.txt" );
	#endif
	
	m_dLogStartTime += 9 * 3600;
	
	return m_iCnt;
}

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
	
	while( gzgets( fp, szBuf, BUF_SIZE ) != Z_NULL ){
		VSD_LOG_t	VsdLogTmp;
		
		if(( p = strstr( szBuf, "LAP" )) != NULL ){ // ���b�v�^�C���L�^��������
			
			if( pLapLog == NULL ){
				pLapLog = new CLapLog();
				pLapLog->m_iLapMode = LAPMODE_MAGNET;
			}
			
			UINT	uLap, uMin, uSec, uMSec;
			
			uReadCnt = sscanf( p, "LAP%d%d:%d.%d", &uLap, &uMin, &uSec, &uMSec );
			
			int iTime = ( uMin * 60 + uSec ) * 1000 + uMSec;
			
			LapTime.uLap	= uLap;
			LapTime.fLogNum	= ( float )m_iCnt;
			LapTime.iTime	= ( uReadCnt == 4 ) ? iTime : 0;
			pLapLog->m_Lap.push_back( LapTime );
			
			if(
				uReadCnt == 4 &&
				( pLapLog->m_iBestTime == TIME_NONE || pLapLog->m_iBestTime > iTime )
			){
				pLapLog->m_iBestTime	= iTime;
				pLapLog->m_iBestLap	= pLapLog->m_iLapNum - 1;
				
				iLogFreqLog	 += m_iCnt - ( int )pLapLog->m_Lap[ pLapLog->m_iLapNum - 1 ].fLogNum;
				iLogFreqTime += iTime;
			}
			++pLapLog->m_iLapNum;
		}
		
		double	dLati = 0;
		double	dLong = 0;
		if(( p = strstr( szBuf, "GPS" )) != NULL ){ // GPS�L�^��������
			sscanf( p, "GPS%lg%lg", &dLati, &dLong );
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
			
			VsdLogTmp.SetTacho( iTacho );
			VsdLogTmp.SetSpeed( dSpeed );
			VsdLogTmp.SetDistance( dDistance );
			
			if( m_iMaxTacho < iTacho ) m_iMaxTacho = iTacho;
			
			if( uReadCnt < 5 && m_iCnt ){
				// G�f�[�^���Ȃ��Ƃ��́Cspeed���狁�߂適�p�~
				dGx = 0;
				dGy = 0;
				//dGy = ( VsdLogTmp.fSpeed - m_Log[ m_iCnt - 1 ].fSpeed ) * ( 1000.0 / 3600 / 9.8 * LOG_FREQ );
			}else{
				if( dGx >= 4 ){	
					// �P�ʂ� G �ɕϊ�
					dGx = -dGx / ACC_1G_Y;
					dGy =  dGy / ACC_1G_Z;
				}
				
				if( m_iCnt == 0 ){
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
					m_iCnt &&
					Distance( m_iCnt - 1 ) == VsdLogTmp.Distance() &&
					( Gy( m_iCnt - 1 ) - dGy ) >= -0.02 &&
					( Gy( m_iCnt - 1 ) - dGy ) <=  0.02
				){
					dGcx += dGx / 160;
					dGcy += dGy / 160;
				}
			}
			
			VsdLogTmp.SetGx( dGx );
			VsdLogTmp.SetGy( dGy );
			
			if( m_dMaxGx < fabs( dGx )) m_dMaxGx = fabs( dGx );
			if( m_dMaxGy < -dGy       ) m_dMaxGy = -dGy;
			
			// ���O�J�n�E�I���F��
			if( VsdLogTmp.Speed() >= 300 ){
				if( !bCalibrating ){
					bCalibrating = TRUE;
					m_iLogStart  = m_iLogStop;
					m_iLogStop   = m_iCnt;
				}
			}else{
				bCalibrating = FALSE;
				
				if( m_iMaxSpeed < VsdLogTmp.Speed())
					m_iMaxSpeed = ( int )ceil( VsdLogTmp.Speed());
			}
			
			VsdLogTmp.SetTime(( double )m_iCnt / LOG_FREQ );
			PushRecord( VsdLogTmp, dLong, dLati );
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
	
	// Vsd log �̔Ԍ�
	m_Log.push_back( m_Log[ m_iCnt - 1 ] );
	m_Log[ m_iCnt ].SetTime( FLT_MAX );
	
	return m_iCnt;
}

/*** MAP ��]���� ***********************************************************/

void CVsdLog::RotateMap( double dAngle ){
	
	int	i;
	double dMaxX, dMinX, dMaxY, dMinY;
	
	dMaxX = dMinX = dMaxY = dMinY = 0;
	
	for( i = 0; i < m_iCnt; ++i ){
		if( _isnan( X0( i ))){
			m_Log[ i ].SetX( X0( i ));
			m_Log[ i ].SetY( Y0( i ));
		}else{
			m_Log[ i ].SetX(  cos( dAngle ) * X0( i ) + sin( dAngle ) * Y0( i ));
			m_Log[ i ].SetY( -sin( dAngle ) * X0( i ) + cos( dAngle ) * Y0( i ));
			
			if     ( dMaxX < X( i )) dMaxX = X( i );
			else if( dMinX > X( i )) dMinX = X( i );
			if     ( dMaxY < Y( i )) dMaxY = Y( i );
			else if( dMinY > Y( i )) dMinY = Y( i );
		}
	}
	
	m_dMapSizeX	= dMaxX - dMinX;
	m_dMapSizeY	= dMaxY - dMinY;
	m_dMapOffsX	= dMinX;
	m_dMapOffsY	= dMinY;
}
