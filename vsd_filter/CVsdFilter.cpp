/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFilter.cpp - CVsdFilter class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "CVsdFilter.h"
#include "CVsdFile.h"
#include "ScriptIF.h"
#include "rev_num.h"

/*** macros *****************************************************************/

#define SLineWidth		( m_piParamT[ TRACK_SLineWidth ] / 10.0 )

/*** static member **********************************************************/

HINSTANCE	CVsdFilter::m_hInst 	= NULL;
const int CVsdFilter::m_iRevision = PROG_REVISION;

/*** �R���X�g���N�^ *********************************************************/

void CVsdFilter::Constructor( void ){
	
	m_VsdLog 			= NULL;
	m_GPSLog 			= NULL;
	m_LapLog			= NULL;
	
	m_bCalcLapTimeReq	= TRUE;
	
	m_Polygon			= NULL;	// DrawPolygon �p�o�b�t�@
	m_pFont				= NULL;
	
	m_szLogFile			= NULL;
	m_szLogFileReader	= NULL;
	m_szGPSLogFile		= NULL;
	m_szGPSLogFileReader= NULL;
	m_szLapChart		= NULL;
	m_szSkinFile		= NULL;
	m_szSkinDirA		= NULL;
	m_szPluginDirA		= NULL;
	m_szSkinDirW		= NULL;
	m_szPluginDirW		= NULL;
	
	// str param �ɏ����l�ݒ�
	#define DEF_STR_PARAM( id, var, init, conf_name ) StringNew( var, init );
	#include "def_str_param.h"
	
	// plugin dll path �擾
	SetPluginDir();
	
	// SkinDir �Z�b�g
	SetSkinFile( m_szSkinFile );
	
	m_Script	= NULL;
	
	m_iWidth	=
	m_iHeight	= 0;
	
	//---- GDI+�̏����ݒ�
	Gdiplus::GdiplusStartup( &m_gdiplusToken, &m_gdiplusStartupInput, NULL );
}

/*** �f�X�g���N�^ ***********************************************************/

void CVsdFilter::Destructor( void ){
	//---- GDI+�̉��
	Gdiplus::GdiplusShutdown( m_gdiplusToken );
	
	delete m_VsdLog;
	delete m_GPSLog;
	delete m_LapLog;
	delete [] m_szLogFile;
	delete [] m_szLogFileReader;
	delete [] m_szGPSLogFile;
	delete [] m_szGPSLogFileReader;
	delete [] m_szLapChart;
	delete [] m_szSkinFile;
	delete [] m_szSkinDirA;
	delete [] m_szPluginDirA;
	delete [] m_szSkinDirW;
	delete [] m_szPluginDirW;
	delete [] m_Polygon;
	delete m_pFont;
	delete m_Script;
}

/*** ���O���[�h ************************************************************/

int CVsdFilter::ReadLog( CVsdLog *&pLog, const char *szFileName, const char *szReaderFunc ){
	if( pLog ) delete pLog;
	pLog = new CVsdLog( this );
	
	CLapLog *pLapLog = NULL;
	
	int iRet = pLog->ReadLog( szFileName, szReaderFunc, pLapLog );
	if( iRet ){
		if( pLog->m_pLogLongitude )
			pLog->RotateMap( m_piParamT[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	}else{
		delete pLog;
		pLog = NULL;
	}
	
	if( pLapLog ){
		pLapLog->m_iLapSrc = pLog == m_VsdLog ? LAPSRC_VSD : LAPSRC_GPS;
		
		if( m_LapLog ){
			// �d�����Ă���̂ŁC�D��x�̍������[�h���c�����CVSD �̕���D��
			if(
				m_LapLog->m_iLapMode <  LAPMODE_MAGNET ||
				m_LapLog->m_iLapMode == LAPMODE_MAGNET && pLog == m_VsdLog
			){
				delete m_LapLog;
				m_LapLog = pLapLog;
			}else{
				delete pLapLog;
			}
		}else{
			m_LapLog = pLapLog;
		}
	}
	
	return iRet;
}

/***************************************************************************/

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


/*** ���b�v�ԍ� �� ���O�ԍ� �ϊ� ********************************************/

double CVsdFilter::LapNum2LogNum( CVsdLog* Log, int iLapNum ){
	
	// iLapNum �����������Ƃ��� 0 ��Ԃ��Ƃ�
	if( iLapNum < 0 ) return 2;	// �Ԍ��������� Time=0 �̃��O
	
	if( m_LapLog->m_iLapSrc == LAPSRC_VSD ){
		// fLogNum �� VSD ���O�ԍ�
		if( Log == m_VsdLog ) return m_LapLog->m_Lap[ iLapNum ].fLogNum;
		
		// ���b�v�ԍ�(VSD)��GPS ���O�ԍ��ɕϊ�
		return Log->GetIndex(
			m_VsdLog->GetTime( m_LapLog->m_Lap[ iLapNum ].fLogNum ) * SLIDER_TIME,
			&VsdSt, &GPSSt
		);
	}else if( m_LapLog->m_iLapSrc == LAPSRC_GPS ){
		// fLogNum �� GPS ���O�ԍ�
		if( Log == m_GPSLog ) return m_LapLog->m_Lap[ iLapNum ].fLogNum;
		
		// ���b�v�ԍ�(GPS)��VSD ���O�ԍ��ɕϊ�
		return Log->GetIndex(
			Log->GetTime( m_LapLog->m_Lap[ iLapNum ].fLogNum ) * SLIDER_TIME,
			&GPSSt, &VsdSt
		);
	}
	
	// fLogNum �̓r�f�I�t���[���ԍ�
	return Log->GetIndex(
		m_LapLog->m_Lap[ iLapNum ].fLogNum,
		&VideoSt, Log == m_VsdLog ? &VsdSt : &GPSSt
	);
}

/*** ���b�v�^�C���Đ��� (�蓮) **********************************************/

CLapLog *CVsdFilter::CreateLapTimeHand( int iLapSrc ){
	
	int		iTime, iPrevTime;
	int		iFrame = 0;
	double	dLogNum;
	LAP_t	LapTime;
	
	CLapLog *pLapLog = new CLapLog();
	pLapLog->m_iLapMode = LAPMODE_HAND;
	pLapLog->m_iLapSrc  = iLapSrc;
	
	while(( iFrame = GetFrameMark( iFrame )) >= 0 ){
		
		if( pLapLog->m_iLapSrc == LAPSRC_GPS ){
			dLogNum	= GetLogIndex( iFrame, GPS, -1 );
			iTime	= m_GPSLog->GetTime( dLogNum );
			LapTime.fLogNum	= ( float )dLogNum;
		}else{
			// LAPSRC_VIDEO
			iTime	= ( int )( iFrame * 1000.0 / GetFPS());
			LapTime.fLogNum	= ( float )iFrame;
		}
		
		if( m_piParamT[ TRACK_SLineWidth ] < 0 ){
			// �W���J�[�i���[�h
			LapTime.uLap	= ( pLapLog->m_iLapNum / 2 ) + 1;
			LapTime.iTime	= ( pLapLog->m_iLapNum & 1 ) ? iTime - iPrevTime : 0;
		}else{
			LapTime.uLap	= pLapLog->m_iLapNum;
			LapTime.iTime	= pLapLog->m_iLapNum ? iTime - iPrevTime : 0;
		}
		
		iPrevTime = iTime;
		++iFrame;
		
		pLapLog->PushLap( LapTime );
	}
	
	if( pLapLog->m_Lap.size() == 0 ){
		delete pLapLog;
		return NULL;
	}
	LapTime.fLogNum	= FLT_MAX;	// �Ԍ�
	LapTime.iTime	= 0;		// �Ԍ�
	pLapLog->m_Lap.push_back( LapTime );
	
	return pLapLog;
}

/*** ���b�v�^�C���Đ��� (GPS auto) ******************************************/

CLapLog *CVsdFilter::CreateLapTimeAuto( void ){
	
	int iFrame;
	LAP_t	LapTime;
	
	if(( iFrame = GetFrameMark( 0 )) < 0 ) return NULL;
	
	/*** �X�^�[�g���C���̈ʒu���擾 ***/
	// iFrame �ɑΉ����� GPS ���O�ԍ��擾
	double dLogNum = GetLogIndex( iFrame, GPS, -1 );
	
	int iLogNum = ( int )dLogNum;
	
	// iLogNum �` iLogNum + 1 �̕��ʂ��Z�o
	
	double dAngle = atan2(
		( m_GPSLog->Y0( iLogNum + 1 ) - m_GPSLog->Y0( iLogNum )),
		( m_GPSLog->X0( iLogNum + 1 ) - m_GPSLog->X0( iLogNum ))
	);
	
	#define x1 m_dStartLineX1
	#define y1 m_dStartLineY1
	#define x2 m_dStartLineX2
	#define y2 m_dStartLineY2
	#define x3 m_GPSLog->X0( i )
	#define y3 m_GPSLog->Y0( i )
	#define x4 m_GPSLog->X0( i + 1 )
	#define y4 m_GPSLog->Y0( i + 1 )
	
	// ���z���d�ǂ̈ʒu�����߂�
	x2 = m_GPSLog->X0( dLogNum );	// �X�^�[�g�n�_
	y2 = m_GPSLog->Y0( dLogNum );
	
	x1 = x2 + cos( dAngle + 90 * ToRAD ) * SLineWidth / 2;
	y1 = y2 + sin( dAngle + 90 * ToRAD ) * SLineWidth / 2;
	x2 = x2 + cos( dAngle - 90 * ToRAD ) * SLineWidth / 2;
	y2 = y2 + sin( dAngle - 90 * ToRAD ) * SLineWidth / 2;
	
	/*****/
	
	CLapLog *pLapLog = new CLapLog();
	pLapLog->m_iLapMode = LAPMODE_GPS;
	pLapLog->m_iLapSrc  = LAPSRC_GPS;
	
	int iTime, iPrevTime;
	int	iLapNum = 0;
	
	for( int i = 1; i < m_GPSLog->GetCnt() - 1; ++i ){
		
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
			( m_GPSLog->Y0( i + 1 ) - m_GPSLog->Y0( i )),
			( m_GPSLog->X0( i + 1 ) - m_GPSLog->X0( i ))
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
		iTime = m_GPSLog->GetTime( dLogNum );
		
		if( m_piParamS[ SHADOW_LAP_START ] - 1 <= iLapNum && iLapNum <= m_piParamS[ SHADOW_LAP_END ] ){
			LapTime.uLap	= pLapLog->m_iLapNum;
			LapTime.fLogNum	= ( float )dLogNum;
			LapTime.iTime	= pLapLog->m_iLapNum ? iTime - iPrevTime : 0;
			
			iPrevTime = iTime;
			
			pLapLog->PushLap( LapTime );
		}
		++iLapNum;
	}
	
	if( pLapLog->m_Lap.size() == 0 ){
		delete pLapLog;
		return NULL;
	}
	
	LapTime.fLogNum	= FLT_MAX;	// �Ԍ�
	LapTime.iTime	= 0;		// �Ԍ�
	pLapLog->m_Lap.push_back( LapTime );
	
	return pLapLog;
}

/*** �`���[�g���[�h *********************************************************/

#define NAME_BUF_SIZE	64

int CVsdFilter::LapChartRead( const char *szFileName ){
	
	// ���ɗD��x�̍��� Log ������΃��^�[�� (���͂Ȃ�)
	if( m_LapLog && m_LapLog->m_iLapMode > LAPMODE_CHART ){
		return 0;
	}
	
	CLapLogAll *pLapLog = new CLapLogAll();
	int iRet = pLapLog->LapChartRead( szFileName );
	if( iRet == 0 ){
		delete pLapLog;
		return 0;
	}
	
	// �����ł���� m_LapLog ���㏑��
	if( m_LapLog ) delete m_LapLog;
	m_LapLog = pLapLog;
	return iRet;
}

/*** JavaScript IF �ǉ��̏����� *********************************************/

void CVsdFilter::InitJS_Sub( CVsdLog *pLog, v8::Local<v8::FunctionTemplate> tmpl ){
	if( !pLog ) return;
	
	char szBuf[ 256 ];
	
	v8::Handle<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
	v8::Handle<v8::ObjectTemplate> inst  = tmpl->InstanceTemplate();
	std::map<std::string, CLog *>::iterator it;
	
	for( it = pLog->m_Logs.begin(); it != pLog->m_Logs.end(); ++it ){
		if( m_VsdLog == NULL || pLog == m_VsdLog || m_VsdLog->GetElement( it->first.c_str()) == NULL ){
			// Max �o�^
			sprintf( szBuf, "Max%s", it->first.c_str());
			proto->Set(
				v8::String::New( szBuf ),
				v8::Number::New( pLog->GetMax( it->first.c_str()))
			);
			
			// Min �o�^
			sprintf( szBuf, "Min%s", it->first.c_str());
			proto->Set(
				v8::String::New( szBuf ),
				v8::Number::New( pLog->GetMin( it->first.c_str()))
			);
			
			// ���ݒl�o�^
			if( 0 );
			#define DEF_LOG( name ) \
			else if( strcmp( it->first.c_str(), #name ) == 0 ){ \
				inst->SetAccessor( v8::String::New( #name ), CVsdFilterIF::Get_##name ); \
			}
			#include "def_log.h"
			else{
				inst->SetAccessor( v8::String::New( it->first.c_str()), CVsdFilterIF::Get_Value );
			}
		}
	}
}

void CVsdFilter::InitJS( v8::Local<v8::FunctionTemplate> tmpl ){
	InitJS_Sub( m_VsdLog, tmpl );
	InitJS_Sub( m_GPSLog, tmpl );
}

/*** �t�@�C�����X�g�擾 *****************************************************/

// szPath �� [ MAX_PATH + 1 ] �̔z��ŁCwork �Ɏg�p�����
BOOL ListTree( LPTSTR szPath, LPCTSTR szFile, BOOL ( *CallbackFunc )( LPCTSTR, LPCTSTR, void * ), void *pParam ){
	
	HANDLE				hFindFile;		/* find handle						*/
	WIN32_FIND_DATA		FindData;		/* find data struc					*/
	
	BOOL	bSuccess = TRUE;			/* success flag						*/
	
	// szPath �� \ �ŏI����ĂȂ����̑Ώ�
	int iFileIdx = _tcslen( szPath );	// \0
	
	if( iFileIdx != 0 && szPath[ iFileIdx - 1 ] != '\\' ){
		_tcscat_s( szPath, MAX_PATH, _T( "\\" ));
		++iFileIdx;
	}
	
	_tcscat_s( szPath, MAX_PATH, szFile );
	
	if(( hFindFile = FindFirstFile( szPath, &FindData )) != INVALID_HANDLE_VALUE ){
		do{
			if( *FindData.cFileName != _T( '_' ) &&
				_tcscmp( FindData.cFileName, _T( "." ))&&
				_tcscmp( FindData.cFileName, _T( ".." ))){
				
				if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
					_tcscpy_s( &szPath[ iFileIdx ], MAX_PATH - iFileIdx, FindData.cFileName );
					ListTree( szPath, szFile, CallbackFunc, pParam );
				}else{
					szPath[ iFileIdx ] = _T( '\0' );
					if( !CallbackFunc( szPath, FindData.cFileName, pParam )){
						bSuccess = FALSE;
						break;
					}
				}
			}
		}while( FindNextFile( hFindFile, &FindData ));
		
		FindClose( hFindFile );
	}
	return( bSuccess );
}

/****************************************************************************/

BOOL WINAPI DllMain(
	HINSTANCE	hinstDLL,	// handle to DLL module
	DWORD		fdwReason,	// reason for calling function
	LPVOID		lpvReserved	// reserved
){
	if( fdwReason == DLL_PROCESS_ATTACH ){
		CVsdFilter::m_hInst = hinstDLL;
	}
	return TRUE;
}
