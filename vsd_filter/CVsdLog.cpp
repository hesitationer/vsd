/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdLog.cpp - CVsdLog class implementation
	
*****************************************************************************/

#include "StdAfx.h"
#include "CVsdFilter.h"

/*** macros *****************************************************************/

#define GRAVITY				9.80665
#define CALIB_MARK_SPEED	300
#define MAX_TURN_R			200

/*** �R���X�g���N�^ *********************************************************/

CVsdLog::CVsdLog( CVsdFilter *pVsd ){
	m_dFreq	= LOG_FREQ;
	
	m_iLogStartTime	= -1;
	
	m_dLong2Meter = 0;
	m_dLati2Meter = 0;
	m_iCnt		= 0;
	m_pVsd		= pVsd;
	
	#define DEF_LOG( name )	m_pLog##name = NULL;
	#include "def_log.h"
}

/*** frame# �ɑΉ����郍�O index �𓾂� *************************************/

double CVsdLog::GetIndex( double dFromVal, int *piFrom, int *piLog, int iPrevIdx ){
	if( piFrom[ 0 ] == piFrom[ 1 ] ) return 2;	// �Ԍ��������� Time=0 �̃��O
	return GetIndex(
		(
			piLog[ 0 ] +
			( piLog[ 1 ] - piLog[ 0 ] ) * ( dFromVal - piFrom[ 0 ] ) /
			( double )( piFrom[ 1 ] - piFrom[ 0 ] )
		) / SLIDER_TIME,
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
	std::map<std::string, CLog *>::iterator it;
	
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
			fprintf( fp, "%f", it->second->GetRaw( i ));
		}
		fputs( "\n", fp );
	}
	fclose( fp );
}
#endif

/*** key �̑��݊m�F *********************************************************/

CLog *CVsdLog::GetElement( const char *szKey, BOOL bCreate ){
	std::string strKey( szKey );
	std::map<std::string, CLog *>::iterator itr;
	
	if(( itr = m_Logs.find( strKey )) != m_Logs.end()){
		return itr->second;
	}
	if( bCreate ){
		CLog *p;
		
		// Speed �Ƃ���{�f�[�^�̎Q�Ɨp�|�C���^�ɑ��
		if( 0 );
		#define DEF_LOG( name )	DEF_LOG_T( name, CLogFloat )
		#define DEF_LOG_T( name, type )	\
			else if( strKey == #name ) p = m_pLog##name = new type();
		#include "def_log.h"
		else{
			p = new CLogFloat();
		}
		
		m_Logs[ strKey ] = p;
		return p;
	}
	return NULL;
}

/*** ���O�� Set *************************************************************/

void CVsdLog::Set( const char *szKey, int iIndex, double dVal ){
	GetElement( szKey, TRUE )->Set( iIndex, dVal );
	if( m_iCnt <= iIndex ) m_iCnt = iIndex + 1;
}

/*** 1���R�[�h�R�s�[ ********************************************************/

void CVsdLog::CopyRecord( int iTo, int iFrom ){
	std::map<std::string, CLog *>::iterator it;
	
	for( it = m_Logs.begin(); it != m_Logs.end(); ++it ){
		CLog *pLog = it->second;
		pLog->Set( iTo, pLog->GetRaw( iFrom ));
	}
	if( m_iCnt <= iTo ) m_iCnt = iTo + 1;
}

/*** �Ԍ��ǉ� ***************************************************************/

void CVsdLog::AddWatchDog( void ){
	int iCnt = GetCnt() - 1;
	CopyRecord( 0, 2 );           AddStopRecord( 0, -WATCHDOG_TIME );
	CopyRecord( 1, 2 );           AddStopRecord( 1, Time( 2 ) - 500 );
	CopyRecord( iCnt + 1, iCnt ); AddStopRecord( iCnt + 1, Time( iCnt ) + 500 );
	CopyRecord( iCnt + 2, iCnt ); AddStopRecord( iCnt + 2, WATCHDOG_TIME );
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogRescan( void ){
	
	/*** CLog �̕��𑖍����āC���낢��␳ *****************************/
	
	BOOL	bCreateSpeed	= FALSE;
	BOOL	bCreateG		= FALSE;
	BOOL	bCreateDir		= FALSE;
#ifdef USE_TURN_R
	BOOL	bCreateTurnR	= FALSE;
#endif
	
	if( m_pLogLongitude != NULL && m_pLogLatitude != NULL ){
		if( !m_pLogSpeed ){
			bCreateSpeed = TRUE;
			GetElement( "Speed", TRUE );
			m_pLogSpeed->Resize( GetCnt(), 0 );
		}
		
		if( !m_pLogGx ){
			bCreateG = TRUE;
			GetElement( "Gx", TRUE );
			m_pLogGx->Resize( GetCnt(), 0 );
			GetElement( "Gy", TRUE );
			m_pLogGy->Resize( GetCnt(), 0 );
		}
		
		if( !m_pLogDirection ){
			bCreateDir = TRUE;
			GetElement( "Direction", TRUE );
			m_pLogDirection->Resize( GetCnt(), 0 );
		}
		
		#ifdef USE_TURN_R
			if( !m_pLogTurnR ){
				bCreateTurnR = TRUE;
				GetElement( "TurnR", TRUE );
				m_pLogTurnR->Resize( GetCnt(), MAX_TURN_R );
			}
		#endif
	}
	
	#pragma omp parallel
	{
		double dMaxSpeed, dMinSpeed;
		double dMaxGx, dMinGx;
		double dMaxGy, dMinGy;
		
		dMaxSpeed = -FLT_MAX;
		dMinSpeed =  FLT_MAX;
		
		// �X�s�[�h����
		if( bCreateSpeed ){
			#pragma omp for
			for( int i = 1; i < GetCnt(); ++i ){
				if( Time( i ) - Time( i - 1 ) >= ( TIME_STOP - TIME_STOP_MARGIN * 2 )){
					// ���Ԃ��J���Ă����~���O
					SetRawSpeed( i, 0 );
					if( dMaxSpeed < 0 ) dMaxSpeed = 0;
					if( dMinSpeed > 0 ) dMinSpeed = 0;
				}else{
					double d;
					SetRawSpeed( i,
						d = ( Distance( i ) - Distance( i - 1 ))
						* ( 3600.0 / 1000 * 1000 ) /
						( Time( i ) - Time( i - 1 ))
					);
					if( dMaxSpeed < d ) dMaxSpeed = d;
					if( dMinSpeed > d ) dMinSpeed = d;
				}
			}
			// �Ԍ��͂��ł� 0 �� SetRawSpeed( GetCnt() - 1, 0 );
			
			#pragma omp critical
			{
				SetMaxMinSpeed( dMaxSpeed, dMinSpeed );
			}
		}
		
		// G �v�Z
		if( bCreateG ){
			double	dBearingPrev = 100;
			
			#pragma omp for
			for( int i = 2; i < GetCnt() - 1; ++i ){
				if( dBearingPrev = 100 ){
					dBearingPrev = atan2( Y0( i ) - Y0( i - 2 ), X0( i ) - X0( i - 2 ));
				}
				
				// bearing �v�Z
				double dBearing = atan2( Y0( i + 1 ) - Y0( i - 1 ), X0( i + 1 ) - X0( i - 1 ));
				
				if( bCreateDir ){
					double dDir = dBearing / ToRAD + 90;
					if( dDir < 0 ) dDir += 360;
					SetDirection( i, dDir );
				}
				
				// �� G �v�Z
				// Gx / Gy �����
				SetRawGy( i,
					( Speed( i ) - Speed( i - 1 ))
					* ( 1000 / 3.600 / GRAVITY )
					/ ( Time( i ) - Time( i - 1 ))
				);
				
				// ��G = v��
				double dBearingDelta = dBearing - dBearingPrev;
				if     ( dBearingDelta >  M_PI ) dBearingDelta -= M_PI * 2;
				else if( dBearingDelta < -M_PI ) dBearingDelta += M_PI * 2;
				
				SetRawGx( i,
					dBearingDelta * ( 1000 / GRAVITY )
					/ ( Time( i ) - Time( i - 1 ))
					* ( Speed( i ) / 3.600 )
				);
				
				// �}5G �ȏ�́C�폜
				if( Gx( i ) < -3 || Gx( i ) > 3 ){
					SetRawGx( i, Gx( i - 1 ));
				}
				
				dBearingPrev = dBearing;
			}
			
			// �X���[�W���O
			#define G_SMOOTH_NUM	3	// ���ω�����v�f��
			#define G_SMOOTH_CNT	( m_dFreq > 5 ? 2 : 1 )	// �X���[�W���O��
			#define G_SMOOTH_RATIO	0.3	// min/max �v�Z�p�W��
			
			#pragma omp single
			{
				// �Ԍ��ƁC�ŏ� / �Ō�� G ������
				SetRawGx( 0, 0 ); SetRawGy( 0, 0 );
				SetRawGx( 1, 0 ); SetRawGy( 1, 0 );
				SetRawGx( 2, 0 ); SetRawGy( 2, 0 );
				SetRawGx( GetCnt() - 3, 0 ); SetRawGy( GetCnt() - 3, 0 );
				SetRawGx( GetCnt() - 2, 0 ); SetRawGy( GetCnt() - 2, 0 );
				SetRawGx( GetCnt() - 1, 0 ); SetRawGy( GetCnt() - 1, 0 );
			}
			
			for( UINT v = G_SMOOTH_CNT; v; --v ){
				dMaxGx = dMaxGy = -FLT_MAX;
				dMinGx = dMinGy =  FLT_MAX;
				double dx0, dy0;
				dx0 = dy0 = 0;
				
				#pragma omp for
				for( int i = ( G_SMOOTH_NUM - 1 ) / 2; i < GetCnt() - G_SMOOTH_NUM / 2; ++i ){
					
					// �Ԍ����͏����X�L�b�v
					if( i < 2 || i >= GetCnt() - 2 ) continue;
					
					if( Speed( i ) == 0 ){
						SetRawGx( i, 0 );
						SetRawGy( i, 0 );
						
						if( dMaxGx < 0 ) dMaxGx = 0;
						if( dMinGx > 0 ) dMinGx = 0;
						if( dMaxGy < 0 ) dMaxGy = 0;
						if( dMinGy > 0 ) dMinGy = 0;
					}else{
						double dx, dy;
						
						SetRawGx( i, dx = (
							( G_SMOOTH_NUM >= 7 ? Gx( i - 3 ) : 0 ) +
							( G_SMOOTH_NUM >= 6 ? Gx( i + 3 ) : 0 ) +
							( G_SMOOTH_NUM >= 5 ? Gx( i - 2 ) : 0 ) +
							( G_SMOOTH_NUM >= 4 ? Gx( i + 2 ) : 0 ) +
							( G_SMOOTH_NUM >= 3 ? Gx( i - 1 ) : 0 ) +
							( G_SMOOTH_NUM >= 2 ? Gx( i + 1 ) : 0 ) +
							Gx( i + 0 )
						) / G_SMOOTH_NUM );
						SetRawGy( i, dy = (
							( G_SMOOTH_NUM >= 7 ? Gy( i - 3 ) : 0 ) +
							( G_SMOOTH_NUM >= 6 ? Gy( i + 3 ) : 0 ) +
							( G_SMOOTH_NUM >= 5 ? Gy( i - 2 ) : 0 ) +
							( G_SMOOTH_NUM >= 4 ? Gy( i + 2 ) : 0 ) +
							( G_SMOOTH_NUM >= 3 ? Gy( i - 1 ) : 0 ) +
							( G_SMOOTH_NUM >= 2 ? Gy( i + 1 ) : 0 ) +
							Gy( i + 0 )
						) / G_SMOOTH_NUM );
						
						if( v == 1 ){
							dx0 = dx * G_SMOOTH_RATIO + dx0 * ( 1 - G_SMOOTH_RATIO );
							dy0 = dy * G_SMOOTH_RATIO + dy0 * ( 1 - G_SMOOTH_RATIO );
							
							if( dMaxGx < dx0 ) dMaxGx = dx0;
							if( dMinGx > dx0 ) dMinGx = dx0;
							if( dMaxGy < dy0 ) dMaxGy = dy0;
							if( dMinGy > dy0 ) dMinGy = dy0;
						}
					}
				}
			}
			
			#pragma omp critical
			{
				SetMaxMinGx( dMaxGx, dMinGx );
				SetMaxMinGy( dMaxGy, dMinGy );
			}
			
			#ifdef USE_TURN_R
				if( bCreateTurnR ){
					#pragma omp for
					for( int i = 2; i < GetCnt(); ++i ){
						// r = v * v / G
						double dTurnR = Gx( i ) == 0 || Speed( i ) < 1 ?
							MAX_TURN_R :
							pow( Speed( i ) * ( 1000.0 / 3600 ), 2 ) / ( fabs( Gx( i )) * GRAVITY );
						
						if( dTurnR > MAX_TURN_R ) dTurnR = MAX_TURN_R;
						SetTurnR( i, dTurnR );
					}
				}
			#endif
		}
  	}
	
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

/*** ���O���[�h *************************************************************/

int CVsdLog::ReadLog( const char *szFileName, const char *szReaderFunc, CLapLog *&pLapLog ){
	{
		// JavaScript �I�u�W�F�N�g������
		CScript Script( m_pVsd );
		if( Script.InitLogReader() != ERR_OK ){
			return 0;
		}
		
		// �X�N���v�g���s
		LPWSTR pStr = NULL;
		LPWSTR pReader = NULL;
		Script.Run_ss( L"ReadLog",
			StringNew( pStr, szFileName ),
			StringNew( pReader, szReaderFunc )
		);
		delete [] pStr;
		delete [] pReader;
		
		if( Script.m_uError != ERR_OK ){
			m_pVsd->DispErrorMessage( Script.GetErrorMessage());
			return 0;
		}
		
		/*** JS �� Log �ɃA�N�Z�X *******************************************/
		
		{
			v8::Isolate::Scope IsolateScope( Script.m_pIsolate );
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
			
			// CLog * �� vector
			std::vector<CLog *>	CArrays;
			
			UINT	uIdxTime		= ~0;
			UINT	uIdxLapTime		= ~0;
			UINT	uIdxDistance	= ~0;
			UINT	uIdxSpeed		= ~0;
			UINT	uIdxLong		= ~0;
			UINT	uIdxLati		= ~0;
			
			UINT	uIdx = 0;
			for( UINT u = 0; u < Keys->Length(); ++u ){
				v8::String::AsciiValue strKey( Keys->Get( u ));
				char *pKey = *strKey;
				
				v8::Local<v8::Array> ArrayTmp = v8::Local<v8::Array>::Cast( hLog->Get( Keys->Get( u )));
				if( ArrayTmp->IsArray()){
					if     ( !strcmp( *strKey, "Time"      )) uIdxTime		= uIdx;
					else if( !strcmp( *strKey, "Distance"  )) uIdxDistance	= uIdx;
					else if( !strcmp( *strKey, "Speed"     )) uIdxSpeed		= uIdx;
					else if( !strcmp( *strKey, "Longitude" )) uIdxLong		= uIdx;
					else if( !strcmp( *strKey, "Latitude"  )) uIdxLati		= uIdx;
					else if( !strcmp( *strKey, "LapTime"   )){
						uIdxLapTime	= uIdx;
						CArrays.push_back( NULL );
						goto Skip;	// LapTime �� CVsdLog �ɂ͐ς܂Ȃ�
					}
					
					// strKey �� vector �쐬
					CArrays.push_back( GetElement( pKey, TRUE ));
					
				  Skip:
					// JS �� property ��������� Array �� vector �����
					JSArrays.push_back( ArrayTmp );
					
					++uIdx;
				}
			}
			
			// Time ���݊m�F
			if( uIdxTime == ~0 ) return 0;
			m_iLogStartTime = ( time_t )JSArrays[ uIdxTime ]->Get( 0 )->NumberValue();
			
			BOOL bCreateDistance = FALSE;
			
			if( uIdxLong != ~0 && uIdxLati != ~0 ){
				// Distance �������Ƃ��́C���
				if( uIdxDistance == ~0 ){
					uIdxDistance = uIdx++;
					CArrays.push_back( GetElement( "Distance", TRUE ));
					bCreateDistance = TRUE;
				}
				
				// �ܓx�o�x�����[�g�� �ϊ��萔
				double	dLong0;
				double	dLati0;
				
				dLong0 = JSArrays[ uIdxLong ]->Get( 0 )->NumberValue();
				m_pLogLongitude->SetBaseVal( dLong0 );
				dLati0 = JSArrays[ uIdxLati ]->Get( 0 )->NumberValue();
				m_pLogLatitude->SetBaseVal( dLati0 );
				
				m_dLong2Meter =  GPSLogGetLength( dLong0, dLati0, dLong0 + 1.0 / 3600, dLati0 ) * 3600;
				m_dLati2Meter = -GPSLogGetLength( dLong0, dLati0, dLong0, dLati0 + 1.0 / 3600 ) * 3600;
			}
			
			// vector �ɐς�
			double	dDistance = 0;
			int		iSameCnt = 0;
			
			double	dLogHzTime	= 0;
			double	iLogHzCnt	= 0;
			
			UINT	uLapCnt	= 1;
			
			UINT	uCalibrating = 0;
			
			for( UINT uIdx = 0; uIdx < JSArrays[ uIdxTime ]->Length(); ++uIdx ){
				int iCnt = GetCnt();
				int	iLapTime = -1;
				
				for( UINT uKey = 0; uKey < Keys->Length(); ++uKey ){
					
					double dVal = JSArrays[ uKey ]->Get( uIdx )->NumberValue();
					
					if( uKey == uIdxTime ){
						time_t t = ( time_t )dVal;
						if( t < m_iLogStartTime ) t += 24 * 3600 * 1000;
						SetTime( iCnt, ( double )( t - m_iLogStartTime ));
					}else if( uKey == uIdxSpeed ){
						// �L�����u���[�V�������́C��U 0km/h �ɂ���
						if( dVal >= CALIB_MARK_SPEED ){
							++uCalibrating;
							dVal = 0;
						}else{
							uCalibrating = 0;
						}
						CArrays[ uKey ]->Set( iCnt, dVal );
					}else if( uKey == uIdxLapTime ){
						if( !_isnan( dVal )) iLapTime = ( int )dVal;
					}else{
						CArrays[ uKey ]->Set( iCnt, dVal );
					}
				}
				
				if( iCnt == 0 ){
					// �Ԍ��쐬
					if( uIdxDistance != ~0 ) SetDistance( 0, 0 );
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
					if( bCreateDistance ){
						double x = X0( iCnt - 1 ) - X0( iCnt ); x *= x;
						double y = Y0( iCnt - 1 ) - Y0( iCnt ); y *= y;
						
						dDistance += sqrt( x + y );
						SetDistance( iCnt, dDistance );
					}
					
					// �O�̃��O���� TIME_STOP ����Ă��Ă��� �w��km/h �ȉ��Ȃ�C��~�Ƃ݂Ȃ�
					double dDiffTime = Time( iCnt ) - Time( iCnt - 1 );
					if(
						uIdxDistance != ~0 &&
						dDiffTime >= TIME_STOP &&
						( Distance( iCnt ) - Distance( iCnt - 1 )) / dDiffTime < ( 5.0 /*[km/h]*/ * 1000 / 3600 )
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
					}else{
						// ��~���ԂłȂ���΁C���O Hz ���v�Z����
						dLogHzTime += dDiffTime;
						++iLogHzCnt;
					}
				}
				
				// ���b�v�^�C����ς�
				if( iLapTime >= 0 ){
					if( pLapLog == NULL ){
						pLapLog = new CLapLog();
						pLapLog->m_iLapMode = LAPMODE_MAGNET;
					}
					LAP_t	LapTime;
					
					LapTime.uLap	= uLapCnt;
					LapTime.fLogNum	= ( float )( GetCnt() - 1 );
					LapTime.iTime	= iLapTime;
					pLapLog->m_Lap.push_back( LapTime );
					
					if( iLapTime ) ++uLapCnt;
					
					if(
						iLapTime > 0 &&
						( pLapLog->m_iBestTime == TIME_NONE || pLapLog->m_iBestTime > iLapTime )
					){
						pLapLog->m_iBestTime	= iLapTime;
						pLapLog->m_iBestLap	= pLapLog->m_iLapNum - 1;
					}
					++pLapLog->m_iLapNum;
				}
				
				// �L�����u���[�V�������̎��Ԃ��L�^
				if( uCalibrating ){
					if( uCalibrating == 1 ){
						m_dCalibStart = m_dCalibStop;
						m_dCalibStop  = Time( GetCnt() - 1 );
					}
					// 300km/h �ɖ߂�
					SetRawSpeed( GetCnt() - 1, CALIB_MARK_SPEED );
				}
			}
			// ���O Hz �ŏI�W�v
			m_dFreq = iLogHzCnt / dLogHzTime * 1000;
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
		
		// Time �� Max Min �ݒ�
		m_pLogTime->InitMinMax();
		m_pLogTime->SetMaxMin( Time( GetCnt() - 2 ), 0 );
	}
	
	// Lap log �̔Ԍ�
	if( pLapLog ){
		LAP_t	LapTime;
		LapTime.fLogNum	= FLT_MAX;	// �Ԍ�
		LapTime.iTime	= 0;		// �Ԍ�
		pLapLog->m_Lap.push_back( LapTime );
	}
	
	return GetCnt();
}

/*** MAP ��]���� ***********************************************************/

void CVsdLog::RotateMap( double dAngle ){
	
	if( !m_pLogLongitude ) return;
	
	if( m_pLogX ){
		m_pLogX->InitMinMax();
		m_pLogY->InitMinMax();
	}
	
	for( int i = 0; i < GetCnt(); ++i ){
		SetX( i,  cos( dAngle ) * X0( i ) + sin( dAngle ) * Y0( i ));
		SetY( i, -sin( dAngle ) * X0( i ) + cos( dAngle ) * Y0( i ));
	}
}
