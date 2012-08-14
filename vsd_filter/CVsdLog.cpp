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
#define GPS_LOG_OFFS	15

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
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdLog::~CVsdLog(){
	m_iCnt	= 0;
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogUpConvert( std::vector<GPS_LOG_t>& GPSLog, BOOL bAllParam ){
	
	UINT	u;
	
	m_iCnt = 0;
	
	UINT uCnt = GPSLog.size();
	if( uCnt < 2 ) return 0;			// 2�f�[�^���Ȃ���ΏI��
	
	GPS_LOG_t	GpsLogTmp;
	GpsLogTmp.SetTime( FLT_MAX );
	GPSLog.push_back( GpsLogTmp );		// �Ԍ�
	
	double	t;
	double	dMileage = 0;
	double	dBearing;
	
	/*
	VSD_LOG_t	LogTmp;
	
	LogTmp.SetSpeed( 0 );
	LogTmp.SetTacho( 0 );
	LogTmp.SetGx( 0 );
	LogTmp.SetGy( 0 );
	LogTmp.SetMileage( 0 );
	
	m_Log.push_back( LogTmp );
	*/
	
	/*** GPS_LOG_t �̕��𑖍����āC���낢��␳ *****************************/
	
	for( u = 1; u < uCnt - 1; ++u ){
		// speed ���Ȃ��ꍇ�̕␳
		if( !GPSLog[ u ].IsValidSpeed()){
			GPSLog[ u ].SetSpeed(
				sqrt(
					pow( GPSLog[ u + 1 ].X() - GPSLog[ u - 1 ].X(), 2 ) +
					pow( GPSLog[ u + 1 ].Y() - GPSLog[ u - 1 ].Y(), 2 )
				) / ( GPSLog[ u + 1 ].Time() - GPSLog[ u - 1 ].Time())
				* ( 3600.0 / 1000 )
			);
		}
		
		// bearing ���Ȃ��ꍇ�̕␳
		//if( GPSLog[ u ].Bearing() == FLT_MAX ){
		if( 1 ){	// GPS �f�[�^�� bearing �͖���
			GPSLog[ u ].SetBearing(
				fmod(
					atan2(
						GPSLog[ u + 1 ].Y() - GPSLog[ u - 1 ].Y(),
						GPSLog[ u + 1 ].X() - GPSLog[ u - 1 ].X()
					) / ToRAD + 360 * 2 + 90,
					360
				)
			);
		}
	}
	
	GPSLog[ 0 ].SetSpeed(			GPSLog[ 1 ].Speed());
	GPSLog[ 0 ].SetBearing(			GPSLog[ 1 ].Bearing());
	GPSLog[ uCnt - 1 ].SetSpeed(	GPSLog[ uCnt - 2 ].Speed());
	GPSLog[ uCnt - 1 ].SetBearing(	GPSLog[ uCnt - 2 ].Bearing());
	
	for( u = 1; u < uCnt - 1; ++u ){
		// Gx / Gy �����
		GPSLog[ u ].SetGy(
			( GPSLog[ u + 1 ].Speed() - GPSLog[ u - 1 ].Speed())
			* ( 1 / 3.600 / GRAVITY )
			/ ( GPSLog[ u + 1 ].Time() - GPSLog[ u - 1 ].Time())
		);
		
		// ��G = v��
		dBearing = GPSLog[ u + 1 ].Bearing() - GPSLog[ u - 1 ].Bearing();
		if     ( dBearing >  180 ) dBearing -= 360;
		else if( dBearing < -180 ) dBearing += 360;
		
		GPSLog[ u ].SetGx(
			dBearing * ( ToRAD / GRAVITY )
			/ ( GPSLog[ u + 1 ].Time() - GPSLog[ u - 1 ].Time())
			* ( GPSLog[ u ].Speed() / 3.600 )
		);
		
		// �}5G �ȏ�́C�폜
		if( GPSLog[ u ].Gx() < -3 || GPSLog[ u ].Gx() > 3 ){
			GPSLog[ u ].SetGx( GPSLog[ u - 1 ].Gx());
		}
	}
	
	GPSLog[ 0 ].SetGx(			GPSLog[ 1 ].Gx());
	GPSLog[ 0 ].SetGy(			GPSLog[ 1 ].Gy());
	GPSLog[ uCnt - 1 ].SetGx(	GPSLog[ uCnt - 2 ].Gx());
	GPSLog[ uCnt - 1 ].SetGy(	GPSLog[ uCnt - 2 ].Gy());
	
	/************************************************************************/
	
	for( m_iCnt = 0, u = 0; ; ++m_iCnt ){
		
		// GPSLog[ u ].Time() <= m_iCnt / LOG_FREQ < GPSLog[ u + 1 ].Time()
		// �͈̔͂ɂȂ�悤����
		
		// m_iCnt ��������������Ă���̂ŃC���N��
		if(( float )m_iCnt / LOG_FREQ < GPSLog[ u ].Time()){
			for( ; ( float )m_iCnt / LOG_FREQ < GPSLog[ u ].Time(); ++m_iCnt );
		}
		
		// m_iCnt ������������Ă���̂ŁCu ���C���N��
		if(( float )m_iCnt / LOG_FREQ >= GPSLog[ u + 1 ].Time()){
			for( ; ( float )m_iCnt / LOG_FREQ >= GPSLog[ u + 1 ].Time(); ++u );
			
			// GPS ���O�͈͂𒴂����̂� return
			if( u > uCnt - 2 ) break;
		}
		
		// 5�b�ȏ� GPS ���O�������Ă���΁C�⊮���̌v�Z�����Ȃ�
		//if( GPSLog[ u + 1 ].Time() - GPSLog[ u ].Time() > 5 ) continue;
		
		t = (( double )m_iCnt / LOG_FREQ - GPSLog[ u ].Time())
			/ ( GPSLog[ u + 1 ].Time() - GPSLog[ u ].Time());
		
		#define GetLogIntermediateVal( p )\
			( GPSLog[ u ].p() * ( 1 - t ) + GPSLog[ u + 1 ].p() * t )
		
		VSD_LOG_t	LogTmp;
		
		LogTmp.SetX0( GetLogIntermediateVal( X ));
		LogTmp.SetY0( GetLogIntermediateVal( Y ));
		
		if( bAllParam ){
			LogTmp.SetSpeed( GetLogIntermediateVal( Speed ));
			
			if( m_iMaxSpeed < LogTmp.Speed())
				m_iMaxSpeed = ( int )ceil( LogTmp.Speed());
			
			if( m_iCnt ){
				dMileage += sqrt(
					pow( m_Log[ m_iCnt - 1 ].X0() - LogTmp.X0(), 2 ) +
					pow( m_Log[ m_iCnt - 1 ].Y0() - LogTmp.Y0(), 2 )
				);
			}
			
			LogTmp.SetMileage( dMileage );
			LogTmp.SetTacho( 0 );
			
			LogTmp.SetGx( GetLogIntermediateVal( Gx ));
			LogTmp.SetGy( GetLogIntermediateVal( Gy ));
		}else{
			// PSP GPS log �̂Ƃ��́CG �� MAX �l�݂̂��`�F�b�N
			if( m_dMaxGy < LogTmp.Gy()) m_dMaxGy = LogTmp.Gy();
			if( m_dMinGy > LogTmp.Gy()) m_dMinGy = LogTmp.Gy();
		}
		
		m_Log.push_back( LogTmp );
	}
	
	// �X���[�W���O
	if( bAllParam ){
		UINT	v = 2;
		double	dGx0, dGx1 = 0;
		double	dGy0, dGy1 = 0;
		
		while( v-- ) for( u = 2; u < ( UINT )m_iCnt - 2; ++u ){
			m_Log[ u ].SetGx( dGx0 = (
				m_Log[ u - 2 ].Gx() +
				m_Log[ u - 1 ].Gx() +
				m_Log[ u + 0 ].Gx() +
				m_Log[ u + 1 ].Gx() +
				m_Log[ u + 2 ].Gx()
			) / 5 );
			m_Log[ u ].SetGy( dGy0 = (
				m_Log[ u - 2 ].Gy() +
				m_Log[ u - 1 ].Gy() +
				m_Log[ u + 0 ].Gy() +
				m_Log[ u + 1 ].Gy() +
				m_Log[ u + 2 ].Gy()
			) / 5 );
			
			dGx1 = dGx1 * 0.9 + dGx0 * 0.1;
			dGy1 = dGy1 * 0.9 + dGy0 * 0.1;
			if( m_dMaxGx < fabs( dGx1 )) m_dMaxGx = fabs( dGx1 );
			if( m_dMaxGy < dGy1 ) m_dMaxGy = dGy1;
			if( m_dMinGy > dGy1 ) m_dMinGy = dGy1;
		}
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

int CVsdLog::ReadGPSLog( const char *szFileName ){
	
	UINT	uGPSCnt = 0;
	TCHAR	szCurDir[ MAX_PATH ];
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
	
	std::vector<GPS_LOG_t> GPSLog;
	
	getcwd( szCurDir, MAX_PATH );	// �J�����g dir
	
	// �}���`�t�@�C���̏ꍇ�C1�ڂ� dir �߂��Ȃ̂ł����� cd
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
				
				GPS_LOG_t	GpsLogTmp;
				
				// �P�ʂ�␳
				// �ܓx�E�o�x�����[�g��
				GpsLogTmp.SetX(( dLong - dLong0 ) * dLong2Meter );
				GpsLogTmp.SetY(( dLati0 - dLati ) * dLati2Meter );
				
				// ���x�E�������x�N�g�����W
				GpsLogTmp.SetSpeed( BigEndianS( 12 ) / 10.0 );
				//GpsLogTmp.SetBearing( BigEndianS( 14 ));
				GpsLogTmp.SetTime( dTime );
				
				GPSLog.push_back( GpsLogTmp );
				
				if( uGPSCnt >=2 ){
					if( GPSLog[ uGPSCnt - 1 ].Time() == GpsLogTmp.Time()){
						// �������������O�������Ƃ����̃J�E���g������
						++uSameCnt;
					}else if( uSameCnt ){
						// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
						++uSameCnt;
						
						for( u = 1; u < uSameCnt; ++ u ){
							GPSLog[ uGPSCnt - uSameCnt + u ].SetTime(
								( GpsLogTmp.Time() - GPSLog[ uGPSCnt - uSameCnt ].Time())
								/ uSameCnt * u
							);
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
				
				GPS_LOG_t	GpsLogTmp;
				
				// �P�ʂ�␳
				// �ܓx�E�o�x�����[�g��
				GpsLogTmp.SetX(( dLong - dLong0 ) * dLong2Meter );
				GpsLogTmp.SetY(( dLati0 - dLati ) * dLati2Meter );
				
				// ���x�E�������x�N�g�����W
				GpsLogTmp.SetSpeed( *( short int *)( szBuf + 0x4 ) / 10.0 );
				
				//GpsLogTmp.SetBearing( FLT_MAX );
				GpsLogTmp.SetTime( dTime - dTime0 );
				
				GPSLog.push_back( GpsLogTmp );
				
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
			
			
			GPS_LOG_t	GpsLogTmp;
			// �P�ʂ�␳
			// �ܓx�E�o�x�����[�g��
			GpsLogTmp.SetX(( dLong - dLong0 ) * dLong2Meter );
			GpsLogTmp.SetY(( dLati0 - dLati ) * dLati2Meter );
			
			// ���x�E�������x�N�g�����W
			if( uParamCnt >= 6 ) GpsLogTmp.SetSpeed( dSpeed * 1.852 ); // knot/h �� km/h
			//GpsLogTmp.SetBearing( uParamCnt < 7 ? FLT_MAX : dBearing );
			GpsLogTmp.SetTime( dTime );
			
			GPSLog.push_back( GpsLogTmp );
			
			if( uGPSCnt >=2 ){
				if( GPSLog[ uGPSCnt - 1 ].Time() == GpsLogTmp.Time()){
					// �������������O�������Ƃ����̃J�E���g������
					++uSameCnt;
				}else if( uSameCnt ){
					// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
					++uSameCnt;
					
					for( u = 1; u < uSameCnt; ++ u ){
						GPSLog[ uGPSCnt - uSameCnt + u ].SetTime(
							GPSLog[ uGPSCnt - uSameCnt + u ].Time() +
							( GpsLogTmp.Time() - GPSLog[ uGPSCnt - uSameCnt ].Time())
							/ uSameCnt * u
						);
					}
					uSameCnt = 0;
				}
			}
			
			// 20km/h �ȉ��� 3�b�ȏ� log �̊Ԋu���J���Ƃ��C0km/h �� log ��⊮����
			if(
				uGPSCnt >= 2 &&
				GpsLogTmp.Time() - GPSLog[ uGPSCnt - 1 ].Time() >= 3 &&
				GpsLogTmp.Speed() <= 20
			){
				// -1 +0 +1 +2
				// A  B
				//  ��
				// A  A' B' B
				
				// �f�[�^ B �̃R�s�[
				GPSLog.push_back( GpsLogTmp );	// +1
				GPSLog.push_back( GpsLogTmp );	// +2
				
				// �f�[�^ A->A' �̃R�s�[
				GPSLog[ uGPSCnt ] = GPSLog[ uGPSCnt - 1 ];
				
				// �X�s�[�h�� 0 ��
				GPSLog[ uGPSCnt ].SetSpeed( 0 );
				GPSLog[ uGPSCnt + 1 ].SetSpeed( 0 );
				
				// ���Ԓ���
				double dDiff = GPSLog[ uGPSCnt - 1 ].Time() - GPSLog[ uGPSCnt - 2 ].Time();
				GPSLog[ uGPSCnt     ].SetTime( GPSLog[ uGPSCnt     ].Time() + dDiff );
				GPSLog[ uGPSCnt + 1 ].SetTime( GPSLog[ uGPSCnt + 1 ].Time() + dDiff );
				
				uGPSCnt += 2;
			}
			
			uGPSCnt++;
		}
		
		gzclose( fp );
	}
	
	chdir( szCurDir );	// pwd �����ɖ߂�
	
	/************************************************************************/
	
	// �A�b�v�R���o�[�g�p�o�b�t�@�m�ہE������
	GPSLogUpConvert( GPSLog, TRUE );
	
#if 0
	DebugCmd( {
		FILE *fpp = fopen( "D:\\DDS\\vsd\\vsd_filter\\z_gps_raw.txt", "w" );
		for( u = 0; u < uGPSCnt; ++u ){
			fprintf( fpp, "%g\t%g\t%g\t%g\t%g\t%g\n",
				GPSLog[ u ].X(),
				GPSLog[ u ].Y(),
				GPSLog[ u ].Speed(),
				//GPSLog[ u ].Bearing(),
				GPSLog[ u ].Gx(),
				GPSLog[ u ].Gy(),
				GPSLog[ u ].Time()
			);
		}
		fclose( fpp );
	} )
	
	DebugCmd( {
		FILE *fpp = fopen( "D:\\DDS\\vsd\\vsd_filter\\z_upcon_gps.txt", "w" );
		for( u = 0; u < ( UINT )m_iCnt; ++u ){
			fprintf( fpp, "%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\n",
				m_Log[ u ].Speed(),
				m_Log[ u ].Tacho(),
				m_Log[ u ].Mileage(),
				m_Log[ u ].X(),
				m_Log[ u ].Y(),
				m_Log[ u ].X0(),
				m_Log[ u ].Y0(),
				m_Log[ u ].Gx(),
				m_Log[ u ].Gy()
			);
		}
		fclose( fpp );
	} )
#endif
	
	if( m_Log.size()) m_dLogStartTime = dTime0 + 9 * 3600;
	
	return m_iCnt;
}

/*** ���O���[�h *************************************************************/

int CVsdLog::ReadLog( const char *szFileName, CLapLog *&pLapLog ){
	
	TCHAR	szBuf[ BUF_SIZE ];
	gzFile	fp;
	BOOL	bCalibrating = FALSE;
	
	if(( fp = gzopen(( char *)szFileName, "rb" )) == NULL ) return 0;
	
	// GPS ���O�p
	UINT		uGPSCnt = 0;
	std::vector<GPS_LOG_t> GPSLog;
	
	pLapLog			= NULL;
	
	// ���O���[�h
	
	UINT	uReadCnt, uLap, uMin, uSec, uMSec;
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
	
	LAP_t	LapTime;
	
	while( gzgets( fp, szBuf, BUF_SIZE ) != Z_NULL ){
		if(( p = strstr( szBuf, "LAP" )) != NULL ){ // ���b�v�^�C���L�^��������
			
			if( pLapLog == NULL ){
				pLapLog = new CLapLog();
				pLapLog->m_iLapMode = LAPMODE_MAGNET;
			}
			
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
			GPS_LOG_t	GpsLogTmp;
			
			GpsLogTmp.SetX(( dLong - dLong0 ) * dLong2Meter );
			GpsLogTmp.SetY(( dLati0 - dLati ) * dLati2Meter );
			GpsLogTmp.SetSpeed( dSpeed );
			//GpsLogTmp.SetBearing( dBearing );
			
			// LOG_FREQ �� 15Hz ����������̃��O�ł͊ԈႢ�����C���䕪���������Q�Ȃ�?
			GpsLogTmp.SetTime((( int )m_iCnt - GPS_LOG_OFFS ) / ( double )LOG_FREQ );
			
			GPSLog.push_back( GpsLogTmp );
			
			++uGPSCnt;
		}
		
		int	iTacho;
		double dSpeed;
		double dMileage;
		
		// ���ʂ� log
		if(( uReadCnt = sscanf( szBuf, "%u%lg%lg%lg%lg",
			&iTacho,
			&dSpeed,
			&dMileage,
			&dGy,
			&dGx
		)) >= 3 ){
			VSD_LOG_t	VsdLogTmp;
			
			VsdLogTmp.SetTacho( iTacho );
			VsdLogTmp.SetSpeed( dSpeed );
			VsdLogTmp.SetMileage( dMileage );
			
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
					m_Log[ m_iCnt - 1 ].Mileage() == VsdLogTmp.Mileage() &&
					( m_Log[ m_iCnt - 1 ].Gy() - dGy ) >= -0.02 &&
					( m_Log[ m_iCnt - 1 ].Gy() - dGy ) <=  0.02
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
			
			// ���[�^�[�␳
			/*
			VsdLogTmp.SetTacho(
				VsdLogTmp.Tacho() * m_piParamS[ METER_ADJUST ] / 1000.0
			);
			VsdLogTmp.SetSpeed(
				VsdLogTmp.Speed() * m_piParamS[ METER_ADJUST ] / 1000.0
			);
			*/
			
			m_Log.push_back( VsdLogTmp );
			
			++m_iCnt;
		}
	}
	
	// �Â� log �� LOG_FREQ �Ƃ͌���Ȃ��̂ŁC�v�Z�ŋ��߂�
	if( iLogFreqTime ){
		m_dFreq = iLogFreqLog / ( iLogFreqTime / 1000.0 );
	}
	
	/*** GPS ���O����O�Ղ����߂� *******************************************/
	
	if( uGPSCnt ) GPSLogUpConvert( GPSLog );
	
	/************************************************************************/
	
	gzclose( fp );
	
	if( pLapLog ){
		LapTime.fLogNum	= FLT_MAX;	// �Ԍ�
		LapTime.iTime	= 0;		// �Ԍ�
		pLapLog->m_Lap.push_back( LapTime );
	}
	
	return m_iCnt;
}

/*** MAP ��]���� ***********************************************************/

void CVsdLog::RotateMap( double dAngle ){
	
	int	i;
	double dMaxX, dMinX, dMaxY, dMinY;
	
	dMaxX = dMinX = dMaxY = dMinY = 0;
	
	for( i = 0; i < m_iCnt; ++i ){
		if( _isnan( m_Log[ i ].X0())){
			m_Log[ i ].SetX( m_Log[ i ].X0());
			m_Log[ i ].SetY( m_Log[ i ].Y0());
		}else{
			m_Log[ i ].SetX(  cos( dAngle ) * m_Log[ i ].X0() + sin( dAngle ) * m_Log[ i ].Y0());
			m_Log[ i ].SetY( -sin( dAngle ) * m_Log[ i ].X0() + cos( dAngle ) * m_Log[ i ].Y0());
			
			if     ( dMaxX < m_Log[ i ].X()) dMaxX = m_Log[ i ].X();
			else if( dMinX > m_Log[ i ].X()) dMinX = m_Log[ i ].X();
			if     ( dMaxY < m_Log[ i ].Y()) dMaxY = m_Log[ i ].Y();
			else if( dMinY > m_Log[ i ].Y()) dMinY = m_Log[ i ].Y();
		}
	}
	
	m_dMapSizeX	= dMaxX - dMinX;
	m_dMapSizeY	= dMaxY - dMinY;
	m_dMapOffsX	= dMinX;
	m_dMapOffsY	= dMinY;
}
