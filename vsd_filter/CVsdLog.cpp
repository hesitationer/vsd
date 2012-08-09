/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdLog.cpp - CVsdLog class implementation
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "../vsd/main.h"
#include "CVsdLog.h"

/*** macros *****************************************************************/

#define GRAVITY			9.80665

/*** �R���X�g���N�^ *********************************************************/

CVsdLog::CVsdLog(){
	m_iCnt	= 0;
	m_dFreq	= LOG_FREQ;
	
	m_dMaxG = 0;
	m_dMinG = 0;
	
	m_iMaxSpeed		= 0;
	m_dLogStartTime	= -1;
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdLog::~CVsdLog(){
	m_iCnt	= 0;
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogUpConvert( std::vector<GPS_LOG_t>& GPSLog, UINT uCnt, BOOL bAllParam ){
	
	UINT	u;
	
	m_iCnt = 0;
	
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
			if( m_dMaxG < LogTmp.Gy()) m_dMaxG = LogTmp.Gy();
			if( m_dMinG > LogTmp.Gy()) m_dMinG = LogTmp.Gy();
		}
		
		m_Log.push_back( LogTmp );
	}
	
	// �X���[�W���O
	if( bAllParam ){
		UINT	v = 2;
		double	d;
		double	d2 = 0;
		
		while( v-- ) for( u = 2; u < ( UINT )m_iCnt - 2; ++u ){
			m_Log[ u ].SetGx((
				m_Log[ u - 2 ].Gx() +
				m_Log[ u - 1 ].Gx() +
				m_Log[ u + 0 ].Gx() +
				m_Log[ u + 1 ].Gx() +
				m_Log[ u + 2 ].Gx()
			) / 5 );
			m_Log[ u ].SetGy( d = (
				m_Log[ u - 2 ].Gy() +
				m_Log[ u - 1 ].Gy() +
				m_Log[ u + 0 ].Gy() +
				m_Log[ u + 1 ].Gy() +
				m_Log[ u + 2 ].Gy()
			) / 5 );
			
			d2 = d2 * 0.9 + d * 0.1;
			if( m_dMaxG < d2 ) m_dMaxG = d2;
			if( m_dMinG > d2 ) m_dMinG = d2;
		}
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
