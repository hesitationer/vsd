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
	
	m_iCalibStart	=
	m_iCalibStop	= -1;
	
	#define DEF_LOG( name )	m_pLog##name = NULL;
	#include "def_log.h"
}

/*** frame# �ɑΉ����郍�O index �𓾂� *************************************/

double CVsdLog::GetIndex( double dFromVal, int *piFrom, int *piLog, int iPrevIdx ){
	if( piFrom[ 0 ] == piFrom[ 1 ] ) return 2;	// �Ԍ��������� Time=0 �̃��O
	return GetIndex(
		( int )((
			piLog[ 0 ] +
			( piLog[ 1 ] - piLog[ 0 ] ) * ( dFromVal - piFrom[ 0 ] ) /
			( double )( piFrom[ 1 ] - piFrom[ 0 ] )
		) / SLIDER_TIME ),
		iPrevIdx
	);
}

double CVsdLog::GetIndex( int iTime, int iPrevIdx ){
	int idx;
	
	// Time( idx ) <= iTime < Time( idx + 1 )
	// �ƂȂ� idx ��������
	if(
		iPrevIdx < 0 || iPrevIdx >= GetCnt() ||
		GetTime( iPrevIdx ) > iTime
	){
		// iPrevIdx �����������̂ŁCbinary serch ����
		int iSt = 0;
		int iEd = GetCnt() - 1;
		while( 1 ){
			idx = ( iSt + iEd ) / 2;
			if( iSt == iEd ) break;
			
			if( GetTime( idx ) > iTime ){
				iEd = idx - 1;
			}else if( GetTime( idx + 1 ) <= iTime ){
				iSt = idx + 1;
			}else{
				// �q�b�g
				break;
			}
		}
	}else{
		// iPrevIdx �͐���Ȃ̂ŁC������N�_�ɒP���T�[�`����
		idx = iPrevIdx;
		while( GetTime( idx + 1 ) <= iTime ) ++idx;
	}
	
	// index �̒[�������߂�
	return idx +
		( double )( iTime    - GetTime( idx )) /
		( GetTime( idx + 1 ) - GetTime( idx ));
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
	CopyRecord( 1, 2 );           AddStopRecord( 1, GetTime( 2 ) - 500 );
	CopyRecord( iCnt + 1, iCnt ); AddStopRecord( iCnt + 1, GetTime( iCnt ) + 500 );
	CopyRecord( iCnt + 2, iCnt ); AddStopRecord( iCnt + 2, WATCHDOG_TIME );
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogRescan( void ){
	
	/*** CLog �̕��𑖍����āC���낢��␳ *****************************/
	
	BOOL	bCreateSpeed	= FALSE;
	BOOL	bCreateG		= FALSE;
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
		double dMaxSpeed;
		double dMaxGx, dMinGx;
		double dMaxGy, dMinGy;
		
		dMaxSpeed = -FLT_MAX;
		
		// �X�s�[�h����
		if( bCreateSpeed ){
			#pragma omp for
			for( int i = 1; i < GetCnt(); ++i ){
				int iDiffTime = GetTime( i ) - GetTime( i - 1 );
				if(
					iDiffTime > ( TIME_STOP - TIME_STOP_MARGIN * 2 ) &&
					m_pLogDistance &&
					( Distance( i ) - Distance( i - 1 )) * 3600 / iDiffTime < KPH_STOP
				){
					// ���Ԃ��J���Ă��ĂȂ����ړ����Ă��Ȃ�
					SetRawSpeed( i, 0 );
					SetRawSpeed( i - 1, 0 );
					if( dMaxSpeed < 0 ) dMaxSpeed = 0;
				}else{
					double d;
					SetRawSpeed( i,
						d = ( Distance( i ) - Distance( i - 1 ))
						* ( 3600.0 / 1000 * 1000 ) /
						( GetTime( i ) - GetTime( i - 1 ))
					);
					// 100km/h �ȏ�́C+20% �ȏ�̒l�͖�������
					if( dMaxSpeed < d && ( d < 100 || dMaxSpeed * 1.2 > d )) dMaxSpeed = d;
				}
			}
			// �Ԍ��͂��ł� 0 �� SetRawSpeed( GetCnt() - 1, 0 );
			
			#pragma omp critical
			{
				//DebugMsgD( "val=%f max=%f\n", dMaxSpeed, m_pLogSpeed->GetMax());
				SetMaxMinSpeed( dMaxSpeed, 0 );
			}
		}
		
		// G �v�Z
		if( bCreateG ){
			#pragma omp for
			for( int i = 2; i < GetCnt() - 1; ++i ){
				// �� G �v�Z
				// Gx / Gy �����
				SetRawGy( i,
					( Speed( i ) - Speed( i - 1 ))
					* ( 1000 / 3.600 / GRAVITY )
					/ ( GetTime( i ) - GetTime( i - 1 ))
				);
				
				// ��G = v��
				double dBearingDelta = ( Direction( i ) - Direction( i - 1 )) * ToRAD;
				if     ( dBearingDelta >  M_PI ) dBearingDelta -= M_PI * 2;
				else if( dBearingDelta < -M_PI ) dBearingDelta += M_PI * 2;
				
				SetRawGx( i,
					dBearingDelta * ( 1000 / GRAVITY )
					/ ( GetTime( i ) - GetTime( i - 1 ))
					* ( Speed( i ) / 3.600 )
				);
				
				// �}5G �ȏ�́C�폜
				if( Gx( i ) < -3 || Gx( i ) > 3 ){
					SetRawGx( i, Gx( i - 1 ));
				}
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
			
			int iStart = ( G_SMOOTH_NUM - 1 ) / 2;
			if( iStart < 2 ) iStart = 2;
			int iEnd = GetCnt() - G_SMOOTH_NUM / 2;
			if( iEnd >= GetCnt() - 2 ) iEnd = GetCnt() - 2;
			
			double dSumGx = NaN;
			double dSumGy;
			float fBufGx[ G_SMOOTH_NUM ];
			float fBufGy[ G_SMOOTH_NUM ];
			
			for( UINT v = G_SMOOTH_CNT; v; --v ){
				dMaxGx = dMaxGy = 0;
				dMinGx = dMinGy = 0;
				
				#pragma omp for
				for( int i = iStart; i < iEnd; ++i ){
					
					if( _isnan( dSumGx )){
						dSumGx = 0;
						dSumGy = 0;
						
						// �o�b�t�@������
						for( int j = i - ( G_SMOOTH_NUM - 1 ) / 2; j < i + G_SMOOTH_NUM / 2; ++j ){
							fBufGx[ j % G_SMOOTH_NUM ] = ( float )Gx( j ); dSumGx += Gx( j );
							fBufGy[ j % G_SMOOTH_NUM ] = ( float )Gy( j ); dSumGy += Gy( j );
						}
					}
					
					// �X���[�W���O�Ώۃf�[�^�̍Ō�������[�h
					int k = i + G_SMOOTH_NUM / 2;
					fBufGx[ k % G_SMOOTH_NUM ] = ( float )Gx( k ); dSumGx += Gx( k );
					fBufGy[ k % G_SMOOTH_NUM ] = ( float )Gy( k ); dSumGy += Gy( k );
					
					if( Speed( i ) == 0 ){
						SetRawGx( i, 0 );
						SetRawGy( i, 0 );
					}else{
						double dx, dy;
						SetRawGx( i, dx = dSumGx / G_SMOOTH_NUM );
						SetRawGy( i, dy = dSumGy / G_SMOOTH_NUM );
						
						if(      dMaxGx < dx ) dMaxGx = dx;
						else if( dMinGx > dx ) dMinGx = dx;
						if(      dMaxGy < dy ) dMaxGy = dy;
						else if( dMinGy > dy ) dMinGy = dy;
					}
					
					// �X���[�W���O�Ώۃf�[�^�̐擪���A�����[�h
					k = i - ( G_SMOOTH_NUM - 1 ) / 2;
					dSumGx -= fBufGx[ k % G_SMOOTH_NUM ];
					dSumGy -= fBufGy[ k % G_SMOOTH_NUM ];
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
		
		// Direction �␳
		if( m_pLogDirection ){
			#pragma omp for
			for( int i = 1; i < GetCnt(); ++i ){
				if( Speed( i ) < KPH_STOP ) SetDirection( i, Direction( i - 1 ));
			}
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
			UINT	uIdxDirection	= ~0;
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
					else if( !strcmp( *strKey, "Direction" )) uIdxDirection	= uIdx;
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
			
			BOOL bCreateDistance  = FALSE;
			BOOL bCreateDirection = FALSE;
			
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
				
				// Direction �������Ƃ��́C���
				if( uIdxDirection == ~0 ){
					uIdxDirection = uIdx++;
					CArrays.push_back( GetElement( "Direction", TRUE ));
					bCreateDirection = TRUE;
				}
			}
			
			// vector �ɐς�
			double	dDistance = 0;
			int		iSameCnt = 0;
			
			int		iLogHzTime	= 0;
			int		iLogHzCnt	= 0;
			
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
					if( uIdxDistance  != ~0 ) SetDistance( 0, 0 );
					if( uIdxDirection != ~0 ) SetDirection( 0, 0 );
					CopyRecord( 1, 0 );
					CopyRecord( 2, 0 );
					AddStopRecord( 0, -WATCHDOG_TIME );
					AddStopRecord( 1, -TIME_STOP_MARGIN );
				}else{
					// �������Ԃ��A������ꍇ�̎�������
					if( GetTime( iCnt - 1 ) == GetTime( iCnt )){
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
								GetTime( iCnt - iSameCnt ) +
								( GetTime( iCnt ) - GetTime( iCnt - iSameCnt )) * j / iSameCnt
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
					
					// ���ʂ����
					if( bCreateDirection ){
						double dDir = atan2( Y0( iCnt ) - Y0( iCnt - 1 ), X0( iCnt ) - X0( iCnt - 1 )) / ToRAD + 90;
						if( dDir < 0 ) dDir += 360;
						SetDirection( iCnt, dDir );
					}
					
					// �O�̃��O���� TIME_STOP ����Ă��Ă��� �w��km/h �ȉ��Ȃ�C��~�Ƃ݂Ȃ�
					int iDiffTime = GetTime( iCnt ) - GetTime( iCnt - 1 );
					if(
						uIdxDistance != ~0 &&
						iDiffTime > TIME_STOP &&
						( Distance( iCnt ) - Distance( iCnt - 1 )) * 3600 / iDiffTime < KPH_STOP
					){
						// -1 -0
						// A  B
						//     ��
						// -1 -0 +1 +2
						// A  A' B' B
						CopyRecord( iCnt + 1, iCnt     ); // B'
						CopyRecord( iCnt + 2, iCnt     ); // B
						CopyRecord( iCnt,     iCnt - 1 ); // A'
						AddStopRecord( iCnt,     GetTime( iCnt - 1 ) + TIME_STOP_MARGIN ); // A'
						AddStopRecord( iCnt + 1, GetTime( iCnt + 2 ) - TIME_STOP_MARGIN ); // B'
						
						if( uIdxDirection ) SetDirection( iCnt + 2, Direction( iCnt - 1 ));
					}else{
						// ��~���ԂłȂ���΁C���O Hz ���v�Z����
						iLogHzTime += iDiffTime;
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
					pLapLog->PushLap( LapTime );
					
					if( iLapTime ) ++uLapCnt;
				}
				
				// �L�����u���[�V�������̎��Ԃ��L�^
				if( uCalibrating ){
					if( uCalibrating == 1 ){
						m_iCalibStart = m_iCalibStop;
						m_iCalibStop  = GetTime( GetCnt() - 1 );
						
						if( m_iCalibStart < 0 ) m_iCalibStart = m_iCalibStop;
					}
					// 300km/h �ɖ߂�
					SetRawSpeed( GetCnt() - 1, CALIB_MARK_SPEED );
				}
			}
			// ���O Hz �ŏI�W�v
			m_dFreq = 1000.0 * iLogHzCnt / iLogHzTime;
		}
	}
	
	/************************************************************************/
	
	if( GetCnt()){
		int iCnt = GetCnt();
		
		// �I�[���̔Ԍ�
		CopyRecord( iCnt,     iCnt - 1 );
		CopyRecord( iCnt + 1, iCnt - 1 );
		AddStopRecord( iCnt,     GetTime( iCnt - 1 ) + TIME_STOP_MARGIN );
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
		m_pLogTime->SetMaxMin( GetTime( GetCnt() - 2 ), 0 );
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
	
	GetElement( "X", TRUE );
	GetElement( "Y", TRUE );
	
	m_pLogX->InitMinMax();
	m_pLogY->InitMinMax();
	
	for( int i = 0; i < GetCnt(); ++i ){
		SetX( i,  cos( dAngle ) * X0( i ) + sin( dAngle ) * Y0( i ));
		SetY( i, -sin( dAngle ) * X0( i ) + cos( dAngle ) * Y0( i ));
	}
}

/*** �`���[�g���[�h *********************************************************/

int CLapLogAll::LapChartRead( const char *szFileName, CVsdFilter *pVsd ){
	m_iLapMode = LAPMODE_CHART;
	m_iLapSrc  = LAPSRC_VIDEO;
	
	{
		// JavaScript �I�u�W�F�N�g������
		CScript Script( pVsd );
		Script.Initialize();
		
		if( Script.RunFile( L"_log_reader\\_LapChartReader.js" ) == ERR_OK ){
			// �X�N���v�g���s
			LPWSTR pStr = NULL;
			LPWSTR pReader = NULL;
			Script.Run_s( L"ReadLapChart", StringNew( pStr, szFileName ));
			delete [] pStr;
			delete [] pReader;
		}
		
		if( Script.m_uError != ERR_OK ){
			pVsd->DispErrorMessage( Script.GetErrorMessage());
			return 0;
		}
		
		/*** JS �� Log �ɃA�N�Z�X *******************************************/
		
		{
			v8::Isolate::Scope IsolateScope( Script.m_pIsolate );
			v8::HandleScope handle_scope;
			v8::Context::Scope context_scope( Script.m_Context );
			
			// "LapTime" �擾
			v8::Local<v8::Array> hLapTime = v8::Local<v8::Array>::Cast(
				Script.m_Context->Global()->Get( v8::String::New(( uint16_t *)L"LapTime" ))
			);
			if( !hLapTime->IsArray()) return 0;
			
			// �J�����J�[ No �擾
			m_iCamCarIdx = Script.m_Context->Global()->Get( v8::String::New(( uint16_t *)L"CameraCarID" ))->Int32Value();
			
			// LapTime �� key �擾
			v8::Local<v8::Array> Keys = hLapTime->GetPropertyNames();
			
			for( UINT u = 0; u < Keys->Length(); ++u ){
				v8::String::Value strKey( Keys->Get( u ));
				m_strName.push_back( reinterpret_cast<LPWSTR>( *strKey ));
				
				std::vector<int>	int_vec;	// �_�~�[
				m_LapTable.push_back( int_vec );
				
				v8::Local<v8::Array> ArrayTmp = v8::Local<v8::Array>::Cast( hLapTime->Get( Keys->Get( u )));
				for( UINT v = 0; v < ArrayTmp->Length(); ++ v ){
					m_LapTable[ u ].push_back( ArrayTmp->Get( v )->Int32Value());
				}
			}
		}
	}
	
	if( m_LapTable.size() == 0 || CamCarLap().size() <= 1 ){
		return 0;
	}
	
	// ���b�v�^�C���\�������\�ɕϊ�
	for( int j = 0; j < ( int )m_LapTable.size(); ++j ){
		for( int i = m_LapTable[ j ].size() - 2; i >= 0; --i ){
			m_LapTable[ j ][ i ] = m_LapTable[ j ][ i + 1 ] - m_LapTable[ j ][ i ];
		}
	}
	
	// vector �z������
	m_iAllLapIdx.resize( m_strName.size(), -1 );
	m_iAllGapInfo.resize( m_strName.size(), 0 );
	m_iPositionInfo.resize( m_strName.size(), -1 );
	
	return CamCarLap().size();
}

/*** ���ԃ��b�v�f�[�^�\�z ***************************************************/

void CLapLogAll::MakeCamLapData( int iStartFrame, int iEndFrame ){
	
	LAP_t	Lap = {
		0,	// uLap
		0,	// fLogNum
		0	// iTime
	};
	
	m_Lap.clear();
	m_iLapNum = 0;
	
	m_iStartFrame = iStartFrame;
	m_iEndFrame = iEndFrame;
	
	int iTimeSum = CamCarLap()[ CamCarLap().size() - 1 ] - CamCarLap()[ 0 ];
	
	// ���b�v�f�[�^���\�z
	Lap.fLogNum = ( float )m_iStartFrame;
	PushLap( Lap );	// �v���X�^�[�g ( iTime = 0 )
	int iTime = 0;
	
	for( int i = 1; i < ( int )CamCarLap().size(); ++i ){
		iTime += Lap.iTime = GetLapTime( i );
		++Lap.uLap;
		
		Lap.fLogNum = ( float )(
			m_iStartFrame +
			( m_iEndFrame - m_iStartFrame ) * ( double )iTime / iTimeSum
		);
		PushLap( Lap );
	}
	
	Lap.fLogNum	= FLT_MAX;	// �Ԍ�
	Lap.iTime	= 0;		// �Ԍ�
	m_Lap.push_back( Lap );
	
	// CalcLapInfo() �����Z�b�g
	m_iPrevFrame	= INT_MAX;
}

/*** �e���̃��b�v���Čv�Z *************************************************/

void CLapLogAll::CalcLapInfo( int iFrameCnt, double dFPS ){
	
	int iFrameCntRaw = iFrameCnt;
	int i;
	
	// �X�^�[�g�O�E�S�[����� iFrameCnt ��␳
	if( m_iLapIdx < 0 ){
		iFrameCnt = GetLapFrame( 0 );
	}else if( m_iLapIdx >= ( int )CamCarLap().size() - 1 ){
		iFrameCnt = GetLapFrame( CamCarLap().size() - 1 );
	}else{
		//iFrameCnt = GetFrameCnt();
	}
	
	// �����߂����ꂽ�Ƃ��́C�擪����T�[�`������
	if( iFrameCntRaw < m_iPrevFrame ){
		for( i = 0; i < ( int )m_iPositionInfo.size(); ++i ){
			m_iAllLapIdx[ i ] = m_iPositionInfo[ i ] = -1;
			m_iPrevFrame = -1;
		}
	}
	
	// �O��̉��Z�ʒu���� Frame# �܂Ői�߂Ă����āC���ʂ��v�Z����
	for( i = m_iPrevFrame + 1; i <= iFrameCntRaw; ++i ){
		for( UINT u = 0; u < m_iPositionInfo.size(); ++u ){
			
			// ���݃��b�v�������߂�
			int iLapIdx = m_iAllLapIdx[ u ];
			for(;
				iLapIdx <= ( int )m_LapTable[ u ].size() - 2 &&
				i >= GetLapFrame( u, iLapIdx + 1 );
				++iLapIdx
			);
			
			// ���b�v�����X�V���ꂽ
			if( iLapIdx != m_iAllLapIdx[ u ] ){
				
				m_iAllLapIdx[ u ] = iLapIdx;
				
				// ��U���Ԃ̗v�f������
				m_iPositionInfo.erase(
					std::remove(
						m_iPositionInfo.begin(),
						m_iPositionInfo.end(),
						u
					),
					m_iPositionInfo.end()
				);
				
				if( m_iPositionInfo.size() == m_iAllLapIdx.size()){
					m_iPositionInfo.pop_back();
				}
				
				// �}���|�C���g��T���đ}��
				std::vector<int>::iterator v;
				for( v = m_iPositionInfo.begin(); v != m_iPositionInfo.end(); ++v ){
					if(
						*v == -1 ||
						m_iAllLapIdx[ *v ] < iLapIdx
					) break;
				}
				
				// ���b�v���[�_�[���i�[
				if( v == m_iPositionInfo.begin()){
					if(( int )m_iLapLeader.size() <= iLapIdx ){
						m_iLapLeader.push_back( u );
					}else{
						m_iLapLeader[ iLapIdx ] = u;
					}
				}
				
				m_iPositionInfo.insert( v, u );
			}
		}
	}
	m_iPrevFrame = iFrameCntRaw;
	
	// �e�Ԃ̎��� index �����߂�
	#ifdef _OPENMP_AVS
		#pragma omp parallel for
	#endif
	for( UINT u = 0; u < m_strName.size(); ++u ){
		int iLapIdx = m_iAllLapIdx[ u ];
		
		// ���b�v���Ɖ� % ��i�񂾂������߂�
		double dProceeding;
		if( iLapIdx < 0 ){
			iLapIdx = 0;
		}else if( iLapIdx == m_LapTable[ u ].size() - 1 ){
			--iLapIdx;
		}
		dProceeding =
			( double )( iFrameCnt           - GetLapFrame( u, iLapIdx )) /
			( GetLapFrame( u, iLapIdx + 1 ) - GetLapFrame( u, iLapIdx ));
		
		// ��ŋ��߂��ʒu�́C�J�����Ԃɂ�����t���[���ԍ�������
		// ��������J�����ԂƂ̎��ԍ������߂� push
		// ���� 8bit �� id, ��ʎc�肪�^�C����
		m_iAllGapInfo[ u ] = (
			( int )(
				(
					GetLapFrame( iLapIdx ) - iFrameCnt +
					( GetLapFrame( iLapIdx + 1 ) - GetLapFrame( iLapIdx )) * dProceeding
				) / dFPS * -1000
			) << 8
		) | u;
	}
	
	std::sort( m_iAllGapInfo.begin(), m_iAllGapInfo.end());	// �\�[�g
}
