/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter.cpp - VSD filter for AviUti
	$Id$
	
*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "vsd_filter.h"
#include "dds.h"
#include "filter.h"
#include "../vsd/main.h"
#include "dds_lib/dds_lib.h"
#include "CVsdImg.h"
#include "CVsdLog.h"

/*** macros *****************************************************************/

#ifdef CIRCUIT_TOMO
	#define	FILE_EXT		"Pulse-Time Data (*.ptd)\0*.ptd\0Config File (*.cfg)\0*.cfg\0AllFile (*.*)\0*.*\0"
#else
	#define	FILE_EXT		"LogFile (*.log)\0*.log\0Config File (*.cfg)\0*.cfg\0AllFile (*.*)\0*.*\0"
#endif

#define	FILE_CFG_EXT		"Config File (*.cfg)\0*.cfg\0AllFile (*.*)\0*.*\0"

/*** new type ***************************************************************/

/*** gloval var *************************************************************/

HINSTANCE	g_hInst 	= NULL;
CVsdLog		*g_VsdLog;

/****************************************************************************/
//---------------------------------------------------------------------
//		�t�B���^�\���̒�`
//---------------------------------------------------------------------

// �g���b�N�o�[�̖��O
TCHAR	*track_name[] = {
	#define DEF_TRACKBAR( id, init, min, max, name )	name,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̏����l
int		track_default[] = {
	#define DEF_TRACKBAR( id, init, min, max, name )	init,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̉����l
int		track_s[] = {
	#define DEF_TRACKBAR( id, init, min, max, name )	min,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̏���l
int		track_e[] = {
	#define DEF_TRACKBAR( id, init, min, max, name )	max,
	#include "def_trackbar.h"
};

// �`�F�b�N�{�b�N�X�̖��O
TCHAR	*check_name[] = {
	#define DEF_CHECKBOX( id, init, name )	name,
	#include "def_checkbox.h"
};

// �`�F�b�N�{�b�N�X�̏����l (�l��0��1)
int		check_default[] = {
	#define DEF_CHECKBOX( id, init, name )	init,
	#include "def_checkbox.h"
};

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_IMPORT | FILTER_FLAG_EXPORT | FILTER_FLAG_MAIN_MESSAGE,
								//	�t�B���^�̃t���O
								//	FILTER_FLAG_ALWAYS_ACTIVE		: �t�B���^����ɃA�N�e�B�u�ɂ��܂�
								//	FILTER_FLAG_CONFIG_POPUP		: �ݒ���|�b�v�A�b�v���j���[�ɂ��܂�
								//	FILTER_FLAG_CONFIG_CHECK		: �ݒ���`�F�b�N�{�b�N�X���j���[�ɂ��܂�
								//	FILTER_FLAG_CONFIG_RADIO		: �ݒ�����W�I�{�^�����j���[�ɂ��܂�
								//	FILTER_FLAG_EX_DATA				: �g���f�[�^��ۑ��o����悤�ɂ��܂��B
								//	FILTER_FLAG_PRIORITY_HIGHEST	: �t�B���^�̃v���C�I���e�B����ɍŏ�ʂɂ��܂�
								//	FILTER_FLAG_PRIORITY_LOWEST		: �t�B���^�̃v���C�I���e�B����ɍŉ��ʂɂ��܂�
								//	FILTER_FLAG_WINDOW_THICKFRAME	: �T�C�Y�ύX�\�ȃE�B���h�E�����܂�
								//	FILTER_FLAG_WINDOW_SIZE			: �ݒ�E�B���h�E�̃T�C�Y���w��o����悤�ɂ��܂�
								//	FILTER_FLAG_DISP_FILTER			: �\���t�B���^�ɂ��܂�
								//	FILTER_FLAG_EX_INFORMATION		: �t�B���^�̊g������ݒ�ł���悤�ɂ��܂�
								//	FILTER_FLAG_NO_CONFIG			: �ݒ�E�B���h�E��\�����Ȃ��悤�ɂ��܂�
								//	FILTER_FLAG_AUDIO_FILTER		: �I�[�f�B�I�t�B���^�ɂ��܂�
								//	FILTER_FLAG_RADIO_BUTTON		: �`�F�b�N�{�b�N�X�����W�I�{�^���ɂ��܂�
								//	FILTER_FLAG_WINDOW_HSCROLL		: �����X�N���[���o�[�����E�B���h�E�����܂�
								//	FILTER_FLAG_WINDOW_VSCROLL		: �����X�N���[���o�[�����E�B���h�E�����܂�
								//	FILTER_FLAG_IMPORT				: �C���|�[�g���j���[�����܂�
								//	FILTER_FLAG_EXPORT				: �G�N�X�|�[�g���j���[�����܂�
	0,0,						//	�ݒ�E�C���h�E�̃T�C�Y (FILTER_FLAG_WINDOW_SIZE�������Ă��鎞�ɗL��)
	"VSD���[�^�[����",			//	�t�B���^�̖��O
	TRACK_N,					//	�g���b�N�o�[�̐� (0�Ȃ疼�O�����l����NULL�ł悢)
	track_name,					//	�g���b�N�o�[�̖��O�S�ւ̃|�C���^
	track_default,				//	�g���b�N�o�[�̏����l�S�ւ̃|�C���^
	track_s, track_e,			//	�g���b�N�o�[�̐��l�̉������ (NULL�Ȃ�S��0�`256)
	CHECK_N,					//	�`�F�b�N�{�b�N�X�̐� (0�Ȃ疼�O�����l����NULL�ł悢)
	check_name,					//	�`�F�b�N�{�b�N�X�̖��O�S�ւ̃|�C���^
	check_default,				//	�`�F�b�N�{�b�N�X�̏����l�S�ւ̃|�C���^
	func_proc,					//	�t�B���^�����֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL, //func_init,			//	�J�n���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL, //func_exit,			//	�I�����ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	func_update,				//	�ݒ肪�ύX���ꂽ�Ƃ��ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	func_WndProc,				//	�ݒ�E�B���h�E�ɃE�B���h�E���b�Z�[�W���������ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,NULL,					//	�V�X�e���Ŏg���܂��̂Ŏg�p���Ȃ��ł�������
	NULL,						//  �g���f�[�^�̈�ւ̃|�C���^ (FILTER_FLAG_EX_DATA�������Ă��鎞�ɗL��)
	NULL,						//  �g���f�[�^�T�C�Y (FILTER_FLAG_EX_DATA�������Ă��鎞�ɗL��)
	NULL,						//  �t�B���^���ւ̃|�C���^ (FILTER_FLAG_EX_INFORMATION�������Ă��鎞�ɗL��)
	NULL,						//	�Z�[�u���J�n����钼�O�ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	NULL,						//	�Z�[�u���I���������O�ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
};


//---------------------------------------------------------------------
//		�t�B���^�\���̂̃|�C���^��n���֐�
//---------------------------------------------------------------------

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void ){
	return &filter;
}

//---------------------------------------------------------------------
//		������
//---------------------------------------------------------------------

/*
BOOL func_init( FILTER *fp ){
	return TRUE;
}
*/

//---------------------------------------------------------------------
//		�I��
//---------------------------------------------------------------------

/*
BOOL func_exit( FILTER *fp ){
	return TRUE;
}
*/

//---------------------------------------------------------------------
//		�t�B���^�����֐�
//---------------------------------------------------------------------

/*** ���b�v�^�C���Čv�Z *****************************************************/

#ifdef AVS_PLUGIN
void CalcLapTime( void ){
	// �� under construction ...
}
#else
void CalcLapTime( CVsdImg &Img, CVsdLog &Log ){
	
	FRAME_STATUS	fsp;
	int				i;
	double			dTime, dPrevTime;
	
	Log.m_iLapNum	= 0;
	Log.m_fBestTime	= -1;
	
	for( i = 0; i < Img.frame_n; ++i ){
		( Log.filter )->exfunc->get_frame_status( Img.editp, i, &fsp );
		
		if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
			// ���b�v���o
			dTime = i / Log.m_dVideoFPS;
			
			Log.m_Lap[ Log.m_iLapNum ].uLap		= Log.m_iLapNum;
			Log.m_Lap[ Log.m_iLapNum ].iLogNum	= i;
			Log.m_Lap[ Log.m_iLapNum ].fTime	= Log.m_iLapNum ? ( float )( dTime - dPrevTime ) : 0;
			
			if(
				Log.m_iLapNum &&
				( Log.m_fBestTime == -1 || Log.m_fBestTime > Log.m_Lap[ Log.m_iLapNum ].fTime )
			){
				Log.m_fBestTime			= Log.m_Lap[ Log.m_iLapNum ].fTime;
				Log.m_iBestLapLogNum	= Log.m_Lap[ Log.m_iLapNum - 1 ].iLogNum;
			}
			
			dPrevTime = dTime;
			++Log.m_iLapNum;
		}
	}
	Log.m_Lap[ Log.m_iLapNum ].iLogNum	= 0x7FFFFFFF;	// �Ԍ�
	Log.m_Lap[ Log.m_iLapNum ].fTime	= 0;			// �Ԍ�
}
#endif

/*** macros *****************************************************************/

#define VideoSt			( Log.m_piParamT[ TRACK_VSt ] * 100 + Log.m_piParamT[ TRACK_VSt2 ] )
#define VideoEd			( Log.m_piParamT[ TRACK_VEd ] * 100 + Log.m_piParamT[ TRACK_VEd2 ] )
#ifdef CIRCUIT_TOMO
	#define LogSt		(( Log.m_piParamT[ TRACK_LSt ] * 1000 + Log.m_piParamT[ TRACK_LSt2 ] ) / 1000 * LOG_FREQ )
	#define LogEd		(( Log.m_piParamT[ TRACK_LEd ] * 1000 + Log.m_piParamT[ TRACK_LEd2 ] ) / 1000 * LOG_FREQ )
#else
	#define LogSt		( Log.m_piParamT[ TRACK_LSt ] * 100 + Log.m_piParamT[ TRACK_LSt2 ] )
	#define LogEd		( Log.m_piParamT[ TRACK_LEd ] * 100 + Log.m_piParamT[ TRACK_LEd2 ] )
#endif
#define LineTrace		Log.m_piParamT[ TRACK_LineTrace ]

#define DispLap			Log.m_piParamC[ CHECK_LAP ]
#define DispGSnake		Log.m_piParamC[ CHECK_SNAKE ]

#define MAX_MAP_SIZE	( Img.w * Log.m_piParamT[ TRACK_MapSize ] / 1000.0 )

/*** ���[�^�[���`�� *********************************************************/

static const PIXEL_YC	yc_black		= RGB2YC(    0,    0,    0 );
static const PIXEL_YC	yc_white		= RGB2YC( 4095, 4095, 4095 );
static const PIXEL_YC	yc_gray			= RGB2YC( 2048, 2048, 2048 );
static const PIXEL_YC	yc_red			= RGB2YC( 4095,    0,    0 );
static const PIXEL_YC	yc_green		= RGB2YC(    0, 4095,    0 );
static const PIXEL_YC	yc_yellow		= RGB2YC( 4095, 4095,    0 );
static const PIXEL_YC	yc_dark_green	= RGB2YC(    0, 2048,    0 );
static const PIXEL_YC	yc_blue			= RGB2YC(    0,    0, 4095 );
static const PIXEL_YC	yc_cyan			= RGB2YC(    0, 4095, 4095 );
static const PIXEL_YC	yc_dark_blue	= RGB2YC(    0,    0, 2048 );
static const PIXEL_YC	yc_orange		= RGB2YC( 4095, 1024,    0 );

#define COLOR_PANEL			yc_gray
#define COLOR_NEEDLE		yc_red
#define COLOR_SCALE			yc_white
#define COLOR_STR			COLOR_SCALE
#define COLOR_TIME			yc_white
#define COLOR_TIME_EDGE		yc_black
#define COLOR_BEST_LAP		yc_cyan
#define COLOR_G_SENSOR		yc_green
#define COLOR_G_HIST		yc_dark_green
#define COLOR_DIFF_MINUS	yc_cyan
#define COLOR_DIFF_PLUS		yc_red
#define COLOR_CURRENT_POS	yc_red
#define COLOR_FASTEST_POS	yc_green

// ���[�� dLogNum �l���烍�O�̒��Ԓl�����߂�
#define GetVsdLog( p ) ( \
	Log.m_VsdLog[ ( UINT )dLogNum     ].p * ( 1 - ( dLogNum - ( UINT )dLogNum )) + \
	Log.m_VsdLog[ ( UINT )dLogNum + 1 ].p * (       dLogNum - ( UINT )dLogNum ))


BOOL DrawVSD( CVsdImg &Img, CVsdLog &Log ){
//
//	fp->track[n]			: �g���b�N�o�[�̐��l
//	fp->check[n]			: �`�F�b�N�{�b�N�X�̐��l
//	fpip->w 				: ���ۂ̉摜�̉���
//	fpip->h 				: ���ۂ̉摜�̏c��
//	fpip->w					: �摜�̈�̉���
//	fpip->h					: �摜�̈�̏c��
//	fpip->ycp_edit			: �摜�̈�ւ̃|�C���^
//	fpip->ycp_temp			: �e���|�����̈�ւ̃|�C���^
//	fpip->ycp_edit[n].y		: ��f(�P�x    )�f�[�^ (     0 �` 4095 )
//	fpip->ycp_edit[n].cb	: ��f(�F��(��))�f�[�^ ( -2048 �` 2047 )
//	fpip->ycp_edit[n].cr	: ��f(�F��(��))�f�[�^ ( -2048 �` 2047 )
//
//  ��f�f�[�^�͔͈͊O�ɏo�Ă��邱�Ƃ�����܂��B
//  �܂��͈͓��Ɏ��߂Ȃ��Ă����܂��܂���B
//
//	�摜�T�C�Y��ς������Ƃ��� fpip->w �� fpip->h ��ς��܂��B
//
//	�e���|�����̈�ɏ��������摜���i�[�������Ƃ���
//	fpip->ycp_edit �� fpip->ycp_temp �����ւ��܂��B
//
	
	static char	szBuf[ 128 ];
	static	int	iLapIdx	= -1;
	int	i;
	
	BOOL	bInLap = FALSE;	// ���b�v�^�C���v����
	
#ifdef CIRCUIT_TOMO
	const int	iMeterR			= 50 * Img.w / 320;
	const int	iMeterCx		= iMeterR * 3 + 4;
	const int	iMeterCy		= ( int )( Img.h - iMeterR * 0.75 - 2 );
	const int	iMeterMinDeg	= 135;
	const int	iMeterMaxDeg	= 45;
	const int	iMeterMaxVal	= Log.m_piParamT[ TRACK_TACHO ] * 1000;
	const int	iMeterDegRange	= ( iMeterMaxDeg + 360 - iMeterMinDeg ) % 360;
	const int	iMeterScaleLen	= iMeterR / 8;
	
	const int	iMeterSCx		= iMeterR + 2;
	const int	iMeterSMaxVal	= Log.m_piParamT[ TRACK_SPEED ];
#else
	const int	iMeterR			= 50 * Img.w / 320;
	const int	iMeterCx		= Img.w - iMeterR - 2;
	const int	iMeterCy		= Img.h - iMeterR - 2;
	const int	iMeterMinDeg	= 135;
	const int	iMeterMaxDeg	= 45;
	const int	iMeterMaxVal	= 7000;
	const int	iMeterDegRange	= ( iMeterMaxDeg + 360 - iMeterMinDeg ) % 360;
	const int	iMeterScaleLen	= iMeterR / 8;
#endif
	
	// �t�H���g�T�C�Y������
	Img.InitFont();
	
	// ���O�ʒu�̌v�Z
	double	dLogNum = ( VideoEd == VideoSt ) ? -1 :
						( double )( LogEd - LogSt ) / ( VideoEd - VideoSt ) * ( Img.frame - VideoSt ) + LogSt;
	int		iLogNum = ( int )dLogNum;
	
	// �t���[���\��
	if( Log.m_piParamC[ CHECK_FRAME ] ){
		sprintf( szBuf, "V%6d/%6d", Img.frame, Img.frame_n - 1 );
		Img.DrawString( szBuf, COLOR_STR, COLOR_TIME_EDGE, 0, Img.w / 2, Img.h / 2 );
	#ifdef CIRCUIT_TOMO
		sprintf( szBuf, "L%.3f/%.3f", ( double )dLogNum / LOG_FREQ, Log.m_iVsdLogNum / LOG_FREQ );
	#else
		sprintf( szBuf, "L%6d/%6d", ( int )dLogNum, Log.m_iVsdLogNum - 1 );
	#endif
		Img.DrawString( szBuf, COLOR_STR, COLOR_TIME_EDGE, 0 );
	}
	
	/*** Lap �^�C���`�� ***/
	
	if( Log.IsHandLaptime() && DispLap && Log.m_bCalcLapTimeReq && Log.m_Lap ){
		Log.m_bCalcLapTimeReq = FALSE;
		#ifdef AVS_PLUGIN
			CalcLapTime();
		#else
			CalcLapTime( Img, Log );
		#endif
	}
	
	if( DispLap && Log.m_iLapNum ){
		/*
		Img.DrawRect(
			iLapX, iLapY, Img.w - 1, iLapY + Img.GetFontH() * 5 - 1,
			COLOR_PANEL,
			CVsdImg::IMG_ALFA | CVsdImg::IMG_FILL
		);
		*/
		
		if( Log.IsHandLaptime() ){
			// CIRCUIT_TOMO �ł� Log.m_Lap[].iLogNum �̓t���[��# �Ȃ̂�
			iLogNum = Img.frame;
		}
		
		// �J�����g�|�C���^�����������Ƃ��́C-1 �Ƀ��Z�b�g
		if(
			iLapIdx >= Log.m_iLapNum ||
			iLapIdx >= 0 && Log.m_Lap[ iLapIdx ].iLogNum > iLogNum
		) iLapIdx = -1;
		
		for( ; Log.m_Lap[ iLapIdx + 1 ].iLogNum <= iLogNum; ++iLapIdx );
		
		// ���ԕ\��
		if( iLapIdx >= 0 && Log.m_Lap[ iLapIdx + 1 ].fTime != 0 ){
			double dTime = Log.IsHandLaptime() ?
				( double )( Img.frame - Log.m_Lap[ iLapIdx ].iLogNum ) / Log.m_dVideoFPS :
				( dLogNum - Log.m_Lap[ iLapIdx ].iLogNum ) / LOG_FREQ;
			
			sprintf( szBuf, "Time%2d'%02d.%03d", ( int )dTime / 60, ( int )dTime % 60, ( int )( dTime * 1000 ) % 1000 );
			Img.DrawString( szBuf, COLOR_TIME, COLOR_TIME_EDGE, 0, Img.w - Img.GetFontW() * 13, 1 );
			bInLap = TRUE;
		}else{
			// �܂��J�n���Ă��Ȃ�
			Img.DrawString( "Time -'--.---", COLOR_TIME, COLOR_TIME_EDGE, 0, Img.w - Img.GetFontW() * 13, 1 );
		}
		
	#ifndef CIRCUIT_TOMO
		/*** �x�X�g�Ƃ̎Ԋԋ����\�� ***/
		if( !Log.IsHandLaptime() ){
			if( bInLap ){
				// ���̎��̑��s���������߂�
				double dMileage = GetVsdLog( fMileage ) - Log.m_VsdLog[ Log.m_Lap[ iLapIdx ].iLogNum ].fMileage;
				
				// �ő� Lap �́C���ꑖ�s�����ɂ�����^�C�� (=���O�ԍ�,����) �����߂�
				// iBestLogNum <= �ŏI�I�ɋ��߂錋�� < iBestLogNum + 1  �ƂȂ�
				static int iBestLogNum = 0;
				
				// iBestLogNum ����������������C���Z�b�g
				if(
					iBestLogNum < Log.m_iBestLapLogNum ||
					iBestLogNum >= Log.m_iVsdLogNum ||
					( Log.m_VsdLog[ iBestLogNum ].fMileage - Log.m_VsdLog[ Log.m_iBestLapLogNum ].fMileage ) > dMileage
				) iBestLogNum = Log.m_iBestLapLogNum;
				
				for(
					;
					( Log.m_VsdLog[ iBestLogNum + 1 ].fMileage - Log.m_VsdLog[ Log.m_iBestLapLogNum ].fMileage ) <= dMileage &&
					iBestLogNum < Log.m_iVsdLogNum;
					++iBestLogNum
				);
				
				// �ő� Lap �́C1/15�b�ȉ��̒l�����߂� = A / B
				double dBestLogNum =
					( double )iBestLogNum +
					// A: �ő����b�v�́C�ケ�ꂾ������Ȃ��� dMileage �Ɠ����ł͂Ȃ�
					( dMileage - ( Log.m_VsdLog[ iBestLogNum ].fMileage - Log.m_VsdLog[ Log.m_iBestLapLogNum ].fMileage )) /
					// B: �ő����b�v�́C1/15�b�̊Ԃɂ��̋����𑖂���
					( Log.m_VsdLog[ iBestLogNum + 1 ].fMileage - Log.m_VsdLog[ iBestLogNum ].fMileage );
				
				double dDiffTime =
					(
						( dLogNum - Log.m_Lap[ iLapIdx ].iLogNum ) -
						( dBestLogNum - Log.m_iBestLapLogNum )
					) / LOG_FREQ;
				
				if( -0.001 < dDiffTime && dDiffTime < 0.001 ) dDiffTime = 0;
				
				sprintf(
					szBuf, "    %c%d'%06.3f",
					dDiffTime <= 0 ? '-' : '+',
					( int )( fabs( dDiffTime )) / 60,
					fmod( fabs( dDiffTime ), 60 )
				);
				Img.DrawString( szBuf, dDiffTime <= 0 ? COLOR_DIFF_MINUS : COLOR_DIFF_PLUS, COLOR_TIME_EDGE, 0 );
			}else{
				Img.m_iPosY += Img.GetFontH();
			}
		}
	#endif
		
		Img.m_iPosY += Img.GetFontH() / 4;
		
		// Best �\��
		sprintf(
			szBuf, "Best%2d'%02d.%03d",
			( int )Log.m_fBestTime / 60,
			( int )Log.m_fBestTime % 60,
			( int )( Log.m_fBestTime * 1000 ) % 1000
		);
		Img.DrawString( szBuf, COLOR_TIME, COLOR_TIME_EDGE, 0 );
		
		// Lap�^�C���\��
		i = 0;
		for( int iLapIdxTmp = iLapIdx + 1; iLapIdxTmp >= 0 && i < 3; --iLapIdxTmp ){
			if( Log.m_Lap[ iLapIdxTmp ].fTime != 0 ){
				sprintf(
					szBuf, "%3d%c%2d'%02d.%03d",
					Log.m_Lap[ iLapIdxTmp ].uLap,
					( i == 0 && bInLap ) ? '*' : ' ',
					( int )Log.m_Lap[ iLapIdxTmp ].fTime / 60,
					( int )Log.m_Lap[ iLapIdxTmp ].fTime % 60,
					( int )( Log.m_Lap[ iLapIdxTmp ].fTime * 1000 ) % 1000
				);
				Img.DrawString( szBuf,
					Log.m_fBestTime == Log.m_Lap[ iLapIdxTmp ].fTime ? COLOR_BEST_LAP : COLOR_TIME,
					COLOR_TIME_EDGE, 0 );
				++i;
			}
		}
	}
	
	if( Log.m_iVsdLogNum == 0 ) return TRUE;
	
	/*** ���[�^�[�p�l�� ***/
	Img.DrawCircle(
		iMeterCx, iMeterCy, iMeterR, COLOR_PANEL,
		CVsdImg::IMG_ALFA | CVsdImg::IMG_FILL
	);
	
	// �^�R���[�^
	for( i = 0; i <= iMeterMaxVal; i += 500 ){
		int iDeg = iMeterDegRange * i / iMeterMaxVal + iMeterMinDeg;
		if( iDeg >= 360 ) iDeg -= 360;
		
		// ���[�^�[�p�l���ڐ���
		if( iMeterMaxVal <= 12000 || i % 1000 == 0 ){
			Img.DrawLine(
				( int )( cos( iDeg * ToRAD ) * iMeterR ) + iMeterCx,
				( int )( sin( iDeg * ToRAD ) * iMeterR ) + iMeterCy,
				( int )( cos( iDeg * ToRAD ) * ( iMeterR - iMeterScaleLen )) + iMeterCx,
				( int )( sin( iDeg * ToRAD ) * ( iMeterR - iMeterScaleLen )) + iMeterCy,
				( iMeterMaxVal <= 12000 && i % 1000 == 0 || i % 2000 == 0 ) ? 2 : 1,
				COLOR_SCALE, 0
			);
			
			// ���[�^�[�p�l���ڐ��萔�l
			if( iMeterMaxVal <= 12000 && i % 1000 == 0 || i % 2000 == 0 ){
				sprintf( szBuf, "%d", i / 1000 );
				Img.DrawString(
					szBuf,
					COLOR_STR, 0,
					( int )( cos( iDeg * ToRAD ) * iMeterR * .8 ) + iMeterCx - Img.GetFontW() / ( i >= 10000 ? 1 : 2 ),
					( int )( sin( iDeg * ToRAD ) * iMeterR * .8 ) + iMeterCy - Img.GetFontH() / 2
				);
			}
		}
	}
	
#ifdef CIRCUIT_TOMO
	// �X�s�[�h���[�^�[�p�l��
	Img.DrawCircle(
		iMeterSCx, iMeterCy, iMeterR, COLOR_PANEL,
		CVsdImg::IMG_ALFA | CVsdImg::IMG_FILL
	);
	
	int	iStep = (( iMeterSMaxVal / 20 ) + 4 ) / 5 * 5;
	
	for( i = 0; i <= iMeterSMaxVal; i += iStep ){
		int iDeg = iMeterDegRange * i / iMeterSMaxVal + iMeterMinDeg;
		if( iDeg >= 360 ) iDeg -= 360;
		
		// ���[�^�[�p�l���ڐ���
		if( i % iStep == 0 ){
			Img.DrawLine(
				( int )( cos( iDeg * ToRAD ) * iMeterR ) + iMeterSCx,
				( int )( sin( iDeg * ToRAD ) * iMeterR ) + iMeterCy,
				( int )( cos( iDeg * ToRAD ) * ( iMeterR - iMeterScaleLen )) + iMeterSCx,
				( int )( sin( iDeg * ToRAD ) * ( iMeterR - iMeterScaleLen )) + iMeterCy,
				( i % ( iStep * 2 ) == 0 ) ? 2 : 1,
				COLOR_SCALE, 0
			);
			
			// ���[�^�[�p�l���ڐ��萔�l
			if( i % ( iStep * 2 ) == 0 ){
				sprintf( szBuf, "%d", i );
				Img.DrawString(
					szBuf,
					COLOR_STR, 0,
					( int )( cos( iDeg * ToRAD ) * iMeterR * .75 ) + iMeterSCx - Img.GetFontW() * strlen( szBuf ) / 2,
					( int )( sin( iDeg * ToRAD ) * iMeterR * .75 ) + iMeterCy - Img.GetFontH() / 2
				);
			}
		}
	}
#endif // CIRCUIT_TOMO
	
	/*** ���[�^�[�`�� ***/
	
	if( dLogNum < 0 || dLogNum > Log.m_iVsdLogNum - 1 ) return TRUE;
	
	double	dSpeed	= GetVsdLog( fSpeed );
	double	dTacho	= GetVsdLog( fTacho );
	
#ifdef CIRCUIT_TOMO
	// �X�s�[�h�E�^�R�C��
	
	dSpeed *= 3600.0 / Log.m_piParamT[ TRACK_PULSE_SPEED ];
	dTacho *= 1000.0 * 60 / Log.m_piParamT[ TRACK_PULSE_TACHO ];
	
#else // CIRCUIT_TOMO
	// G �X�l�[�N
	int	iGx, iGy;
	
	if( DispGSnake ){
		
		int iGxPrev = 0, iGyPrev;
		
		for( i = -G_HIST; i <= 1 ; ++i ){
			
			if( iLogNum + i >= 0 ){
				// i == 1 ���͍Ō�̒��r���[�� LogNum
				iGx = iMeterCx + ( int )((( i != 1 ) ? Log.m_VsdLog[ iLogNum + i ].fGx : GetVsdLog( fGx )) * iMeterR / MAX_G_SCALE );
				iGy = iMeterCy - ( int )((( i != 1 ) ? Log.m_VsdLog[ iLogNum + i ].fGy : GetVsdLog( fGy )) * iMeterR / MAX_G_SCALE );
				
				if( iGxPrev ) Img.DrawLine(
					iGx, iGy, iGxPrev, iGyPrev,
					LINE_WIDTH, COLOR_G_HIST, 0
				);
				
				iGxPrev = iGx;
				iGyPrev = iGy;
			}
		}
	}else{
		iGx = iMeterCx + ( int )( GetVsdLog( fGx ) * iMeterR / MAX_G_SCALE );
		iGy = iMeterCy - ( int )( GetVsdLog( fGy ) * iMeterR / MAX_G_SCALE );
	}
	
	// G �C���W�P�[�^
	Img.DrawCircle(
		iGx, iGy, iMeterR / 20,
		COLOR_G_SENSOR, CVsdImg::IMG_FILL
	);
	
	// MAP �\��
	if( LineTrace ){
		double dGx, dGy;
		
		int iGxPrev = INVALID_POS_I, iGyPrev;
		
		int	iLineSt = iLapIdx >= 0 ? Log.m_Lap[ iLapIdx ].iLogNum : 0;
		if( iLogNum - iLineSt > ( int )( LineTrace * LOG_FREQ ))
			iLineSt = iLogNum - ( int )( LineTrace * LOG_FREQ );
		
		int iLineEd = iLapIdx != Log.m_iLapNum - 1 ? Log.m_Lap[ iLapIdx + 1 ].iLogNum : Log.m_iVsdLogNum - 1;
		if( iLineEd - iLogNum > ( int )( LineTrace * LOG_FREQ ))
			iLineEd = iLogNum + ( int )( LineTrace * LOG_FREQ );
		
		for( i = iLineSt; i <= iLineEd ; ++i ){
			#define GetMapPos( p, a ) ( (( p ) - Log.m_dMapOffs ## a ) / Log.m_dMapSize * MAX_MAP_SIZE + 8 )
			dGx = GetMapPos( Log.m_VsdLog[ i ].fX, X );
			dGy = GetMapPos( Log.m_VsdLog[ i ].fY, Y );
			
			if( !_isnan( dGx )){
				iGx = ( int )dGx;
				iGy = ( int )dGy;
				
				if( iGxPrev != INVALID_POS_I ){
					// Line �̐F�p�� G �����߂�
					double dG = sqrt(
						Log.m_VsdLog[ i ].fGx * Log.m_VsdLog[ i ].fGx +
						Log.m_VsdLog[ i ].fGy * Log.m_VsdLog[ i ].fGy
					) / MAP_G_MAX;
					
					PIXEL_YC yc_line;
					
					if( dG < 0.5 ){
						dG *= 2;
						yc_line.y  = ( short )( MAP_LINE2.y  * dG + MAP_LINE1.y  * ( 1 - dG ));
						yc_line.cb = ( short )( MAP_LINE2.cb * dG + MAP_LINE1.cb * ( 1 - dG ));
						yc_line.cr = ( short )( MAP_LINE2.cr * dG + MAP_LINE1.cr * ( 1 - dG ));
					}else if( dG < 1.0 ){
						dG = ( dG - 0.5 ) * 2;
						yc_line.y  = ( short )( MAP_LINE3.y  * dG + MAP_LINE2.y  * ( 1 - dG ));
						yc_line.cb = ( short )( MAP_LINE3.cb * dG + MAP_LINE2.cb * ( 1 - dG ));
						yc_line.cr = ( short )( MAP_LINE3.cr * dG + MAP_LINE2.cr * ( 1 - dG ));
					}else{
						yc_line = MAP_LINE3;
					}
					
					// Line ������
					Img.DrawLine(
						iGx, iGy, iGxPrev, iGyPrev,
						LINE_WIDTH, yc_line, 0
					);
				}
			}else{
				iGx = INVALID_POS_I;
			}
			
			iGxPrev = iGx;
			iGyPrev = iGy;
		}
		
		// MAP �C���W�P�[�^ (����)
		dGx = GetMapPos( GetVsdLog( fX ), X );
		dGy = GetMapPos( GetVsdLog( fY ), Y );
		
		if( !_isnan( dGx )) Img.DrawCircle(
			( int )dGx, ( int )dGy, iMeterR / 20,
			COLOR_CURRENT_POS, CVsdImg::IMG_FILL
		);
	}
#endif // CIRCUIT_TOMO
	
	// �X�s�[�h / �M�A
	UINT uGear = 0;
	
	if( dTacho ){
	#ifdef CIRCUIT_TOMO
		UINT	u;
		uGear = 6;
		for( u = 1; u <= 5; ++u ){
			if(
				( Log.m_piParamT[ TRACK_GEAR1 + u - 1 ] + Log.m_piParamT[ TRACK_GEAR1 + u ] ) / 20000.0 >
				dSpeed / 3600 * 1000 * 60 / dTacho
			){
				uGear = u;
				break;
			}
		}
	#else
		UINT uGearRatio = ( int )( dSpeed * 100 * ( 1 << 8 ) / dTacho );
		
		if     ( uGearRatio < GEAR_TH( 1 ))	uGear = 1;
		else if( uGearRatio < GEAR_TH( 2 ))	uGear = 2;
		else if( uGearRatio < GEAR_TH( 3 ))	uGear = 3;
		else if( uGearRatio < GEAR_TH( 4 ))	uGear = 4;
		else								uGear = 5;
	#endif
	}
	
	// �X�s�[�h�\��
	sprintf( szBuf, "%d\x7F", uGear );
	Img.DrawString(
		szBuf,
		COLOR_STR, 0,
		iMeterCx - 1 * Img.GetFontW(), iMeterCy - iMeterR / 2
	);
	
	sprintf( szBuf, "%3d\x80\x81", ( int )dSpeed );
	Img.DrawString(
		szBuf,
		COLOR_STR, 0,
		iMeterCx - 3 * Img.GetFontW(), iMeterCy + iMeterR / 2
	);
	
#ifndef CIRCUIT_TOMO
	sprintf( szBuf, "%2d\x82", ( int )( sqrt( GetVsdLog( fGx ) * GetVsdLog( fGx ) + GetVsdLog( fGy ) * GetVsdLog( fGy )) * 10 ));
	Img.DrawString(
		szBuf,
		COLOR_STR, 0,
		iMeterCx - 2 * Img.GetFontW(), iMeterCy + iMeterR / 2 - Img.GetFontH()
	);
#else
	// Speed �̐j
	double dSpeedNeedle =
		iMeterDegRange / ( double )iMeterSMaxVal * dSpeed + iMeterMinDeg;
	if( dSpeedNeedle >= 360 ) dSpeedNeedle -= 360;
	dSpeedNeedle = dSpeedNeedle * ToRAD;
	
	Img.DrawLine(
		iMeterSCx, iMeterCy,
		( int )( cos( dSpeedNeedle ) * iMeterR * 0.95 + .5 ) + iMeterSCx,
		( int )( sin( dSpeedNeedle ) * iMeterR * 0.95 + .5 ) + iMeterCy,
		LINE_WIDTH, COLOR_NEEDLE, 0
	);
	
	Img.DrawCircle( iMeterSCx, iMeterCy,  iMeterR / 25, COLOR_NEEDLE, CVsdImg::IMG_FILL );
#endif
	
	// Tacho �̐j
	double dTachoNeedle = iMeterDegRange / ( double )iMeterMaxVal * dTacho + iMeterMinDeg;
	if( dTachoNeedle >= 360 ) dTachoNeedle -= 360;
	dTachoNeedle = dTachoNeedle * ToRAD;
	
	Img.DrawLine(
		iMeterCx, iMeterCy,
		( int )( cos( dTachoNeedle ) * iMeterR * 0.95 + .5 ) + iMeterCx,
		( int )( sin( dTachoNeedle ) * iMeterR * 0.95 + .5 ) + iMeterCy,
		LINE_WIDTH, COLOR_NEEDLE, 0
	);
	
	Img.DrawCircle( iMeterCx, iMeterCy,  iMeterR / 25, COLOR_NEEDLE, CVsdImg::IMG_FILL );
	
	return TRUE;
}

/*** func_proc **************************************************************/

BOOL func_proc( FILTER *fp,FILTER_PROC_INFO *fpip ){
//
//	fp->track[n]			: �g���b�N�o�[�̐��l
//	fp->check[n]			: �`�F�b�N�{�b�N�X�̐��l
//	fpip->w 				: ���ۂ̉摜�̉���
//	fpip->h 				: ���ۂ̉摜�̏c��
//	fpip->w					: �摜�̈�̉���
//	fpip->h					: �摜�̈�̏c��
//	fpip->ycp_edit			: �摜�̈�ւ̃|�C���^
//	fpip->ycp_temp			: �e���|�����̈�ւ̃|�C���^
//	fpip->ycp_edit[n].y		: ��f(�P�x    )�f�[�^ (     0 �` 4095 )
//	fpip->ycp_edit[n].cb	: ��f(�F��(��))�f�[�^ ( -2048 �` 2047 )
//	fpip->ycp_edit[n].cr	: ��f(�F��(��))�f�[�^ ( -2048 �` 2047 )
//
//  ��f�f�[�^�͔͈͊O�ɏo�Ă��邱�Ƃ�����܂��B
//  �܂��͈͓��Ɏ��߂Ȃ��Ă����܂��܂���B
//
//	�摜�T�C�Y��ς������Ƃ��� fpip->w �� fpip->h ��ς��܂��B
//
//	�e���|�����̈�ɏ��������摜���i�[�������Ƃ���
//	fpip->ycp_edit �� fpip->ycp_temp �����ւ��܂��B
//
	
	// �N���X�ɕϊ�
	CVsdImg	&Img = *( CVsdImg *)fpip;
	
	return DrawVSD( Img, *g_VsdLog );
}

/*** WndProc ****************************************************************/

BOOL func_WndProc( HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *filter ){
	
	TCHAR	szBuf[ BUF_SIZE ];
	int		iFrame;
	
	//	TRUE��Ԃ��ƑS�̂��ĕ`�悳���
	
	//	�ҏW���łȂ���Ή������Ȃ�
	if( filter->exfunc->is_editing( editp ) != TRUE ) return FALSE;
	
	switch( message ) {
	  case WM_FILTER_IMPORT:
		if( filter->exfunc->dlg_get_load_name( szBuf, FILE_EXT, NULL )){
			
			g_VsdLog->ReadLog( szBuf );
			
			// trackbar �ݒ�
			track_e[ TRACK_LSt ] =
			track_e[ TRACK_LEd ] =
				#ifdef CIRCUIT_TOMO
					( int )( g_VsdLog->m_iVsdLogNum / LOG_FREQ + 1 );
				#else
					( g_VsdLog->m_iVsdLogNum + 99 ) / 100;
				#endif
			
			// �ݒ�ĕ`��
			filter->exfunc->filter_window_update( filter );
			
			#ifndef CIRCUIT_TOMO
				// log pos �X�V
				func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
			#endif
		}
		return TRUE;
		
	  Case WM_FILTER_EXPORT:
		if( filter->exfunc->dlg_get_save_name( szBuf, FILE_CFG_EXT, NULL ))
			return g_VsdLog->ConfigSave( szBuf );
		
	  Case WM_FILTER_FILE_OPEN:
		
		g_VsdLog = new CVsdLog;
		
		g_VsdLog->m_piParamT	= filter->track;
		g_VsdLog->m_piParamC	= filter->check;
		g_VsdLog->filter		= filter;
		g_VsdLog->editp			= editp;
		
		// fps �擾
		FILE_INFO fi;
		filter->exfunc->get_file_info( editp, &fi );
		g_VsdLog->m_dVideoFPS  = ( double )fi.video_rate / fi.video_scale;
		
		g_VsdLog->m_bCalcLapTimeReq = TRUE;
		
		// trackbar �ݒ�
		track_e[ TRACK_VSt ] =
		track_e[ TRACK_VEd ] = ( filter->exfunc->get_frame_n( editp ) + 99 ) / 100;
		
		// �ݒ�ĕ`��
		filter->exfunc->filter_window_update( filter );
		
	  Case WM_FILTER_FILE_CLOSE:
		delete g_VsdLog;
		g_VsdLog = NULL;
		
	  Case WM_FILTER_MAIN_KEY_DOWN:
		switch( wparam ){
			
		  case 'M':
			// �}�[�N
			FRAME_STATUS	fsp;
			iFrame = filter->exfunc->get_frame( editp );
			
			filter->exfunc->get_frame_status( editp, iFrame, &fsp );
			fsp.edit_flag ^= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
			filter->exfunc->set_frame_status( editp, iFrame, &fsp );
			
			g_VsdLog->m_bCalcLapTimeReq = TRUE;
			return TRUE;
			
		  Case 'S':
			// �t���[�����Z�b�g
			iFrame = filter->exfunc->get_frame( editp );
			
			filter->track[ TRACK_VSt ]	= filter->track[ TRACK_VEd ];
			filter->track[ TRACK_VSt2 ]	= filter->track[ TRACK_VEd2 ];
			
			filter->track[ TRACK_VEd ]	= iFrame / 100;
			filter->track[ TRACK_VEd2 ]	= iFrame % 100;
			
			// �ݒ�ĕ`��
			filter->exfunc->filter_window_update( filter );
		}
	}
	
	return FALSE;
}

/*** �X���C�_�o�[�A�� *******************************************************/

BOOL func_update( FILTER *fp, int status ){
	static	BOOL	bReEnter = FALSE;
	
	if( bReEnter ) return TRUE;
	
	bReEnter = TRUE;
	
#ifndef CIRCUIT_TOMO
	if(
		status == ( FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS ) &&
		fp->check[ CHECK_LOGPOS ]
	){
		fp->track[ TRACK_LSt  ] = g_VsdLog->m_iLogStart / 100;
		fp->track[ TRACK_LSt2 ] = g_VsdLog->m_iLogStart % 100;
		fp->track[ TRACK_LEd  ] = g_VsdLog->m_iLogStop  / 100;
		fp->track[ TRACK_LEd2 ] = g_VsdLog->m_iLogStop  % 100;
		
		// �ݒ�ĕ`��
		fp->exfunc->filter_window_update( fp );
	}
	
	// �}�b�v��]
	if( status == ( FILTER_UPDATE_STATUS_TRACK + TRACK_MapAngle )){
		g_VsdLog->RotateMap();
	}
#endif
	
	bReEnter = FALSE;
	
	return TRUE;
}

/****************************************************************************/

BOOL WINAPI DllMain(
	HINSTANCE	hinstDLL,	// handle to DLL module
	DWORD		fdwReason,	// reason for calling function
	LPVOID		lpvReserved	// reserved
){
	if( fdwReason == DLL_PROCESS_ATTACH ){
		g_hInst = hinstDLL;
	}
	return TRUE;
}
