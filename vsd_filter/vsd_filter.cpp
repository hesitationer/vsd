/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter.cpp - VSD filter for AviUti
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "../vsd/main.h"
#include "filter.h"
#include "dds_lib/dds_lib.h"
#include "CVsdLog.h"
#include "CVsdFont.h"
#include "CScript.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"

/*** macros *****************************************************************/

#ifdef GPS_ONLY
	#define CONFIG_EXT		"cfg"
#else
	#define CONFIG_EXT		"avs"
#endif

#define	FILE_LOG_EXT	"log file (*.log)\0*.log; *.gz\0AllFile (*.*)\0*.*\0"
#define	FILE_GPS_EXT	"GPS file (*.nme* *.dp3*)\0*.nme*; *.dp3*; *.gz\0AllFile (*.*)\0*.*\0"
#define	FILE_CFG_EXT	"Config File (*." CONFIG_EXT ")\0*." CONFIG_EXT "\0AllFile (*.*)\0*.*\0"
#define	FILE_SKIN_EXT	"Skin File (*.js)\0*.js\0AllFile (*.*)\0*.*\0"

#define PROG_NAME		"VSD���[�^�[����"
#define PROG_VERSION	"v1.10beta1"

/*** new type ***************************************************************/

/*** const ******************************************************************/

enum {
	ID_BUTT_SET_VSt = 0xFF00,
	ID_BUTT_SET_VEd,
#ifndef GPS_ONLY
	ID_BUTT_SET_LSt,
	ID_BUTT_SET_LEd,
#endif
	ID_BUTT_SET_GSt,
	ID_BUTT_SET_GEd,
#ifndef GPS_ONLY
	ID_EDIT_LOAD_LOG,
	ID_BUTT_LOAD_LOG,
#endif
	ID_EDIT_LOAD_GPS,
	ID_BUTT_LOAD_GPS,
	ID_EDIT_SEL_SKIN,
	ID_BUTT_SEL_SKIN,
	ID_BUTT_LOAD_CFG,
	ID_BUTT_SAVE_CFG,
};

#define POS_TH_LABEL			30
#define POS_TH_SLIDER			220
#define POS_TH_EDIT				294
#define POS_ADD_LABEL			50
#define POS_ADD_SLIDER			300
#define POS_ADD_EDIT			16
#define POS_CHK_LABEL_L			36
#define POS_CHK_LABEL_R			186

#ifdef GPS_ONLY
	#define POS_SET_BUTT_SIZE		0
#else
	#define POS_SET_BUTT_SIZE		30
#endif

#define POS_FILE_CAPTION_SIZE	70
#define POS_FILE_NAME_SIZE		( rectClient.right - ( POS_FILE_CAPTION_POS + POS_FILE_CAPTION_SIZE + POS_FILE_BUTT_SIZE ))
#define POS_FILE_BUTT_SIZE		40
#define POS_FILE_CAPTION_POS	150
#define POS_FILE_HEIGHT			21
#define POS_FILE_HEIGHT_MARGIN	2

#ifdef GPS_ONLY
	#define POS_FILE_NUM	3
#else
	#define POS_FILE_NUM	4
#endif

#define INVALID_INT		0x80000000

/*** gloval var *************************************************************/

HINSTANCE		g_hInst 	= NULL;

/****************************************************************************/
//---------------------------------------------------------------------
//		�t�B���^�\���̒�`
//---------------------------------------------------------------------

// �g���b�N�o�[�̖��O
TCHAR	*track_name[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	name,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̏����l
int		track_default[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	init,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̉����l
int		track_s[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	min,
	#include "def_trackbar.h"
};
// �g���b�N�o�[�̏���l
int		track_e[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name )	max,
	#include "def_trackbar.h"
};

// �`�F�b�N�{�b�N�X�̖��O
TCHAR	*check_name[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name )	name,
	#include "def_checkbox.h"
};

// �`�F�b�N�{�b�N�X�̏����l (�l��0��1)
int		check_default[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name )	init,
	#include "def_checkbox.h"
};

// �V���h�D�̏����l
int		shadow_default[] = {
	#define DEF_SHADOW( id, init, conf_name )	init,
	#include "def_shadow.h"
};

int		shadow_param[] = {
	#define DEF_SHADOW( id, init, conf_name )	init,
	#include "def_shadow.h"
};

char g_szDescription[] = PROG_NAME " " PROG_VERSION;

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_MAIN_MESSAGE | FILTER_FLAG_EX_INFORMATION,
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
	PROG_NAME,					//	�t�B���^�̖��O
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
	g_szDescription,			//  �t�B���^���ւ̃|�C���^ (FILTER_FLAG_EX_INFORMATION�������Ă��鎞�ɗL��)
	func_save_start,			//	�Z�[�u���J�n����钼�O�ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
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

/*** CVsdFilterAvu �N���X ***************************************************/

class CVsdFilterAvu : public CVsdFilter {
	
  public:
	void				*editp;
	FILTER				*filter;
	FILE_INFO			*fileinfo;
	FILTER_PROC_INFO	*fpip;
	
	CVsdFilterAvu( FILTER *filter, void *editp );
	~CVsdFilterAvu();
	
	int GetIndex( int x, int y ){ return fpip->max_w * y + x; }
	BOOL ConfigSave( const char *szFileName );
	
#ifndef GPS_ONLY
	BOOL ReadLog( const char *szFileName, HWND hwnd );
#endif
	BOOL GPSLogLoad( const char *szFileName, HWND hwnd );
	
	// ���z�֐�
	virtual void PutPixelLow( int x, int y, const PIXEL_YCA &yc, UINT uFlag );
	virtual void FillLineLow( int x1, int y1, int x2, const PIXEL_YCA &yc, UINT uFlag );
	virtual void PutImage( int x, int y, CVsdImage &img );
	
	virtual int	GetWidth( void ){ return fpip->w; }
	virtual int	GetHeight( void ){ return fpip->h; }
	virtual int	GetFrameMax( void ){ return fileinfo->frame_n; }
	virtual int	GetFrameCnt( void ){ return fpip->frame; }
	virtual double	GetFPS( void ){ return ( double )fileinfo->video_rate / fileinfo->video_scale; }
	
	virtual void SetFrameMark( int iFrame );
	virtual int  GetFrameMark( int iFrame );
	
	// 1�X���C�_�����p�p�����[�^
	int	m_iAdjustPointNum;
	int	m_iAdjustPointVid[ 2 ];
	int	m_iAdjustPointGPS[ 2 ];
};

CVsdFilterAvu	*g_Vsd;

/*** �R���X�g���N�^�^�f�X�g���N�^ *******************************************/

CVsdFilterAvu::CVsdFilterAvu( FILTER *filter, void *editp ) :
	filter( filter ), editp( editp )
{
	fileinfo = new FILE_INFO;
	filter->exfunc->get_file_info( editp, fileinfo );
	
	m_piParamT	= filter->track;
	m_piParamC	= filter->check;
	m_piParamS	= shadow_param;
	
#ifdef GPS_ONLY
	VideoSt = VideoEd = VideoSt = VideoEd =
#endif
	m_iAdjustPointNum = 0;
	m_iAdjustPointVid[ 0 ] =
	m_iAdjustPointVid[ 1 ] = INVALID_INT;
}

CVsdFilterAvu::~CVsdFilterAvu(){
	delete fileinfo;
}

/*** PutPixel ***************************************************************/

/* �ϊ���
Y  =  0.299R+0.587G+0.114B
Cr =  0.500R-0.419G-0.081B
Cb = -0.169R-0.332G+0.500B

R = Y+1.402Cr 
G = Y-0.714Cr-0.344Cb 
B = Y+1.772Cb 
*/

/*
inline void CVsdFilter::PutPixel( int x, int y, short iY, short iCr, short iCb ){
	int	iIndex = GetIndex( x, y );
	
	fpip->ycp_edit[ iIndex ].y  = iY;
	fpip->ycp_edit[ iIndex ].cr = iCr;
	fpip->ycp_edit[ iIndex ].cb = iCb;
}
*/

inline void CVsdFilterAvu::PutPixelLow( int x, int y, const PIXEL_YCA &yc, UINT uFlag ){
	
	PIXEL_YC	*ycp = fpip->ycp_edit;
	
	int	iIndex = GetIndex( x, y );
	if( yc.alfa ){
		int iAlfa = ( int )yc.alfa;
		
		ycp[ iIndex ].y  = ( PIXEL_t )( yc.y  + (( ycp[ iIndex ].y  * iAlfa ) >> 8 ));
		ycp[ iIndex ].cr = ( PIXEL_t )( yc.cr + (( ycp[ iIndex ].cr * iAlfa ) >> 8 ));
		ycp[ iIndex ].cb = ( PIXEL_t )( yc.cb + (( ycp[ iIndex ].cb * iAlfa ) >> 8 ));
	}else{
		ycp[ iIndex ] = ( PIXEL_YC &)yc;
	}
}

inline void CVsdFilterAvu::FillLineLow( int x1, int y1, int x2, const PIXEL_YCA &yc, UINT uFlag ){
	
	PIXEL_YC	*ycp = fpip->ycp_edit;
	
	x1 = GetIndex( x1, y1 );
	x2 = GetIndex( x2, y1 );
	
	int iIndex;
	if( yc.alfa ){
		int iAlfa = ( int )yc.alfa;
		
		for( iIndex = x1; iIndex <= x2; ++iIndex ){
			ycp[ iIndex ].y  = ( PIXEL_t )( yc.y  + (( ycp[ iIndex ].y  * iAlfa ) >> 8 ));
			ycp[ iIndex ].cr = ( PIXEL_t )( yc.cr + (( ycp[ iIndex ].cr * iAlfa ) >> 8 ));
			ycp[ iIndex ].cb = ( PIXEL_t )( yc.cb + (( ycp[ iIndex ].cb * iAlfa ) >> 8 ));
		}
	}else{
		for( iIndex = x1; iIndex <= x2; ++iIndex ){
			ycp[ iIndex ] = ( PIXEL_YC &)yc;
		}
	}
}

/*** PutImage ***************************************************************/

void CVsdFilterAvu::PutImage(	// !export
	int x, int y, CVsdImage &img
){
	img.ConvRGBA2YCA();
	
	x &= ~1;	// 2�P��
	
	PIXEL_YC	*ycp = fpip->ycp_edit;
	
	for( int y1 = 0; y1 < img.m_iHeight; ++y1 ){
		for( int x1 = 0; x1 < img.m_iWidth; ++x1 ){
			
			PIXEL_YCA &yc = img.m_pPixelBuf[ x1 + y1 * img.m_iWidth ];
			
			if( yc.alfa != 256 ){
				int	iIndex = GetIndex( x + x1, y + y1 );
				
				if( yc.alfa ){
					int iAlfa = ( int )yc.alfa;
					
					ycp[ iIndex ].y  = ( PIXEL_t )( yc.y  + (( ycp[ iIndex ].y  * iAlfa ) >> 8 ));
					ycp[ iIndex ].cr = ( PIXEL_t )( yc.cr + (( ycp[ iIndex ].cr * iAlfa ) >> 8 ));
					ycp[ iIndex ].cb = ( PIXEL_t )( yc.cb + (( ycp[ iIndex ].cb * iAlfa ) >> 8 ));
				}else{
					ycp[ iIndex ] = ( PIXEL_YC &)yc;
				}
			}
		}
	}
}

/*** �t���[�����}�[�N *******************************************************/

void CVsdFilterAvu::SetFrameMark( int iFrame ){
	FRAME_STATUS	fsp;
	
	filter->exfunc->get_frame_status( editp, iFrame, &fsp );
	fsp.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
	filter->exfunc->set_frame_status( editp, iFrame, &fsp );
}

int CVsdFilterAvu::GetFrameMark( int iFrame ){
	
	FRAME_STATUS	fsp;
	
	for( ; iFrame < GetFrameMax(); ++iFrame ){
		filter->exfunc->get_frame_status( editp, iFrame, &fsp );
		
		if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
			return iFrame;
		}
	}
	return -1;
}

/*** config �Z�[�u **********************************************************/

enum {
	#define DEF_STR_PARAM( id, var, init, conf_name ) id,
	#include "def_str_param.h"
};

BOOL CVsdFilterAvu::ConfigSave( const char *szFileName ){
	FILE	*fp;
	int		i;
	UINT	uStrParamFlag = 0;
	
	if(( fp = fopen( szFileName, "w" )) == NULL ) return FALSE;
	
	fprintf( fp,
		"DirectShowSource( \"%s\", pixel_type=\"YUY2\", fps=%d.0/%d )\n"
		"VSDFilter",
		fileinfo->name,
		fileinfo->video_rate, fileinfo->video_scale
	);
	
	char cSep = '(';
	
	// str param �ɏ����l�ݒ�
	#define DEF_STR_PARAM( id, var, init, conf_name ) \
		if( strcmp( var, init ) != 0 ){ \
			fprintf( fp, "%c \\\n\t" conf_name "=\"%s\"", cSep, var ); \
			cSep = ','; \
			uStrParamFlag |= 1 << id; \
		}
	#include "def_str_param.h"
	
	for( i = 0; i < TRACK_N; ++i ){
		if(
			m_szTrackbarName[ i ] == NULL ||
			i >= TRACK_LineTrace && m_piParamT[ i ] == track_default[ i ] ||
			#ifndef GPS_ONLY
				( i == PARAM_LSt || i == PARAM_LEd ) && !( uStrParamFlag & ( 1 << STRPARAM_LOGFILE )) ||
			#endif
			( i == PARAM_GSt || i == PARAM_GEd ) && !( uStrParamFlag & ( 1 << STRPARAM_GPSFILE ))
		) continue;
		
		fprintf(
			fp, "%c \\\n\t%s=%d", cSep, m_szTrackbarName[ i ], m_piParamT[ i ]
		);
		cSep = ',';
	}
	
	for( i = 0; i < CHECK_N; ++i ){
		if(
			m_szCheckboxName[ i ] == NULL ||
			m_piParamC[ i ] == check_default[ i ]
		) continue;
		
		fprintf(
			fp, ", \\\n\t%s=%d", m_szCheckboxName[ i ], m_piParamC[ i ]
		);
	}
	
	for( i = 0; i < SHADOW_N; ++i ){
		if(
			m_piParamS[ i ] == shadow_default[ i ]
		) continue;
		
		fprintf(
			fp, ", \\\n\t%s=%d", m_szShadowParamName[ i ], m_piParamS[ i ]
		);
	}
	
	// �蓮���b�v�v���}�[�N�o��
	if( m_iLapMode != LAPMODE_MAGNET && m_iLapNum ){
		FRAME_STATUS	fsp;
		BOOL			bFirst = TRUE;
		
		// �}�[�N����Ă���t���[��# �����߂�
		for( i = 0; i < GetFrameMax(); ++i ){
			filter->exfunc->get_frame_status( editp, i, &fsp );
			if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
				fprintf( fp, "%s%u", bFirst ? ", \\\n\tmark=\"" : ",", i );
				bFirst = FALSE;
			}
		}
		fputc( '"', fp );
	}
	
	fprintf( fp, " \\\n)\n"
	#ifndef GPS_ONLY
		"# Amplify( 0.2 )\n"
	#endif
	);
	
	fclose( fp );
	return TRUE;
}

/*** ���O���[�h *************************************************************/

#ifndef GPS_ONLY
BOOL CVsdFilterAvu::ReadLog( const char *szFileName, HWND hwnd ){
	
	char szMsg[ BUF_SIZE ];
	
	if( !CVsdFilter::ReadLog( szFileName )){
		sprintf( szMsg, "�t�@�C�������[�h�ł��܂���\n%s", szFileName );
		MessageBox( NULL,
			szMsg,
			"VSD",
			MB_OK | MB_ICONWARNING
		);
		return FALSE;
	}
	
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_LOG ), szFileName );
	
	// trackbar �ݒ�
	track_e[ PARAM_LSt ] =
	track_e[ PARAM_LEd ] = m_VsdLog->m_iCnt;
	
	return TRUE;
}
#endif

/*** GPS ���O���[�h ********************************************************/

BOOL CVsdFilterAvu::GPSLogLoad( const char *szFileName, HWND hwnd ){
	
	char szMsg[ BUF_SIZE ];
	
	if( !CVsdFilter::GPSLogLoad( szFileName )){
		sprintf( szMsg, "�t�@�C�������[�h�ł��܂���\n%s", szFileName );
		MessageBox( NULL,
			szMsg,
			"VSD",
			MB_OK | MB_ICONWARNING
		);
		return FALSE;
	}
	
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_GPS ), szFileName );
	
	// trackbar �ݒ�
	#ifdef GPS_ONLY
		if( GPSSt == INVALID_INT ){
			GPSSt	= 0;
			GPSEd	= m_GPSLog->m_iCnt;
		}
	#else
		track_e[ PARAM_GSt ] =
		track_e[ PARAM_GEd ] = m_GPSLog->m_iCnt;
	#endif

	return TRUE;
}

/*** func_proc **************************************************************/

BOOL func_proc( FILTER *fp, FILTER_PROC_INFO *fpip ){
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
	
	if( !g_Vsd ) return 0;
	// �N���X�ɕϊ�
	g_Vsd->fpip		= fpip;
	
	return g_Vsd->DrawVSD();
}

/*** �_�C�A���O�T�C�Y�g���ƃp�[�c�ǉ� ***************************************/

void CreateSubControl(
	HWND hwnd, int &iID, HFONT hfont, int iX, int &iY, RECT rectClient,
	char *szCap, char *szEdit, char *szButt
) {
	
	HWND hwndChild;
	
	hwndChild = CreateWindow(
		"STATIC", szCap, WS_CHILD | WS_VISIBLE,
		iX, iY,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"EDIT", szEdit, WS_CHILD | WS_VISIBLE | ES_READONLY,
		iX + POS_FILE_CAPTION_SIZE, iY,
		POS_FILE_NAME_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )iID, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", szButt, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		iX + POS_FILE_CAPTION_SIZE + POS_FILE_NAME_SIZE, iY,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( iID + 1 ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	iID += 2;
	iY += POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN;
}

void ExtendDialog( HWND hwnd ){
	
	union {
		struct {
			POINT	topleft, bottomright;
		} points;
		RECT	rect;
	} rect;
	RECT	rectClient;
	
	HWND	hwndChild	= NULL;
	HFONT	hfont		= NULL;
	
	// �N���C�A���g�̈�̃T�C�Y get ��Ƀ��T�C�Y
	GetWindowRect( hwnd, &rectClient );
	MoveWindow( hwnd,
		rectClient.left, rectClient.top,
		rectClient.right  - rectClient.left + POS_ADD_LABEL + POS_ADD_SLIDER + POS_ADD_EDIT + POS_SET_BUTT_SIZE,
		rectClient.bottom - rectClient.top,
		TRUE
	);
	
	GetClientRect( hwnd, &rectClient );
	
	while( 1 ){
		/*** �q�p�[�c�̃T�C�Y�ύX ***/
		hwndChild = FindWindowEx( hwnd, hwndChild, NULL, NULL );
		if( !hwndChild ) break;
		
		// screen -> client ���W�ɕϊ�
		GetWindowRect( hwndChild, &rect.rect );
		ScreenToClient( hwnd, &rect.points.topleft );
		ScreenToClient( hwnd, &rect.points.bottomright );
		
		// �_�C�A���O���������΂��CEDIT �{�b�N�X�̃T�C�Y��L�΂�
		#define ResizeControl( name ) { \
			if( rect.rect.right >= POS_TH_ ## name ) rect.rect.right += POS_ADD_ ## name; \
			if( rect.rect.left  >= POS_TH_ ## name ) rect.rect.left  += POS_ADD_ ## name; \
		}
		
		// �E���珇�Ԃɏ�������
		ResizeControl( EDIT );
		ResizeControl( SLIDER );
		// �`�F�b�N�{�b�N�X�̃��x������
		if( rect.rect.left < POS_CHK_LABEL_L && POS_CHK_LABEL_R < rect.rect.right ){
			rect.rect.right = POS_FILE_CAPTION_POS;
		}else{
			ResizeControl( LABEL );
		}
		
		// ���ۂɃ��T�C�Y
		MoveWindow( hwndChild,
			rect.rect.left,
			rect.rect.top,
			rect.rect.right  - rect.rect.left,
			rect.rect.bottom - rect.rect.top,
			TRUE
		);
		
		if( !hfont ) hfont = ( HFONT )SendMessage( hwndChild, WM_GETFONT, 0, 0 );
	}
	
	// �ʒu�擾�{�^��
	int i;
	#ifndef GPS_ONLY
		for( i = 0; i <= ( ID_BUTT_SET_GEd - ID_BUTT_SET_VSt ); ++i ){
			hwndChild = CreateWindow(
				"BUTTON", "set", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				rectClient.right - POS_SET_BUTT_SIZE, 14 + ( i + 1 ) * 24,
				POS_SET_BUTT_SIZE, 16,
				hwnd, ( HMENU )( ID_BUTT_SET_VSt + i ), 0, NULL
			);
			SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
		}
	#endif
	
	// ���O���E�t�H���g��
	i = ID_BUTT_SET_GEd + 1;
	int y = rectClient.bottom - ( POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN ) * POS_FILE_NUM + POS_FILE_HEIGHT_MARGIN;
	
#ifndef GPS_ONLY
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "VSD���O",	"",		"�J��" );
#endif
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "GPS���O",	"",		"�J��" );
	CreateSubControl( hwnd, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "�X�L��",	"",		"�J��" );
	
	// cfg load/save �{�^��
	hwndChild = CreateWindow(
		"STATIC", "cfg�t�@�C��", WS_CHILD | WS_VISIBLE,
		rectClient.right - ( POS_FILE_BUTT_SIZE * 2 + POS_FILE_CAPTION_SIZE ), y,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", "�J��", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectClient.right - ( POS_FILE_BUTT_SIZE * 2 ), y,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( i++ ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", "�ۑ�", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectClient.right - POS_FILE_BUTT_SIZE, y,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( i++ ), 0, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
}

/*** ���O���[�h�����t�@�C���Ή��� *******************************************/

int StrCmpForQSort( const void *a, const void *b ){
	return strcmp( *( const char **)a, *( const char **)b );
}

BOOL FileOpenDialog( char *&szBuf, int iBufSize, char *szExt ){
	OPENFILENAME	ofn;
	memset( &ofn, 0, sizeof( ofn ));
	
	ofn.lStructSize	= sizeof( ofn );
	ofn.lpstrFilter	= szExt;
	ofn.lpstrFile	= szBuf;
	ofn.nMaxFile	= iBufSize;
	ofn.Flags		= OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	*szBuf			= '\0';
	
	if( !GetOpenFileName( &ofn )) return FALSE;
	
	// �t�@�C�����̔z����쐬
	char *p = strchr( szBuf, '\0' ) + 1;	// �ŏ��� dir �����X�L�b�v
	char *szFileNames[ 100 ];
	int	iFileCnt = 0;
	
	// �t�@�C���� 1�����Ȃ��ꍇ�C���̂܂܃��^�[��
	if( !*p ) return TRUE;
	
	while( *p ){
		szFileNames[ iFileCnt++ ] = p;
		p = strchr( p, '\0' ) + 1;
	}
	
	// �\�[�g
	qsort( szFileNames, iFileCnt, sizeof( char * ), StrCmpForQSort );
	
	// �\�[�g���ʂ����ɐV�z��쐬
	char *szNewBuf = new char [ BUF_SIZE ];
	p = szNewBuf;
	
	for( int i = 0; i < iFileCnt; ++i ){
		if( i ) *p++ = '/';
		
		strcpy( p, szBuf ); p = strchr( p, '\0' );
		*p++ = '\\';
		strcpy( p, szFileNames[ i ] ); p = strchr( p, '\0' );
		DebugMsgD( "%s\n", szFileNames[ i ] );
	}
	
	// szBuf ��}���ւ�
	delete [] szBuf;
	szBuf = szNewBuf;
	
	return TRUE;
}

/*** WndProc ****************************************************************/

BOOL func_WndProc( HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *filter ){
	
	TCHAR	szBuf[ MAX_PATH ];
	TCHAR	szBuf2[ MAX_PATH ];
	int		iFrame;
	
	//	TRUE��Ԃ��ƑS�̂��ĕ`�悳���
	
	if( message == WM_FILTER_INIT ) ExtendDialog( hwnd );
	
	//	�ҏW���łȂ���Ή������Ȃ�
	if( filter->exfunc->is_editing( editp ) != TRUE ) return FALSE;
	
	switch( message ) {
	  case WM_FILTER_FILE_OPEN:
		
		g_Vsd = new CVsdFilterAvu( filter, editp );
		
		// trackbar �ݒ�
		#ifdef GPS_ONLY
			if( g_Vsd->VideoSt == INVALID_INT ){
				g_Vsd->VideoSt	= 0;
				g_Vsd->VideoEd	= filter->exfunc->get_frame_n( editp );
			}
		#else
			track_e[ PARAM_VSt ] =
			track_e[ PARAM_VEd ] = filter->exfunc->get_frame_n( editp );
		#endif
		
		// �ݒ�ĕ`��
		filter->exfunc->filter_window_update( filter );
		
	  Case WM_FILTER_FILE_CLOSE:
		delete g_Vsd;
		g_Vsd = NULL;
		
	  Case WM_FILTER_MAIN_KEY_DOWN:
		switch( wparam ){
			
		  case 'M':
			// �}�[�N
			FRAME_STATUS	fsp;
			iFrame = filter->exfunc->get_frame( editp );
			
			filter->exfunc->get_frame_status( editp, iFrame, &fsp );
			
			// �������b�v�v�����[�h�̂Ƃ��C���̃}�[�N�����ׂĉ�������
			if(
				g_Vsd->m_piParamC[ CHECK_LAP ] &&
				g_Vsd->m_piParamT[ TRACK_SLineWidth ] > 0 &&
				!( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME )
			){
				for( int i = 0; i < g_Vsd->GetFrameMax(); ++i ){
					filter->exfunc->get_frame_status( editp, i, &fsp );
					if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
						fsp.edit_flag &= ~EDIT_FRAME_EDIT_FLAG_MARKFRAME;
						filter->exfunc->set_frame_status( editp, i, &fsp );
					}
				}
				filter->exfunc->get_frame_status( editp, iFrame, &fsp );
			}
			
			fsp.edit_flag ^= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
			filter->exfunc->set_frame_status( editp, iFrame, &fsp );
			
			g_Vsd->m_bCalcLapTimeReq = TRUE;
			return TRUE;
		}
		
	  Case WM_FILTER_UPDATE:
		iFrame = filter->exfunc->get_frame( editp );
		if(
			g_Vsd->m_iAdjustPointNum == 0 ||
			g_Vsd->m_iAdjustPointNum == 1 && g_Vsd->m_iAdjustPointVid[ 0 ] != iFrame ||
			g_Vsd->m_iAdjustPointNum == 2 && g_Vsd->m_iAdjustPointVid[ 0 ] != iFrame && g_Vsd->m_iAdjustPointVid[ 1 ] != iFrame
		){
			g_Vsd->m_piParamT[ TRACK_LogOffset ] = 0;
			filter->exfunc->filter_window_update( filter );
		}
		
	  Case WM_COMMAND:
		if( ID_BUTT_SET_VSt <= wparam && wparam <= ID_BUTT_SET_GEd ){
		#ifndef GPS_ONLY
			// �t���[�����Z�b�g
			switch( wparam ){
				case ID_BUTT_SET_VSt:	filter->track[ PARAM_VSt ] = filter->exfunc->get_frame( editp );
				Case ID_BUTT_SET_VEd:	filter->track[ PARAM_VEd ] = filter->exfunc->get_frame( editp );
				Case ID_BUTT_SET_LSt:	if( g_Vsd->m_VsdLog ) filter->track[ PARAM_LSt ] = g_Vsd->m_VsdLog->m_iLogNum;
				Case ID_BUTT_SET_LEd:	if( g_Vsd->m_VsdLog ) filter->track[ PARAM_LEd ] = g_Vsd->m_VsdLog->m_iLogNum;
				Case ID_BUTT_SET_GSt:	if( g_Vsd->m_GPSLog ) filter->track[ PARAM_GSt ] = g_Vsd->m_GPSLog->m_iLogNum;
				Case ID_BUTT_SET_GEd:	if( g_Vsd->m_GPSLog ) filter->track[ PARAM_GEd ] = g_Vsd->m_GPSLog->m_iLogNum;
			}
			// �ݒ�ĕ`��
			filter->exfunc->filter_window_update( filter );
		#endif
			
		}else switch( wparam ){
		  case ID_BUTT_LOAD_CFG:	// .avs ���[�h
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_CFG_EXT, NULL ) &&
				g_Vsd->ConfigLoad( szBuf )
			){
				// ���O���[�h
			#ifndef GPS_ONLY
				if( *g_Vsd->m_szLogFile ){
					g_Vsd->ReadLog(
						GetFullPathWithCDir( szBuf2, g_Vsd->m_szLogFile, szBuf ),
						hwnd
					);
				}
			#endif
				if( *g_Vsd->m_szGPSLogFile ){
					g_Vsd->GPSLogLoad(
						GetFullPathWithCDir( szBuf2, g_Vsd->m_szGPSLogFile, szBuf ),
						hwnd
					);
				}
				
				if( *g_Vsd->m_szSkinFile ) g_Vsd->DeleteScript();
				
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
				
				#ifndef GPS_ONLY
					// log pos �����F���̍X�V
					func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
				#endif
			}
			
			// skin �����_�C�A���O�ɐݒ�
			SetWindowText( GetDlgItem( hwnd, ID_EDIT_SEL_SKIN ), g_Vsd->m_szSkinFile );
			
		  Case ID_BUTT_SAVE_CFG:
			if( filter->exfunc->dlg_get_save_name( szBuf, FILE_CFG_EXT, NULL ))
				return g_Vsd->ConfigSave( szBuf );
			
		#ifndef GPS_ONLY // {
		  Case ID_BUTT_LOAD_LOG:	// .log ���[�h
			if(
				filter->exfunc->dlg_get_load_name( g_Vsd->m_szLogFile, FILE_LOG_EXT, NULL ) &&
				g_Vsd->ReadLog( g_Vsd->m_szLogFile, hwnd )
			){
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
				
				#ifndef GPS_ONLY
					// log pos �����F���̍X�V
					func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
				#endif
			}
		#endif // }
			
		  Case ID_BUTT_LOAD_GPS:	// GPS ���O���[�h
			if(
				FileOpenDialog( g_Vsd->m_szGPSLogFile, BUF_SIZE, FILE_GPS_EXT ) &&
				g_Vsd->GPSLogLoad( g_Vsd->m_szGPSLogFile, hwnd )
			){
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
			}
			
		  Case ID_BUTT_SEL_SKIN:	// �X�L���I��
			if( filter->exfunc->dlg_get_load_name( g_Vsd->m_szSkinFile, FILE_SKIN_EXT, NULL )){
				g_Vsd->DeleteScript();
				
				// skin �����_�C�A���O�ɐݒ�
				SetWindowText( GetDlgItem( hwnd, ID_EDIT_SEL_SKIN ), g_Vsd->m_szSkinFile );
				
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
			}
			
		  Default:
			return FALSE;
		}
		return TRUE;
	}
	
	return FALSE;
}

/*** 1�X���C�_�o�[���� ******************************************************/

void SetupLogOffset( FILTER *filter ){
	
	int	iFrame = filter->exfunc->get_frame( g_Vsd->editp );
	int	iPoint;
	
	// �����|�C���g�𔻒f����
	switch( g_Vsd->m_iAdjustPointNum ){
	  case 0:
		iPoint = 0;
		g_Vsd->m_iAdjustPointNum = 1;
		
	  Case 1:
		if( abs( g_Vsd->m_iAdjustPointVid[ 0 ] - iFrame ) <= ( int )( g_Vsd->GetFPS() * 60 )){
			// �t���[���ʒu���O��� 1���ȓ��Ȃ�C�O��Ɠ����ꏊ���g�p����
			iPoint = 0;
		}else{
			iPoint = 1;
			g_Vsd->m_iAdjustPointNum = 2;
		}
		
	  Default: //Case 2:
		// 2�|�C���g�̂����C�߂��ق����̗p����
		iPoint = (
			abs( g_Vsd->m_iAdjustPointVid[ 0 ] - iFrame ) >
			abs( g_Vsd->m_iAdjustPointVid[ 1 ] - iFrame )
		) ? 1 : 0;
	}
	
	// �t���[���ʒu���O��ƈ���Ă���΁CGPS ���O�ʒu�Ď擾
	if( g_Vsd->m_iAdjustPointVid[ iPoint ] != iFrame ){
		g_Vsd->m_iAdjustPointGPS[ iPoint ] =
			g_Vsd->VideoEd == g_Vsd->VideoSt ? 0
			: ( int )(( double )
				( g_Vsd->GPSEd - g_Vsd->GPSSt )
				* ( iFrame - g_Vsd->VideoSt ) 
				/ ( g_Vsd->VideoEd - g_Vsd->VideoSt )
				+ g_Vsd->GPSSt
			);
		
		g_Vsd->m_iAdjustPointVid[ iPoint ] =
		( iPoint ? g_Vsd->VideoEd : g_Vsd->VideoSt ) = iFrame;
		g_Vsd->m_piParamT[ TRACK_LogOffset ] = 0;
	}
	
	// �Ώے����|�C���g�̐ݒ�
	( iPoint ? g_Vsd->GPSEd : g_Vsd->GPSSt ) =
		g_Vsd->m_iAdjustPointGPS[ iPoint ] - g_Vsd->m_piParamT[ TRACK_LogOffset ];
	
	// iPoint == 1 ���́C���̒����_�� FPS �ɉ����Ď�������
	if( g_Vsd->m_iAdjustPointNum == 1 ){
		#define ADJUST_WIDTH	( 60 * 10 )	// 10��
		
		g_Vsd->VideoEd =
			g_Vsd->VideoSt + ( int )( g_Vsd->GetFPS() * ADJUST_WIDTH );
		
		g_Vsd->GPSEd =
			g_Vsd->GPSSt + ( int )( LOG_FREQ * ADJUST_WIDTH );
	}
	
	filter->exfunc->filter_window_update( filter );
}

/*** �X���C�_�o�[�A�� *******************************************************/

BOOL func_update( FILTER *filter, int status ){
	static	BOOL	bReEnter = FALSE;
	
	if( bReEnter ) return TRUE;
	
	bReEnter = TRUE;
	
	if(
		g_Vsd && (
			#ifndef GPS_ONLY
				status >= FILTER_UPDATE_STATUS_TRACK + PARAM_VSt &&
				status <= FILTER_UPDATE_STATUS_TRACK + PARAM_GSt ||
			#endif
			status == FILTER_UPDATE_STATUS_TRACK + TRACK_LogOffset ||
			status == FILTER_UPDATE_STATUS_TRACK + TRACK_SLineWidth
		)
	) g_Vsd->m_bCalcLapTimeReq = TRUE;
	
	#ifndef GPS_ONLY
		if(
			status == ( FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS ) &&
			filter->check[ CHECK_LOGPOS ]
		){
			filter->track[ PARAM_LSt ] = g_Vsd->m_iLogStart;
			filter->track[ PARAM_LEd ] = g_Vsd->m_iLogStop;
			
			// �ݒ�ĕ`��
			filter->exfunc->filter_window_update( filter );
		}
	#endif
	
	// �}�b�v��]
	if( status == ( FILTER_UPDATE_STATUS_TRACK + TRACK_MapAngle )){
		if( g_Vsd->m_VsdLog )
			g_Vsd->m_VsdLog->RotateMap( filter->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
		if( g_Vsd->m_GPSLog )
			g_Vsd->m_GPSLog->RotateMap( filter->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	}
	
	if( status == FILTER_UPDATE_STATUS_TRACK + TRACK_LogOffset ){
		SetupLogOffset( filter );
	}
	
	bReEnter = FALSE;
	
	return TRUE;
}

/*** save �J�n�O�ɓ������ off **********************************************/

BOOL func_save_start( FILTER *filter,int s,int e,void *editp ){
	filter->check[ CHECK_SYNCINFO ] = 0;
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
