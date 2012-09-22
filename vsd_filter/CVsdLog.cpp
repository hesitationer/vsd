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
	if( m_iCnt <= iTo ) m_iCnt = iTo + 1;
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
	
	/*** VSD_LOG_t �̕��𑖍����āC���낢��␳ *****************************/
	
	m_dFreq = 0;
	int		iFreqCnt = 0;
	
	#ifdef _OPENMP
		#pragma omp parallel
	#endif
	{
		if( m_pLogX0 != 0 && m_pLogY0 != 0 ){
			// �X�s�[�h����
			if( !m_pLogSpeed ){
				#ifdef _OPENMP
					#pragma omp for
				#endif
				for( int i = 0; i < GetCnt() - 1; ++i ){
					if( Time( i + 1 ) - Time( i ) >= ( TIME_STOP - TIME_STOP_MARGIN * 2 )){
						// ���Ԃ��J���Ă����~���O
						SetSpeed( i++, 0 );
						SetSpeed( i,   0 );
					}else{
						SetSpeed( i,
							( Distance( i + 1 ) - Distance( i ))
							* ( 3600.0 / 1000 ) /
							( Time( i + 1 ) - Time( i ))
						);
					}
				}
				// �Ԍ��͂��ł� 0 �� SetSpeed( GetCnt() - 1, 0 );
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
	{
		// JavaScript �I�u�W�F�N�g������
		CScript Script;
		Script.Initialize();
		if( Script.RunFile( L"log_reader\\nmea.js" ) != ERR_OK ){
			return 0;	// �G���[
		}
		
		// �X�N���v�g���s
		LPWSTR pStr = NULL;
		Script.Run_s( L"ReadLog", StringNew( pStr, szFileName ));
		delete [] pStr;
		
		/*** JS �� Log �ɃA�N�Z�X *******************************************/
		
		{
			#ifdef AVS_PLUGIN
				v8::Isolate::Scope IsolateScope( Script.m_pIsolate );
			#endif
			v8::HandleScope handle_scope;
			v8::Context::Scope context_scope( Script.m_Context );
			
			// "Log" �擾
			v8::Local<v8::Array> hLog = v8::Local<v8::Array>::Cast(
				Script.m_Context->Global()->Get( v8::String::New(( uint16_t *)L"Log" ))
			);
			if( !hLog->IsArray()) return 0;
			
			// Log �� key �擾
			v8::Local<v8::Array> Keys = hLog->GetPropertyNames();
			
			// JavaScript Array �� vector
			std::vector<v8::Local<v8::Array> >	JSArrays;
			
			// VSD_LOG_t * �� vector
			std::vector<VSD_LOG_t *>	CArrays;
			
			UINT	uIdxTime	= ~0;
			UINT	uIdxDistance= ~0;
			UINT	uIdxX0		= ~0;
			UINT	uIdxY0		= ~0;
			
			
			for( UINT u = 0; u < Keys->Length(); ++u ){
				v8::String::AsciiValue strKey( Keys->Get( u ));
				char *pKey = *strKey;
				
				v8::Local<v8::Array> ArrayTmp = v8::Local<v8::Array>::Cast( hLog->Get( Keys->Get( u )));
				if( ArrayTmp->IsArray()){
					if     ( !strcmp( *strKey, "Time"      )) uIdxTime		= u;
					else if( !strcmp( *strKey, "Distance"  )) uIdxDistance	= u;
					else if( !strcmp( *strKey, "Longitude" )){ uIdxX0		= u; pKey = "X0"; }
					else if( !strcmp( *strKey, "Latitude"  )){ uIdxY0		= u; pKey = "Y0"; }
					
					// strKey �� vector �쐬
					CArrays.push_back( GetElement( pKey, TRUE ));
					
					// JS �� property ��������� Array �� vector �����
					JSArrays.push_back( ArrayTmp );
				}
			}
			
			// Distance �������Ƃ��́C���
			BOOL bExistDistance = TRUE;
			if( uIdxDistance == ~0 ){
				uIdxDistance = CArrays.size();
				CArrays.push_back( GetElement( "Distance", TRUE ));
				bExistDistance = FALSE;
			}
			
			// Time ���݊m�F
			if( uIdxTime == ~0 ) return 0;
			m_dLogStartTime = JSArrays[ uIdxTime ]->Get( 0 )->NumberValue() / 1000.0;
			
			// �ܓx�o�x�����[�g�� �ϊ��萔
			double dLong2Meter = 0;
			double dLati2Meter = 0;
			
			if( uIdxX0 != ~0 && uIdxY0 != ~0 ){
				m_dLong0 = JSArrays[ uIdxX0 ]->Get( 0 )->NumberValue();
				m_dLati0 = JSArrays[ uIdxY0 ]->Get( 0 )->NumberValue();
				
				dLong2Meter = GPSLogGetLength( m_dLong0, m_dLati0, m_dLong0 + 1.0 / 3600, m_dLati0 ) * 3600;
				dLati2Meter = GPSLogGetLength( m_dLong0, m_dLati0, m_dLong0, m_dLati0 + 1.0 / 3600 ) * 3600;
			}
			
			// vector �ɐς�
			double	dDistance = 0;
			int		iSameCnt = 0;
			
			for( UINT uIdx = 0; uIdx < JSArrays[ uIdxTime ]->Length(); ++uIdx ){
				int iCnt = GetCnt();
				
				for( UINT uKey = 0; uKey < Keys->Length(); ++uKey ){
					
					double dVal = JSArrays[ uKey ]->Get( uIdx )->NumberValue();
					
					if( uKey == uIdxTime ){
						if( dVal < m_dLogStartTime ) dVal += 24 * 3600;
						SetTime( iCnt, dVal / 1000.0 - m_dLogStartTime );
					}else if( uKey == uIdxX0 ){
						SetX0( iCnt, ( dVal - m_dLong0 ) * dLong2Meter );
					}else if( uKey == uIdxY0 ){
						SetY0( iCnt, ( m_dLati0 - dVal ) * dLati2Meter );
					}else{
						CArrays[ uKey ]->Set( iCnt, dVal );
					}
				}
				
				if( iCnt == 0 ){
					// �Ԍ��쐬
					SetDistance( 0, 0 );
					CopyRecord( 1, 0 );
					CopyRecord( 2, 0 );
					AddStopRecord( 0, -WATCHDOG_TIME );
					AddStopRecord( 1, -TIME_STOP_MARGIN );
				}else{
					// �������Ԃ��A������ꍇ�̎�������
					if( Time( iCnt - 1 ) == Time( iCnt )){
						// �������������O�������Ƃ����̃J�E���g������
						++iSameCnt;
					}else if( iSameCnt ){
						// �������������O���r�؂ꂽ�̂ŁC���Ԃ�␳����
						++iSameCnt;
						
						// -4 -3 -2 -1 +0
						//  A  A  A  A  B
						//u =  1  2   3
						
						for( int j = 1; j < iSameCnt; ++j ){
							SetTime(
								iCnt - iSameCnt + j,
								Time( iCnt - iSameCnt ) + 
								( Time( iCnt ) - Time( iCnt - iSameCnt )) / iSameCnt * j
							);
						}
						iSameCnt = 0;
					}
					
					// ���s���������
					if( !bExistDistance ){
						double x = X0( iCnt - 1 ) - X0( iCnt ); x *= x;
						double y = Y0( iCnt - 1 ) - Y0( iCnt ); y *= y;
						
						dDistance += sqrt( x + y );
						SetDistance( iCnt, dDistance );
					}
					
					// �O�̃��O���� TIME_STOP ����Ă��Ă��� �w��km/h �ȉ��Ȃ�C��~�Ƃ݂Ȃ�
					double dDiffTime = Time( iCnt ) - Time( iCnt - 1 );
					if(
						dDiffTime >= TIME_STOP &&
						Distance( iCnt ) / dDiffTime < ( 5.0 /*[km/h]*/ * 1000 / 3600 )
					){
						// -1 -0
						// A  B
						//     ��
						// -1 -0 +1 +2
						// A  A' B' B
						CopyRecord( iCnt + 1, iCnt     ); // B'
						CopyRecord( iCnt + 2, iCnt     ); // B
						CopyRecord( iCnt,     iCnt - 1 ); // A'
						AddStopRecord( iCnt,     Time( iCnt - 1 ) + TIME_STOP_MARGIN ); // A'
						AddStopRecord( iCnt + 1, Time( iCnt + 2 ) - TIME_STOP_MARGIN ); // B'
					}
				}
			}
		}
	}
	
	/************************************************************************/
	
	if( GetCnt()){
		int iCnt = GetCnt();
		
		// �I�[���̔Ԍ�
		CopyRecord( iCnt,     iCnt - 1 );
		CopyRecord( iCnt + 1, iCnt - 1 );
		AddStopRecord( iCnt,     Time( iCnt - 1 ) + TIME_STOP_MARGIN );
		AddStopRecord( iCnt + 1, WATCHDOG_TIME );
		
		
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
