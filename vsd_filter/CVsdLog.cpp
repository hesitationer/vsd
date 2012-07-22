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
	m_Log	= new VSD_LOG_t[ MAX_VSD_LOG ];
	m_dFreq	= LOG_FREQ;
	
	float	NaN = sqrt( -1.0f );
	
	// ������
	for( UINT u = 0; u < MAX_VSD_LOG; ++u ){
		m_Log[ u ].fX0 = m_Log[ u ].fY0 = NaN;
	}
	
	m_dMaxG = 0;
	m_dMinG = 0;
	
	m_iMaxSpeed		= 0;
	m_dLogStartTime	= -1;
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdLog::~CVsdLog(){
	delete [] m_Log;
	m_Log	= NULL;
	m_iCnt	= 0;
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogUpConvert( GPS_LOG_t *GPSLog, UINT uCnt, BOOL bAllParam ){
	
	UINT	u;
	
	m_iCnt = 0;
	
	if( uCnt < 2 ) return 0;			// 2�f�[�^���Ȃ���ΏI��
	GPSLog[ uCnt ].fTime = FLT_MAX;		// �Ԍ�
	
	double	t;
	double	dMileage = 0;
	double	dBearing;
	
	m_Log[ 0 ].fSpeed	=
	m_Log[ 0 ].fTacho	=
	m_Log[ 0 ].fGx		=
	m_Log[ 0 ].fGy		=
	m_Log[ 0 ].fMileage	= 0;
	
	/*** GPS_LOG_t �̕��𑖍����āC���낢��␳ *****************************/
	
	for( u = 1; u < uCnt - 1; ++u ){
		// speed ���Ȃ��ꍇ�̕␳
		if( GPSLog[ u ].fSpeed == FLT_MAX ){
			GPSLog[ u ].fSpeed = ( float )(
				sqrt(
					pow( GPSLog[ u + 1 ].fX - GPSLog[ u - 1 ].fX, 2 ) +
					pow( GPSLog[ u + 1 ].fY - GPSLog[ u - 1 ].fY, 2 )
				) / ( GPSLog[ u + 1 ].fTime - GPSLog[ u - 1 ].fTime )
				* ( 3600.0 / 1000 )
			);
		}
		
		// bearing ���Ȃ��ꍇ�̕␳
		//if( GPSLog[ u ].fBearing == FLT_MAX ){
		if( 1 ){	// GPS �f�[�^�� bearing �͖���
			GPSLog[ u ].fBearing = ( float )(
				fmod(
					atan2(
						GPSLog[ u + 1 ].fY - GPSLog[ u - 1 ].fY,
						GPSLog[ u + 1 ].fX - GPSLog[ u - 1 ].fX
					) / ToRAD + 360 * 2 + 90,
					360
				)
			);
		}
	}
	
	GPSLog[ 0 ].fSpeed			= GPSLog[ 1 ].fSpeed;
	GPSLog[ 0 ].fBearing		= GPSLog[ 1 ].fBearing;
	GPSLog[ uCnt - 1 ].fSpeed	= GPSLog[ uCnt - 2 ].fSpeed;
	GPSLog[ uCnt - 1 ].fBearing	= GPSLog[ uCnt - 2 ].fBearing;
	
	for( u = 1; u < uCnt - 1; ++u ){
		// Gx / Gy �����
		GPSLog[ u ].fGy = ( float )(
			( GPSLog[ u + 1 ].fSpeed - GPSLog[ u - 1 ].fSpeed )
			* ( 1 / 3.600 / GRAVITY )
			/ ( GPSLog[ u + 1 ].fTime - GPSLog[ u - 1 ].fTime )
		);
		
		// ��G = v��
		dBearing = GPSLog[ u + 1 ].fBearing - GPSLog[ u - 1 ].fBearing;
		if     ( dBearing >  180 ) dBearing -= 360;
		else if( dBearing < -180 ) dBearing += 360;
		
		GPSLog[ u ].fGx = ( float )(
			dBearing * ( ToRAD / GRAVITY )
			/ ( GPSLog[ u + 1 ].fTime - GPSLog[ u - 1 ].fTime )
			* ( GPSLog[ u ].fSpeed / 3.600 )
		);
		
		// �}5G �ȏ�́C�폜
		if( GPSLog[ u ].fGx < -3 || GPSLog[ u ].fGx > 3 ){
			GPSLog[ u ].fGx = GPSLog[ u - 1 ].fGx;
		}
	}
	
	GPSLog[ 0 ].fGx			= GPSLog[ 1 ].fGx;
	GPSLog[ 0 ].fGy			= GPSLog[ 1 ].fGy;
	GPSLog[ uCnt - 1 ].fGx	= GPSLog[ uCnt - 2 ].fGx;
	GPSLog[ uCnt - 1 ].fGy	= GPSLog[ uCnt - 2 ].fGy;
	
	/************************************************************************/
	
	for( m_iCnt = 0, u = 0;m_iCnt < MAX_VSD_LOG; ++m_iCnt ){
		
		// GPSLog[ u ].fTime <= m_iCnt / LOG_FREQ < GPSLog[ u + 1 ].fTime
		// �͈̔͂ɂȂ�悤����
		
		// m_iCnt ��������������Ă���̂ŃC���N��
		if(( float )m_iCnt / LOG_FREQ < GPSLog[ u ].fTime ){
			for( ; ( float )m_iCnt / LOG_FREQ < GPSLog[ u ].fTime; ++m_iCnt );
		}
		
		// m_iCnt ������������Ă���̂ŁCu ���C���N��
		if(( float )m_iCnt / LOG_FREQ >= GPSLog[ u + 1 ].fTime ){
			for( ; ( float )m_iCnt / LOG_FREQ >= GPSLog[ u + 1 ].fTime; ++u );
			
			// GPS ���O�͈͂𒴂����̂� return
			if( u > uCnt - 2 ) break;
		}
		
		// 5�b�ȏ� GPS ���O�������Ă���΁C�⊮���̌v�Z�����Ȃ�
		//if( GPSLog[ u + 1 ].fTime - GPSLog[ u ].fTime > 5 ) continue;
		
		t = (( double )m_iCnt / LOG_FREQ - GPSLog[ u ].fTime )
			/ ( GPSLog[ u + 1 ].fTime - GPSLog[ u ].fTime );
		
		#define GetLogIntermediateVal( p )\
			( GPSLog[ u ].p * ( 1 - t ) + GPSLog[ u + 1 ].p * t )
		
		m_Log[ m_iCnt ].fX0 = ( float )GetLogIntermediateVal( fX );
		m_Log[ m_iCnt ].fY0 = ( float )GetLogIntermediateVal( fY );
		
		if( bAllParam ){
			m_Log[ m_iCnt ].fSpeed = ( float )GetLogIntermediateVal( fSpeed );
			
			if( m_iMaxSpeed < m_Log[ m_iCnt ].fSpeed )
				m_iMaxSpeed = ( int )ceil( m_Log[ m_iCnt ].fSpeed / 10 ) * 10;
			
			if( m_iCnt ){
				dMileage += sqrt(
					pow( m_Log[ m_iCnt - 1 ].fX0 - m_Log[ m_iCnt ].fX0, 2 ) +
					pow( m_Log[ m_iCnt - 1 ].fY0 - m_Log[ m_iCnt ].fY0, 2 )
				);
			}
			
			m_Log[ m_iCnt ].fMileage = ( float )dMileage;
			m_Log[ m_iCnt ].fTacho = 0;
			
			m_Log[ m_iCnt ].fGx = ( float )GetLogIntermediateVal( fGx );
			m_Log[ m_iCnt ].fGy = ( float )GetLogIntermediateVal( fGy );
		}else{
			// PSP GPS log �̂Ƃ��́CG �� MAX �l�݂̂��`�F�b�N
			if( m_dMaxG < m_Log[ m_iCnt ].fGy ) m_dMaxG = m_Log[ m_iCnt ].fGy;
			if( m_dMinG > m_Log[ m_iCnt ].fGy ) m_dMinG = m_Log[ m_iCnt ].fGy;
		}
	}
	
	// �X���[�W���O
	if( bAllParam ){
		UINT	v = 2;
		double	d;
		double	d2 = 0;
		
		while( v-- ) for( u = 2; u < ( UINT )m_iCnt - 2; ++u ){
			m_Log[ u ].fGx = (
				m_Log[ u - 2 ].fGx +
				m_Log[ u - 1 ].fGx +
				m_Log[ u + 0 ].fGx +
				m_Log[ u + 1 ].fGx +
				m_Log[ u + 2 ].fGx
			) / 5;
			d = m_Log[ u ].fGy = (
				m_Log[ u - 2 ].fGy +
				m_Log[ u - 1 ].fGy +
				m_Log[ u + 0 ].fGy +
				m_Log[ u + 1 ].fGy +
				m_Log[ u + 2 ].fGy
			) / 5;
			
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
		if( _isnan( m_Log[ i ].fX0 )){
			m_Log[ i ].fX = m_Log[ i ].fX0;
			m_Log[ i ].fY = m_Log[ i ].fY0;
		}else{
			m_Log[ i ].fX = ( float )(  cos( dAngle ) * m_Log[ i ].fX0 + sin( dAngle ) * m_Log[ i ].fY0 );
			m_Log[ i ].fY = ( float )( -sin( dAngle ) * m_Log[ i ].fX0 + cos( dAngle ) * m_Log[ i ].fY0 );
			
			if     ( dMaxX < m_Log[ i ].fX ) dMaxX = m_Log[ i ].fX;
			else if( dMinX > m_Log[ i ].fX ) dMinX = m_Log[ i ].fX;
			if     ( dMaxY < m_Log[ i ].fY ) dMaxY = m_Log[ i ].fY;
			else if( dMinY > m_Log[ i ].fY ) dMinY = m_Log[ i ].fY;
		}
	}
	
	dMaxX -= dMinX;
	dMaxY -= dMinY;
	
	m_dMapSize	= dMaxX > dMaxY ? dMaxX : dMaxY;
	m_dMapOffsX	= dMinX;
	m_dMapOffsY	= dMinY;
}
