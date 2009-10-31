/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdLog.cpp - CVsdLog class implementation
	$Id: CVsdLog.cpp 218 2009-10-29 15:22:40Z  $
	
*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "dds.h"
#include "../vsd/main.h"

#include "CVsdLog.h"

/*** macros *****************************************************************/

/*** �R���X�g���N�^ *********************************************************/

CVsdLog::CVsdLog(){
	m_iCnt	= 0;
	m_Log	= new VSD_LOG_t[ MAX_VSD_LOG ];
	
	float	NaN = 0;
	NaN /= *( volatile float *)&NaN;
	
	// ������
	for( UINT u = 0; u < MAX_VSD_LOG; ++u ){
		m_Log[ u ].fX0 = m_Log[ u ].fY0 = NaN;
	}
}

/*** �f�X�g���N�^ ***********************************************************/

CVsdLog::~CVsdLog(){
	delete [] m_Log;
	m_Log	= NULL;
	m_iCnt	= 0;
}

/*** GPS ���O�� up-convert **************************************************/

UINT CVsdLog::GPSLogUpConvert( GPS_LOG_t *GPSLog, UINT uCnt, BOOL bAllParam ){
	
	UINT	u		= 0;
	
	m_iCnt = 0;
	
	if( uCnt < 2 ) return 0;						// 2�f�[�^���Ȃ���ΏI��
	GPSLog[ uCnt ].fTime = ( float )999999999.9;	// �Ԍ�
	
	double	t;
	double	dMileage = 0;
	
	for( ;; ++m_iCnt ){
		
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
			(( float )( GPSLog[ u ].p * ( 1 - t ) + GPSLog[ u + 1 ].p * t ))
		
		m_Log[ m_iCnt ].fX0 = GetLogIntermediateVal( fX );
		m_Log[ m_iCnt ].fY0 = GetLogIntermediateVal( fY );
		
		if( bAllParam ){
			m_Log[ m_iCnt ].fSpeed = ( float )GetLogIntermediateVal( fSpeed );
			
			if( m_iCnt ){
				dMileage += sqrt(
					pow( m_Log[ m_iCnt - 1 ].fX0 - m_Log[ m_iCnt ].fX0, 2 ) +
					pow( m_Log[ m_iCnt - 1 ].fY0 - m_Log[ m_iCnt ].fY0, 2 )
				);
			}
			m_Log[ m_iCnt ].fMileage = ( float )dMileage;
			
			m_Log[ m_iCnt ].fGx =
			m_Log[ m_iCnt ].fGy =
			m_Log[ m_iCnt ].fTacho = 0;
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
		m_Log[ i ].fX = ( float )(  cos( dAngle ) * m_Log[ i ].fX0 + sin( dAngle ) * m_Log[ i ].fY0 );
		m_Log[ i ].fY = ( float )( -sin( dAngle ) * m_Log[ i ].fX0 + cos( dAngle ) * m_Log[ i ].fY0 );
		
		if     ( dMaxX < m_Log[ i ].fX ) dMaxX = m_Log[ i ].fX;
		else if( dMinX > m_Log[ i ].fX ) dMinX = m_Log[ i ].fX;
		if     ( dMaxY < m_Log[ i ].fY ) dMaxY = m_Log[ i ].fY;
		else if( dMinY > m_Log[ i ].fY ) dMinY = m_Log[ i ].fY;
	}
	
	dMaxX -= dMinX;
	dMaxY -= dMinY;
	
	m_dMapSize	= dMaxX > dMaxY ? dMaxX : dMaxY;
	m_dMapOffsX	= dMinX;
	m_dMapOffsY	= dMinY;
}
