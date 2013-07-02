/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	vsd_filter.cpp - VSD filter for AviUtl
	
*****************************************************************************/

#include "StdAfx.h"

#include "CVsdFilter.h"
#include "rev_num.h"

/*** macros *****************************************************************/

#ifdef PUBLIC_MODE
	#define CONFIG_EXT		"cfg"
#else
	#define CONFIG_EXT		"avs"
#endif

#define	FILE_CFG_EXT		"Config File (*." CONFIG_EXT ")\0*." CONFIG_EXT "\0AllFile (*.*)\0*.*\0"
#define	FILE_SKIN_EXT		"Skin File (*.js)\0*.js\0AllFile (*.*)\0*.*\0"
#define FILE_LAPCHART_EXT	"Lap Chart (*.txt)\0*.txt\0AllFile (*.*)\0*.*\0"

/*** new type ***************************************************************/

/*** const ******************************************************************/

enum {
	ID_BUTT_SET_VSt = 0xFF00,
	ID_BUTT_SET_VEd,
	ID_BUTT_SET_LSt,
	ID_BUTT_SET_LEd,
	ID_BUTT_SET_GSt,
	ID_BUTT_SET_GEd,
	ID_EDIT_LOAD_LOG,
	ID_BUTT_LOAD_LOG,
	ID_EDIT_LOAD_GPS,
	ID_BUTT_LOAD_GPS,
	ID_BUTT_LAP_START,
	ID_BUTT_LAP_END,
	ID_EDIT_LOAD_LAPCHART,
	ID_BUTT_LOAD_LAPCHART,
	ID_COMBO_SEL_SKIN,
	ID_BUTT_SEL_SKIN,	// NOT USED
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

#ifdef PUBLIC_MODE
	#define POS_SET_BUTT_SIZE		0
#else
	#define POS_SET_BUTT_SIZE		30
#endif

#define POS_ADD_DIALOG_HEIGHT	( 10 + POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN )
#define POS_FILE_NUM			5

#define POS_FILE_CAPTION_SIZE	70
#define POS_FILE_NAME_SIZE		( rectClient.right - ( POS_FILE_CAPTION_POS + POS_FILE_CAPTION_SIZE + POS_FILE_BUTT_SIZE ))
#define POS_FILE_BUTT_SIZE		40
#define POS_FILE_CAPTION_POS	150
#define POS_FILE_HEIGHT			21
#define POS_FILE_HEIGHT_MARGIN	2

#define INVALID_INT		0x80000000

#define OFFSET_ADJUST_WIDTH	( 60 * 10 * 1000 )	// 10��


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

char g_szDescription[] = PROG_NAME_J " " PROG_REVISION_STR;

FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_MAIN_MESSAGE | FILTER_FLAG_EX_INFORMATION,
	0,0,						//	�ݒ�E�C���h�E�̃T�C�Y (FILTER_FLAG_WINDOW_SIZE�������Ă��鎞�ɗL��)
	PROG_NAME_J,				//	�t�B���^�̖��O
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

/*** CVsdFilter �N���X ***************************************************/

CVsdFilter	*g_Vsd;

/*** tarckbar / checkbox conf_name �� ***/

const char *CVsdFilter::m_szTrackbarName[] = {
	#define DEF_TRACKBAR( id, init, min, max, name, conf_name ) conf_name,
	#include "def_trackbar.h"
};

const char *CVsdFilter::m_szCheckboxName[] = {
	#define DEF_CHECKBOX( id, init, name, conf_name ) conf_name,
	#include "def_checkbox.h"
};

const char *CVsdFilter::m_szShadowParamName[] = {
	#define DEF_SHADOW( id, init, conf_name ) conf_name,
	#include "def_shadow.h"
};

/*** �R���X�g���N�^�^�f�X�g���N�^ *******************************************/

CVsdFilter::CVsdFilter( FILTER *filter, void *editp ) :
	filter( filter ), editp( editp )
{
	Constructor();	// ��{�N���X�̃R���X�g���N�^
	
	fileinfo = new FILE_INFO;
	filter->exfunc->get_file_info( editp, fileinfo );
	
	m_piParamT	= filter->track;
	m_piParamC	= filter->check;
	m_piParamS	= shadow_param;
	
#ifdef PUBLIC_MODE
	VsdSt = GPSSt = 0;
	VsdEd = GPSEd = ( int )( OFFSET_ADJUST_WIDTH * SLIDER_TIME );
	VideoSt = VideoEd = INVALID_INT;
#endif
	
	m_iVideoStartTime = -1;
	m_iAdjustPointNum = 0;
	m_iAdjustPointVid[ 0 ] =
	m_iAdjustPointVid[ 1 ] = INVALID_INT;
	
	m_szLogFilter	= NULL;
	fpip			= NULL;
}

CVsdFilter::~CVsdFilter(){
	Destructor();	// ��{�N���X�̃f�X�g���N�^
	
	delete fileinfo;
	delete [] m_szLogFilter;
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

void CVsdFilter::PutPixel( int x, int y, const PIXEL_YCA_ARG yc ){
	
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

void CVsdFilter::FillLine( int x1, int y1, int x2, const PIXEL_YCA_ARG yc ){
	
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

UINT CVsdFilter::PutImage( int x, int y, CVsdImage &img, UINT uAlign ){
	PIXEL_YC	*ycp = fpip->ycp_edit;
	
	if( uAlign & ALIGN_HCENTER ){
		x -= img.m_iWidth / 2;
	}else if( uAlign & ALIGN_RIGHT ){
		x -= img.m_iWidth;
	}
	
	if( uAlign & ALIGN_VCENTER ){
		y -= img.m_iHeight / 2;
	}else if( uAlign & ALIGN_BOTTOM ){
		y -= img.m_iHeight;
	}
	
	int xst = ( -x <= img.m_iOffsX ) ? img.m_iOffsX : -x;
	int yst = ( -y <= img.m_iOffsY ) ? img.m_iOffsY : -y;
	int xed = x + img.m_iOffsX + img.m_iRawWidth  <= GetWidth()  ? img.m_iOffsX + img.m_iRawWidth  : GetWidth()  - x;
	int yed = y + img.m_iOffsY + img.m_iRawHeight <= GetHeight() ? img.m_iOffsY + img.m_iRawHeight : GetHeight() - y;
	
	for( int y1 = yst; y1 < yed; ++y1 ){
		for( int x1 = xst; x1 < xed; ++x1 ){
			
			PIXEL_YCA yc( img.GetPixel0( x1, y1 ));
			
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
	
	return ERR_OK;
}

/*** �������`�� ***********************************************************/

WCHAR *CVsdFilter::DrawSyncInfoFormatTime(
	WCHAR	*pBuf, UINT uBufSize, INT64 &iBaseTime, int iTime
){
	struct tm	tmLocal;
	time_t	time = ( iBaseTime + iTime ) / 1000;
	localtime_s( &tmLocal, &time );
	
	wcsftime( pBuf, uBufSize, L"%H:%M:%S", &tmLocal );
	
	LPWSTR	p = wcschr( pBuf, '\0' );
	swprintf(
		p, uBufSize - ( p - pBuf ), L".%02d",
		(( UINT )iBaseTime + iTime ) % 1000 / 10
	);
	
	return pBuf;
}

void CVsdFilter::DrawSyncInfoSub(
	int x, int y, CVsdFont &Font,
	LPCWSTR	szCaption,
	INT64	&iBaseTime,
	int		iCurTime,
	int		iStartTime,
	int		iEndTime
){
	WCHAR	szBuf[ 32 ];
	
	// �L���v�V����
	DrawText( x, y, szCaption, Font, color_white );
	
	// ���t
	struct tm	tmLocal;
	time_t time	= ( time_t )( iBaseTime + iCurTime ) / 1000;
	
	localtime_s( &tmLocal, &time );
	wcsftime( szBuf, sizeof( szBuf ), L"%Y/%m/%d", &tmLocal );
	DrawText( POS_DEFAULT, POS_DEFAULT, szBuf, Font, color_white );
	
	// ���� x 3
	DrawText(
		POS_DEFAULT, POS_DEFAULT,
		DrawSyncInfoFormatTime( szBuf, sizeof( szBuf ), iBaseTime, iCurTime ),
		Font, color_white
	);
	#ifndef PUBLIC_MODE
		DrawText(
			POS_DEFAULT, POS_DEFAULT,
			DrawSyncInfoFormatTime( szBuf, sizeof( szBuf ), iBaseTime, iStartTime ),
			Font, color_white
		);
		DrawText(
			POS_DEFAULT, POS_DEFAULT,
			DrawSyncInfoFormatTime( szBuf, sizeof( szBuf ), iBaseTime, iEndTime ),
			Font, color_white
		);
		
		// ���[�J���^�C���� 0:00:00 �� time_t �𓾂�
		tmLocal.tm_sec =
		tmLocal.tm_min =
		tmLocal.tm_hour = 0;
		
		INT64 iTmp = ( INT64 )mktime( &tmLocal ) * 1000;
		
		DrawText(
			POS_DEFAULT, POS_DEFAULT,
			DrawSyncInfoFormatTime( szBuf, sizeof( szBuf ), iTmp, iEndTime - iStartTime ),
			Font, color_white
		);
	#endif
}

void CVsdFilter::DrawSyncInfo( int x, int y, CVsdFont &Font, UINT uAlign ){
	
	// �t���[���\��
	
	#ifdef PUBLIC_MODE
		#define SYNC_INFO_LINES	3
	#else
		#define SYNC_INFO_LINES	6
	#endif
	
	if( uAlign & ALIGN_HCENTER ){
		x -= Font.GetWidth() * 40 / 2;
	}else if( uAlign & ALIGN_RIGHT ){
		x -= Font.GetWidth() * 40;
	}
	
	if( uAlign & ALIGN_VCENTER ){
		y -= Font.GetHeight() * SYNC_INFO_LINES / 2;
	}else if( uAlign & ALIGN_BOTTOM ){
		y -= Font.GetHeight() * SYNC_INFO_LINES;
	}
	
	m_iTextPosX = x;
	m_iTextPosY = y;
	
	m_iTextPosY += Font.GetHeight();
	DrawText( POS_DEFAULT, POS_DEFAULT, L"����", Font, color_white );
	#ifndef PUBLIC_MODE
	m_iTextPosY += Font.GetHeight();
		DrawText( POS_DEFAULT, POS_DEFAULT, L"�J�n", Font, color_white );
		DrawText( POS_DEFAULT, POS_DEFAULT, L"�I��", Font, color_white );
		DrawText( POS_DEFAULT, POS_DEFAULT, L"�͈�", Font, color_white );
	#endif
	
	x += Font.GetWidth() * 5;
	
	if( m_iVideoStartTime != -1 ){
		DrawSyncInfoSub(
			x, y, Font, L"�r�f�I",
			m_iVideoStartTime,
			( int )( GetFrameCnt() * 1000 / GetFPS()),
			( int )( VideoSt * 1000 / GetFPS()),
			( int )( VideoEd * 1000 / GetFPS())
		);
		x += Font.GetWidth() * 12;
	}
	
	if( m_VsdLog ){
		DrawSyncInfoSub(
			x, y, Font, L"�ԗ����O",
			m_VsdLog->m_iLogStartTime,
			m_VsdLog->GetTime(),
			( int )( VsdSt / SLIDER_TIME ),
			( int )( VsdEd / SLIDER_TIME )
		);
		x += Font.GetWidth() * 12;
	}
	
	if( m_GPSLog ){
		DrawSyncInfoSub(
			x, y, Font, L"GPS���O",
			m_GPSLog->m_iLogStartTime,
			m_GPSLog->GetTime(),
			( int )( GPSSt / SLIDER_TIME ),
			( int )( GPSEd / SLIDER_TIME )
		);
		x += Font.GetWidth() * 12;
	}
}

/*** �G���[���b�Z�[�W *******************************************************/

void CVsdFilter::DispErrorMessage( LPCWSTR szMsg ){
	MessageBoxW( NULL, szMsg, PROG_NAME_J_W, MB_ICONWARNING );
}

/*** �t���[�����}�[�N *******************************************************/

void CVsdFilter::SetFrameMark( int iFrame ){
	FRAME_STATUS	fsp;
	
	filter->exfunc->get_frame_status( editp, iFrame, &fsp );
	fsp.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
	filter->exfunc->set_frame_status( editp, iFrame, &fsp );
}

int CVsdFilter::GetFrameMark( int iFrame ){
	
	FRAME_STATUS	fsp;
	
	for( ; iFrame < GetFrameMax(); ++iFrame ){
		filter->exfunc->get_frame_status( editp, iFrame, &fsp );
		
		if( fsp.edit_flag & EDIT_FRAME_EDIT_FLAG_MARKFRAME ){
			return iFrame;
		}
	}
	return -1;
}

/*** �t�@�C���쐬�����擾 ***************************************************/

void CVsdFilter::GetFileCreationTime( void ){
	FILETIME	ft;
	
	HANDLE hFile = CreateFile(
		fileinfo->name,			// �t�@�C����
		GENERIC_READ,			// �A�N�Z�X���[�h
		FILE_SHARE_READ,		// ���L���[�h
		NULL,					// �Z�L�����e�B�L�q�q
		OPEN_EXISTING,			// �쐬���@
		FILE_ATTRIBUTE_NORMAL,	// �t�@�C������
		NULL					// �e���v���[�g�t�@�C���̃n���h��
	);
	
	if( hFile == NULL ) return;
	
	//if( GetFileTime( hFile, NULL, NULL, &ft )){		// �f�o�b�O�p ���C�g�^�C��
	if( GetFileTime( hFile, &ft, NULL, NULL )){
		m_iVideoStartTime =
			(
				((( LONGLONG )ft.dwHighDateTime << 32 ) + ft.dwLowDateTime ) -
				116444736000000000
			) / 10000;
	}
	
	CloseHandle( hFile );
}

/*** �r�f�I�����ƃ��O�����̏������� *****************************************/

#ifdef PUBLIC_MODE
#define InRange( a, b, v ) ( \
	(( a ) < ( b )) ? (( a ) <= ( v ) && ( v ) <= ( b )) : \
	                  (( a ) <= ( v ) || ( v ) <= ( b )) \
)

void CVsdFilter::AutoSync( CVsdLog *pLog, int *piParam ){
	// filetime ���擾�ł��Ă��Ȃ�
	if(
		pLog == NULL ||
		m_piParamC[ CHECK_LOGPOS ] == 0 ||
		m_iVideoStartTime == -1
	) return;
	
	// �r�f�I�I������ (24H �ȓ�)
	int iVideoStartTime = ( int )( m_iVideoStartTime % ( 24 * 3600 * 1000 ));
	int iVideoEndTime =
		( iVideoStartTime + ( int )( GetFrameMax() * 1000 / GetFPS()))
		% ( 24 * 3600 * 1000 );
	
	// ���O�J�n�E�I������ (24H �ȓ�)
	int iLogStartTime = ( int )( pLog->m_iLogStartTime % ( 24 * 3600 * 1000 ));
	int iLogEndTime   = (( int )( pLog->MaxTime()) + iLogStartTime ) % ( 24 * 3600 * 1000 );
	
	if(
		InRange( iVideoStartTime, iVideoEndTime, iLogStartTime ) ||
		InRange( iVideoStartTime, iVideoEndTime, iLogEndTime ) ||
		InRange( iLogStartTime, iLogEndTime, iVideoStartTime ) ||
		InRange( iLogStartTime, iLogEndTime, iVideoEndTime )
	){
		// �r�f�I�� 10���ɏ���������
		VideoSt = 0;
		VideoEd = ( int )(( OFFSET_ADJUST_WIDTH / 1000.0 ) * GetFPS());
		
		piParam[ 0 ] = ( int )(( iVideoStartTime - iLogStartTime ) % ( 24 * 3600 * 1000 ) * SLIDER_TIME );
		piParam[ 1 ] = piParam[ 0 ] + ( int )( OFFSET_ADJUST_WIDTH * SLIDER_TIME );
	}
}

#undef InRange

#endif // PUBLIC_MODE

/*** ���O���[�h *************************************************************/

BOOL CVsdFilter::ReadVsdLog( HWND hwnd ){
	
	//char szMsg[ BUF_SIZE ];
	int iCnt;
	
	if( !( iCnt = CVsdFilter::ReadLog( m_VsdLog, m_szLogFile, m_szLogFileReader ))){
		return FALSE;
	}
	
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_LOG ), m_szLogFile );
	
	// trackbar �ݒ�
	#ifdef PUBLIC_MODE
		AutoSync( m_VsdLog, &VsdSt );
	#else
		track_e[ PARAM_LSt ] =
		track_e[ PARAM_LEd ] =
			( int )( m_VsdLog->GetTime( m_VsdLog->GetCnt() - 2 ) * SLIDER_TIME );
	#endif
	
	return TRUE;
}

/*** GPS ���O���[�h ********************************************************/

BOOL CVsdFilter::ReadGPSLog( HWND hwnd ){
	
	//char szMsg[ BUF_SIZE ];
	
	int iCnt;
	if( !( iCnt = CVsdFilter::ReadLog( m_GPSLog, m_szGPSLogFile, m_szGPSLogFileReader ))){
		return FALSE;
	}
	
	SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_GPS ), m_szGPSLogFile );
	
	// trackbar �ݒ�
	#ifdef PUBLIC_MODE
		AutoSync( m_GPSLog, &GPSSt );
	#else
		track_e[ PARAM_GSt ] =
		track_e[ PARAM_GEd ] =
			( int )( m_GPSLog->GetTime( m_GPSLog->GetCnt() - 2 ) * SLIDER_TIME );
	#endif
	
	return TRUE;
}

/*** func_proc **************************************************************/

BOOL func_proc( FILTER *fp, FILTER_PROC_INFO *fpip ){
	if( !g_Vsd ) return 0;
	// �N���X�ɕϊ�
	g_Vsd->fpip		= fpip;
	
	return g_Vsd->DrawVSD();
}

/*** *.js �t�@�C�����X�g�擾 ************************************************/

BOOL ListTreeCallback( const char *szPath, const char *szFile, void *pParam ){
	if( !IsExt( szFile, "js" )) return TRUE;
	
	char szBuf[ MAX_PATH + 1 ];
	
	strcat( strcpy( szBuf, szPath ), szFile );
	char *p = szBuf + strlen( g_Vsd->m_szPluginDirA );
	
	SendMessage(( HWND )pParam, CB_ADDSTRING, 0, ( LPARAM )p );
	
	return TRUE;
}

void SetSkinFileList( HWND hwnd ){
	char szBuf[ MAX_PATH + 1 ];
	
	if( SendMessage( hwnd, CB_GETCOUNT, 0, 0 ) > 0 ) return;
	
	strcpy( szBuf, g_Vsd->m_szPluginDirA );
	ListTree( szBuf, "*", ListTreeCallback, hwnd );
	SendMessage( hwnd, CB_ADDSTRING, 0, ( LPARAM )"�t�@�C�������w�肵�ĊJ��..." );
	SetWindowText( hwnd, g_Vsd->m_szSkinFile );
}

/*** �_�C�A���O�T�C�Y�g���ƃp�[�c�ǉ� ***************************************/

#define CreateButton( Caption, x, y, id )( \
	hwndChild = CreateWindow( \
		"BUTTON", ( Caption ), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, \
		( x ), ( y ), POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT, \
		hwnd, ( id ), hInst, NULL \
	), \
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 ) \
)

void CreateControlFileName(
	HWND hwnd, HINSTANCE hInst, int &iID, HFONT hfont, int iX, int &iY, RECT rectClient,
	char *szCap, char *szEdit, char *szButt
) {
	
	HWND hwndChild;
	
	hwndChild = CreateWindow(
		"STATIC", szCap, WS_CHILD | WS_VISIBLE,
		iX, iY,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"EDIT", szEdit, WS_CHILD | WS_VISIBLE | ES_READONLY,
		iX + POS_FILE_CAPTION_SIZE, iY,
		POS_FILE_NAME_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )iID, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindow(
		"BUTTON", szButt, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		iX + POS_FILE_CAPTION_SIZE + POS_FILE_NAME_SIZE, iY,
		POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT,
		hwnd, ( HMENU )( iID + 1 ), hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	iID += 2;
	iY += POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN;
}

void CreateControlSkinName(
	HWND hwnd, HINSTANCE hInst, int &iID, HFONT hfont, int iX, int &iY, RECT rectClient,
	char *szCap, char *szEdit
) {
	
	HWND hwndChild;
	
	hwndChild = CreateWindow(
		"STATIC", szCap, WS_CHILD | WS_VISIBLE,
		iX, iY,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	hwndChild = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN,
		iX + POS_FILE_CAPTION_SIZE, iY,
		POS_FILE_NAME_SIZE + POS_FILE_BUTT_SIZE, POS_FILE_HEIGHT * 10,
		hwnd, ( HMENU )iID, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	SendMessage( hwndChild, CB_SETCURSEL, 0, 0 );
	
	iID += 2;
	iY += POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN;
}

void ExtendDialog( HWND hwnd, HINSTANCE hInst ){
	
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
		rectClient.bottom - rectClient.top + POS_ADD_DIALOG_HEIGHT,
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
	#ifndef PUBLIC_MODE
		for( i = 0; i <= ( ID_BUTT_SET_GEd - ID_BUTT_SET_VSt ); ++i ){
			hwndChild = CreateWindow(
				"BUTTON", "set", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				rectClient.right - POS_SET_BUTT_SIZE, 14 + ( i + 2 ) * 24,
				POS_SET_BUTT_SIZE, 16,
				hwnd, ( HMENU )( ID_BUTT_SET_VSt + i ), hInst, NULL
			);
			SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
		}
	#endif
	
	// ���O���E�t�H���g��
	i = ID_BUTT_SET_GEd + 1;
	int y = rectClient.bottom - ( POS_FILE_HEIGHT + POS_FILE_HEIGHT_MARGIN ) * POS_FILE_NUM + POS_FILE_HEIGHT_MARGIN;
	
	CreateControlFileName( hwnd, hInst, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "�ԗ����O",	"",		"�J��" );
	CreateControlFileName( hwnd, hInst, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "GPS���O",	"",		"�J��" );
	
	CreateButton( "�n", rectClient.right - ( POS_FILE_BUTT_SIZE * 2 ), y, ( HMENU )( i++ ));
	CreateButton( "�I", rectClient.right - POS_FILE_BUTT_SIZE,         y, ( HMENU )( i++ ));
	
	rectClient.right -= POS_FILE_BUTT_SIZE * 2;
	CreateControlFileName( hwnd, hInst, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "LAP�\",	"",		"�J��" );
	rectClient.right += POS_FILE_BUTT_SIZE * 2;
	
	CreateControlSkinName( hwnd, hInst, i, hfont, POS_FILE_CAPTION_POS, y, rectClient, "�X�L��",	"" );
	
	// rev
	hwndChild = CreateWindow(
		"STATIC", "VSD for GPS " PROG_REVISION_STR, WS_CHILD | WS_VISIBLE,
		POS_FILE_CAPTION_POS + POS_FILE_CAPTION_SIZE, y,
		POS_FILE_NAME_SIZE / 2, POS_FILE_HEIGHT,
		hwnd, 0, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	// cfg load/save �{�^��
	hwndChild = CreateWindow(
		"STATIC", "cfg�t�@�C��", WS_CHILD | WS_VISIBLE,
		rectClient.right - ( POS_FILE_BUTT_SIZE * 2 + POS_FILE_CAPTION_SIZE ), y,
		POS_FILE_CAPTION_SIZE, POS_FILE_HEIGHT,
		hwnd, 0, hInst, NULL
	);
	SendMessage( hwndChild, WM_SETFONT, ( WPARAM )hfont, 0 );
	
	CreateButton( "�J��", rectClient.right - ( POS_FILE_BUTT_SIZE * 2 ), y, ( HMENU )( i++ ));
	CreateButton( "�ۑ�", rectClient.right - POS_FILE_BUTT_SIZE,         y, ( HMENU )( i++ ));
}

/*** OpenDialog �p�t�B���^�쐬 **********************************************/

BOOL CVsdFilter::CreateFilter( void ){
	char	szBuf[ BUF_SIZE ];
	char	*pBuf;
	
	{
		// JavaScript �I�u�W�F�N�g������
		CScript Script( this );
		if( Script.InitLogReader() != ERR_OK ){
			return FALSE;
		}
		
		/*** JS �� Log �ɃA�N�Z�X *******************************************/
		
		{
			v8::Isolate::Scope IsolateScope( Script.m_pIsolate );
			v8::HandleScope handle_scope;
			v8::Context::Scope context_scope( Script.m_Context );
			
			// "LogReaderInfo" �擾
			v8::Local<v8::Array> hFilter = v8::Local<v8::Array>::Cast(
				Script.m_Context->Global()->Get( v8::String::New( "LogReaderInfo" ))
			);
			if( !hFilter->IsArray()) return FALSE;
			
			m_vecReaderFunc.clear();
			
			pBuf = szBuf;
			// �\������ filter ������� \0 �� cat
			strcpy( pBuf, "�������� (*.*)" );
			pBuf = strchr( pBuf, '\0' ) + 1;
			strcpy( pBuf, "*" );
			pBuf = strchr( pBuf, '\0' ) + 1;
			
			for( UINT u = 0; u < hFilter->Length(); ++u ){
				v8::String::Value      strName( v8::Local<v8::Array>::Cast( hFilter->Get( u ))->Get( v8::String::New( "Caption" )));
				v8::String::AsciiValue strExt ( v8::Local<v8::Array>::Cast( hFilter->Get( u ))->Get( v8::String::New( "Filter" )));
				v8::String::AsciiValue strFunc( v8::Local<v8::Array>::Cast( hFilter->Get( u ))->Get( v8::String::New( "ReaderFunc" )));
				
				WideCharToMultiByte(
					CP_ACP,					// �R�[�h�y�[�W
					0,						// �����̎�ނ��w�肷��t���O
					( LPCWSTR )*strName,	// �}�b�v��������̃A�h���X
					-1,						// �}�b�v��������̃o�C�g��
					pBuf,					// �}�b�v�惏�C�h�����������o�b�t�@�̃A�h���X
					BUF_SIZE - ( pBuf - szBuf ),	// �o�b�t�@�̃T�C�Y
					NULL, NULL
				);
				
				pBuf = strchr( pBuf, '\0' ) + 1;
				strncpy( pBuf, *strExt, BUF_SIZE - ( pBuf - szBuf ));
				pBuf = strchr( pBuf, '\0' ) + 1;
				m_vecReaderFunc.push_back( *strFunc );
			}
			
			*( pBuf++ ) = '\0';
		}
	}
	
	if( m_szLogFilter ) delete [] m_szLogFilter;
	int iSize = pBuf - szBuf;
	m_szLogFilter = new char[ iSize ];
	memcpy_s( m_szLogFilter, iSize, szBuf, iSize );
	
	return TRUE;
}

/*** ���O���[�h�����t�@�C���Ή��� *******************************************/

#define FILTERBUF_SIZE	128

BOOL CVsdFilter::FileOpenDialog( char *&szOut, char *&szReaderFunc ){
	
	char szBuf[ BUF_SIZE ];
	
	// ���O���[�_�p�t�B���^�쐬
	if( m_szLogFilter == NULL && CreateFilter() != TRUE ){
		return FALSE;
	}
	
	OPENFILENAME	ofn;
	memset( &ofn, 0, sizeof( ofn ));
	
	ofn.lStructSize	= sizeof( ofn );
	ofn.lpstrFilter	= m_szLogFilter;
	ofn.lpstrFile	= szBuf;
	ofn.nMaxFile	= BUF_SIZE;
	ofn.Flags		= OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	*szBuf			= '\0';
	
	if( !GetOpenFileName( &ofn )) return FALSE;
	
	// �t�@�C������ '/' �ŘA��
	char *p = szBuf;
	
	while( 1 ){
		p = strchr( p, '\0' );
		if( *( p + 1 ) == '\0' ) break;
		*p = '/';
		++p;
	}
	
	StringNew( szOut, szBuf );
	StringNew(
		szReaderFunc,
		ofn.nFilterIndex >= 2 ? m_vecReaderFunc[ ofn.nFilterIndex - 2 ].c_str() :
								NULL
	);
	return TRUE;
}

/*** �ݒ胍�[�h *************************************************************/

char *CVsdFilter::IsConfigParam( const char *szParamName, char *szBuf, int &iVal ){
	
	int	iLen;
	
	while( isspace( *szBuf )) ++szBuf;
	
	if(
		strncmp( szBuf, szParamName, iLen = strlen( szParamName )) == 0 &&
		szBuf[ iLen ] == '='
	){
		iVal = atoi( szBuf + iLen + 1 );
		return szBuf + iLen + 1;
	}
	
	return NULL;
}

char *CVsdFilter::IsConfigParamStr( const char *szParamName, char *szBuf, char *&szDst ){
	
	int		iLen;
	char	*p;
	
	while( isspace( *szBuf )) ++szBuf;
	
	if(
		strncmp( szBuf, szParamName, iLen = strlen( szParamName )) == 0 &&
		szBuf[ iLen ] == '='
	){
		szBuf += iLen + 1;	// " ���w���Ă���͂�
		
		// ������擪
		if( p = strchr( szBuf, '"' )){
			szBuf = p + 1;
		}
		
		StringNew( szDst, szBuf );
		
		// ������I�[
		if(( p = strchr( szDst, '"' )) || ( p = strchr( szDst, ',' ))){
			*p = '\0';
		}
		
		return szDst;
	}
	
	return NULL;
}

BOOL CVsdFilter::ConfigLoad( const char *szFileName ){
	
	int 	i, iVal;
	FILE	*fp;
	char	szBuf[ BUF_SIZE ];
	
	if(( fp = fopen( szFileName, "r" )) != NULL ){
		m_bCalcLapTimeReq = TRUE;
		
		while( fgets( szBuf, BUF_SIZE, fp )){
			if( char *p = IsConfigParam( "mark", szBuf, iVal )){
				// ���b�v�^�C���}�[�N
				ParseMarkStr( p + 1 );
			}
			
			// str param �̃��[�h
			#define DEF_STR_PARAM( id, var, init, conf_name ) else if( IsConfigParamStr( conf_name, szBuf, var ));
			#include "def_str_param.h"
			
			else{
				// Mark �ȊO�̃p�����[�^
				for( i = 0; i < TRACK_N; ++i ){
					if(
						m_szTrackbarName[ i ] &&
						IsConfigParam( m_szTrackbarName[ i ], szBuf, iVal )
					){
						m_piParamT[ i ] = iVal;
						goto Next;
					}
				}
				
				for( i = 0; i < CHECK_N; ++i ){
					if(
						m_szCheckboxName[ i ] &&
						IsConfigParam( m_szCheckboxName[ i ], szBuf, iVal )
					){
						m_piParamC[ i ] = iVal;
						goto Next;
					}
				}
				
				for( i = 0; i < SHADOW_N; ++i ){
					if( IsConfigParam( m_szShadowParamName[ i ], szBuf, iVal )){
						m_piParamS[ i ] = iVal;
						goto Next;
					}
				}
			}
		  Next: ;
		}
		fclose( fp );
	}
	return TRUE;
}

void CVsdFilter::SetSkinName( char *szSkinFile, HWND hwnd ){
	DeleteScript();
	SetSkinFile( szSkinFile );
	
	// skin �����_�C�A���O�ɐݒ�
	SetWindowText( GetDlgItem( hwnd, ID_COMBO_SEL_SKIN ), m_szSkinFile );
}

/*** config �Z�[�u **********************************************************/

enum {
	#define DEF_STR_PARAM( id, var, init, conf_name ) id,
	#include "def_str_param.h"
};

BOOL CVsdFilter::ConfigSave( const char *szFileName ){
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
		if( var && ( init == NULL || strcmp( var, init ) != 0 )){ \
			fprintf( fp, "%c \\\n\t" conf_name "=\"%s\"", cSep, var ); \
			cSep = ','; \
			uStrParamFlag |= 1 << id; \
		}
	#include "def_str_param.h"
	
	for( i = 0; i < TRACK_N; ++i ){
		if(
			m_szTrackbarName[ i ] == NULL ||
			i >= TRACK_LineTrace && m_piParamT[ i ] == track_default[ i ] ||
			( i == PARAM_LSt || i == PARAM_LEd ) && !( uStrParamFlag & ( 1 << STRPARAM_LOGFILE )) ||
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
			m_piParamS[ i ] == shadow_default[ i ] ||
			( i == SHADOW_LAP_CHART_St || i == SHADOW_LAP_CHART_Ed ) && !( uStrParamFlag & ( 1 << STRPARAM_LAPCHART ))
		) continue;
		
		fprintf(
			fp, ", \\\n\t%s=%d", m_szShadowParamName[ i ], m_piParamS[ i ]
		);
	}
	
	// �蓮���b�v�v���}�[�N�o��
	if( m_LapLog && m_LapLog->m_iLapMode < LAPMODE_MAGNET && m_LapLog->m_iLapNum ){
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
	
	fprintf( fp, " \\\n)\n" );
	#ifndef PUBLIC_MODE
		//fprintf( fp, "# Amplify( 0.2 )\n" );
		
		int iSelStart, iSelEnd;
		if( filter->exfunc->get_select_frame( editp, &iSelStart, &iSelEnd )){
			if( iSelEnd == GetFrameMax() - 1 ) iSelEnd = 0;
			if( iSelStart != 0 || iSelEnd != 0 )
				fprintf( fp, "Trim( %d, %d )\n", iSelStart, iSelEnd );
		}
	#endif
	
	fclose( fp );
	return TRUE;
}

/*** WndProc ****************************************************************/

BOOL func_WndProc( HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *filter ){
	
	TCHAR	szBuf[ MAX_PATH + 1 ];
	TCHAR	szBuf2[ MAX_PATH + 1 ];
	int		iFrame;
	
	//	TRUE��Ԃ��ƑS�̂��ĕ`�悳���
	
	if( message == WM_FILTER_INIT ) ExtendDialog( hwnd, 0 );
	
	//	�ҏW���łȂ���Ή������Ȃ�
	if( filter->exfunc->is_editing( editp ) != TRUE ) return FALSE;
	
	switch( message ) {
	  case WM_FILTER_FILE_OPEN:
		
		g_Vsd = new CVsdFilter( filter, editp );
		
		// trackbar �ݒ�
		#ifdef PUBLIC_MODE
			g_Vsd->VideoSt = 0;
			g_Vsd->VideoEd = ( int )( g_Vsd->GetFPS() * ( OFFSET_ADJUST_WIDTH / 1000.0 ));
		#else
			track_e[ PARAM_VSt ] =
			track_e[ PARAM_VEd ] = filter->exfunc->get_frame_n( editp );
		#endif
		
		g_Vsd->m_piParamS[ SHADOW_LAP_CHART_St ] = 0;
		g_Vsd->m_piParamS[ SHADOW_LAP_CHART_Ed ] = filter->exfunc->get_frame_n( editp );
		
		// ���X�g�{�b�N�X�A�C�e���ǉ�
		SetSkinFileList( GetDlgItem( hwnd, ID_COMBO_SEL_SKIN ));
		
		// �t�@�C���^�C���擾
		g_Vsd->GetFileCreationTime();
		
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
			g_Vsd->m_piParamT[ TRACK_VsdLogOffset ] = 0;
			g_Vsd->m_piParamT[ TRACK_GPSLogOffset ] = 0;
			filter->exfunc->filter_window_update( filter );
		}
		
	  Case WM_COMMAND:
		#ifndef PUBLIC_MODE
		if( ID_BUTT_SET_VSt <= wparam && wparam <= ID_BUTT_SET_GEd ){
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
		}else
		#endif
		
		switch( wparam ){
		  case ID_BUTT_LOAD_CFG:	// .avs ���[�h
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_CFG_EXT, NULL ) &&
				g_Vsd->ConfigLoad( szBuf )
			){
				StrTokFile( szBuf2, szBuf, STF_PATH2 );
				CPushDir push_dir( szBuf2 );
				
				if(
					g_Vsd->m_szLapChart &&
					g_Vsd->LapChartRead( g_Vsd->m_szLapChart )
				){
					filter->exfunc->filter_window_update( filter );
					SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_LAPCHART ), g_Vsd->m_szLapChart );
					g_Vsd->DeleteScript();
				}
				
				// ���O���[�h
				if( g_Vsd->m_szLogFile ){
					DebugCmd(
						UINT	uTimer = GetTickCount();
						DebugMsgD( "VSD Log read start\n" );
					);
					
					g_Vsd->ReadVsdLog( hwnd );
					
					DebugCmd( DebugMsgD( "VSD Log read time = %d\n", GetTickCount() - uTimer ); )
				}
				
				if( g_Vsd->m_szGPSLogFile ){
					DebugCmd(
						UINT	uTimer = GetTickCount();
						DebugMsgD( "GPS Log read start\n" );
					);
					// �P���t�@�C��
					g_Vsd->ReadGPSLog( hwnd );
					
					DebugCmd( DebugMsgD( "GPS Log read time = %d\n", GetTickCount() - uTimer ); )
				}
				
				if( g_Vsd->m_szSkinFile ){
					g_Vsd->SetSkinName( g_Vsd->m_szSkinFile, hwnd );
				}
				
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
				
				#ifndef PUBLIC_MODE
					// log pos �����F���̍X�V
					func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
				#endif
			}
			
		  Case ID_BUTT_SAVE_CFG:
			if( filter->exfunc->dlg_get_save_name( szBuf, FILE_CFG_EXT, NULL ))
				return g_Vsd->ConfigSave( szBuf );
			
		  Case ID_BUTT_LOAD_LOG:	// .log ���[�h
			if( g_Vsd->FileOpenDialog( g_Vsd->m_szLogFile, g_Vsd->m_szLogFileReader )){
				if( g_Vsd->ReadVsdLog( hwnd )){
					// �ݒ�ĕ`��
					filter->exfunc->filter_window_update( filter );
					
					#ifndef PUBLIC_MODE
						// log pos �����F���̍X�V
						func_update( filter, FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS );
					#endif
				}
				g_Vsd->DeleteScript();
			}
			
		  Case ID_BUTT_LOAD_GPS:	// GPS ���O���[�h
			if( g_Vsd->FileOpenDialog( g_Vsd->m_szGPSLogFile, g_Vsd->m_szGPSLogFileReader )){
				if( g_Vsd->ReadGPSLog( hwnd )){
					// �ݒ�ĕ`��
					filter->exfunc->filter_window_update( filter );
				}
				g_Vsd->DeleteScript();
			}
			
		  Case ID_BUTT_LOAD_LAPCHART:	// ���b�v�`���[�g ���[�h
			if(
				filter->exfunc->dlg_get_load_name( szBuf, FILE_LAPCHART_EXT, NULL ) &&
				g_Vsd->LapChartRead( szBuf )
			){
				StringNew( g_Vsd->m_szLapChart, szBuf );
				filter->exfunc->filter_window_update( filter );
				SetWindowText( GetDlgItem( hwnd, ID_EDIT_LOAD_LAPCHART ), szBuf );
				g_Vsd->m_bCalcLapTimeReq = TRUE;
				g_Vsd->DeleteScript();
			}
			
		  Case ID_BUTT_LAP_START:
			g_Vsd->m_piParamS[ SHADOW_LAP_CHART_St ] = filter->exfunc->get_frame( editp );
			g_Vsd->m_bCalcLapTimeReq = TRUE;
			return TRUE;
			
		  Case ID_BUTT_LAP_END:
			g_Vsd->m_piParamS[ SHADOW_LAP_CHART_Ed ] = filter->exfunc->get_frame( editp );
			g_Vsd->m_bCalcLapTimeReq = TRUE;
			return TRUE;
			
		  Case ( CBN_SELCHANGE << 16 ) | ID_COMBO_SEL_SKIN:	// �X�L���I��
			{
				HWND hWndCombo = GetDlgItem( hwnd, ID_COMBO_SEL_SKIN );
				int i = SendMessage( hWndCombo, CB_GETCURSEL, 0, 0 );
				
				// �t�@�C�����J�����I�����ꂽ?
				if( SendMessage( hWndCombo, CB_GETCOUNT, 0, 0 ) - 1 == i ){
					
					// �_�C�A���O���L�����Z�����ꂽ
					if( !filter->exfunc->dlg_get_load_name( szBuf, FILE_SKIN_EXT, NULL )){
						// �Ƃ肠�����C���O�� skin �������̂܂܃R�s�[���Ƃ�
						//strcpy( szBuf, g_Vsd->m_szSkinFile );
						//SetWindowText( hWndCombo, g_Vsd->m_szSkinFile );
						// �����Ă����ʂ�����...
						return TRUE;
					}
					
					SendMessage( hWndCombo, CB_INSERTSTRING, 0, ( LPARAM )szBuf );
					SendMessage( hWndCombo, CB_SETCURSEL, 0, 0 );
				}else{
					// �X�L�����h���b�v�_�E�����X�g����I�����ꂽ
					SendMessage( hWndCombo, CB_GETLBTEXT, i, ( LPARAM )szBuf );
				}
				
				g_Vsd->SetSkinName( szBuf, hwnd );
				filter->exfunc->filter_window_update( filter );	// �ݒ�ĕ`��
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
	// m_iAdjustPointNum = 0 ���ɂ͂�����ʂ邱�Ƃ͂Ȃ�
	if( g_Vsd->m_iAdjustPointVid[ iPoint ] != iFrame ){
		g_Vsd->m_iAdjustPointVsd[ iPoint ] =
			g_Vsd->VideoEd == g_Vsd->VideoSt ? 0
			: ( int )(
				( double )( g_Vsd->VsdEd - g_Vsd->VsdSt )
				* ( iFrame - g_Vsd->VideoSt ) 
				/ ( g_Vsd->VideoEd - g_Vsd->VideoSt )
				+ g_Vsd->VsdSt
			);
		g_Vsd->m_piParamT[ TRACK_VsdLogOffset ] = 0;
		
		g_Vsd->m_iAdjustPointGPS[ iPoint ] =
			g_Vsd->VideoEd == g_Vsd->VideoSt ? 0
			: ( int )(
				( double )( g_Vsd->GPSEd - g_Vsd->GPSSt )
				* ( iFrame - g_Vsd->VideoSt ) 
				/ ( g_Vsd->VideoEd - g_Vsd->VideoSt )
				+ g_Vsd->GPSSt
			);
		g_Vsd->m_piParamT[ TRACK_GPSLogOffset ] = 0;
		
		g_Vsd->m_iAdjustPointVid[ iPoint ] =
		( iPoint ? g_Vsd->VideoEd : g_Vsd->VideoSt ) = iFrame;
	}
	
	// �Ώے����|�C���g�̐ݒ�
	( iPoint ? g_Vsd->VsdEd : g_Vsd->VsdSt ) =
		g_Vsd->m_iAdjustPointVsd[ iPoint ] - g_Vsd->m_piParamT[ TRACK_VsdLogOffset ];
	( iPoint ? g_Vsd->GPSEd : g_Vsd->GPSSt ) =
		g_Vsd->m_iAdjustPointGPS[ iPoint ] - g_Vsd->m_piParamT[ TRACK_GPSLogOffset ];
	
	// iPoint == 1 ���́C���̒����_�� FPS �ɉ����Ď�������
	if( g_Vsd->m_iAdjustPointNum == 1 ){
		g_Vsd->VideoEd = g_Vsd->VideoSt + ( int )( g_Vsd->GetFPS() * ( OFFSET_ADJUST_WIDTH / 1000.0 ));
		g_Vsd->VsdEd   = g_Vsd->VsdSt + ( int )( SLIDER_TIME * OFFSET_ADJUST_WIDTH );
		g_Vsd->GPSEd   = g_Vsd->GPSSt + ( int )( SLIDER_TIME * OFFSET_ADJUST_WIDTH );
		
		#ifndef PUBLIC_MODE
			// �X���C�_�ݒ�l���X���C�_�o�[�̐ݒ�͈͂𒴂��Ă���ꍇ�C
			// �X���C�_�o�[�ݒ�͈͂��Đݒ肷��
			if( g_Vsd->VideoEd > filter->track_e[ PARAM_VSt ] ){
				filter->track_e[ PARAM_VSt ] =
				filter->track_e[ PARAM_VEd ] = g_Vsd->VideoEd;
			}
			if( g_Vsd->VsdEd > filter->track_e[ PARAM_GSt ] ){
				filter->track_e[ PARAM_GSt ] =
				filter->track_e[ PARAM_GEd ] = g_Vsd->VsdEd;
			}
			if( g_Vsd->GPSEd > filter->track_e[ PARAM_GSt ] ){
				filter->track_e[ PARAM_GSt ] =
				filter->track_e[ PARAM_GEd ] = g_Vsd->GPSEd;
			}
		#endif
	}
	
	filter->exfunc->filter_window_update( filter );
}

/*** �X���C�_�o�[�A�� *******************************************************/

BOOL func_update( FILTER *filter, int status ){
	static	BOOL	bReEnter = FALSE;
	
	if( !g_Vsd || bReEnter ) return TRUE;
	
	bReEnter = TRUE;
	
	if(
		status >= FILTER_UPDATE_STATUS_TRACK + TRACK_VsdLogOffset &&
	#ifdef PUBLIC_MODE
		status <= FILTER_UPDATE_STATUS_TRACK + TRACK_GPSLogOffset ||
	#else
		status <= FILTER_UPDATE_STATUS_TRACK + PARAM_GEd ||
	#endif
		status == FILTER_UPDATE_STATUS_TRACK + TRACK_SLineWidth
	) g_Vsd->m_bCalcLapTimeReq = TRUE;
	
	// ���O�ʒu�����F�����[�h�̐ݒ�ύX
	if( status == ( FILTER_UPDATE_STATUS_CHECK + CHECK_LOGPOS )){
		
		#ifdef PUBLIC_MODE
			if( filter->check[ CHECK_LOGPOS ] ){
				g_Vsd->AutoSync( g_Vsd->m_VsdLog, &g_Vsd->VsdSt );
				g_Vsd->AutoSync( g_Vsd->m_GPSLog, &g_Vsd->GPSSt );
			}
		#else
			if( filter->check[ CHECK_LOGPOS ] && g_Vsd->m_VsdLog ){
				filter->track[ PARAM_LSt ] = ( int )( g_Vsd->m_VsdLog->m_iCalibStart * SLIDER_TIME );
				filter->track[ PARAM_LEd ] = ( int )( g_Vsd->m_VsdLog->m_iCalibStop  * SLIDER_TIME );
				
				// �ݒ�ĕ`��
				filter->exfunc->filter_window_update( filter );
			}
		#endif
	}
	
	// �}�b�v��]
	else if( status == ( FILTER_UPDATE_STATUS_TRACK + TRACK_MapAngle )){
		if( g_Vsd->m_VsdLog )
			g_Vsd->m_VsdLog->RotateMap( filter->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
		if( g_Vsd->m_GPSLog )
			g_Vsd->m_GPSLog->RotateMap( filter->track[ TRACK_MapAngle ] * ( -ToRAD / 10 ));
	}
	
	else if(
		status == FILTER_UPDATE_STATUS_TRACK + TRACK_VsdLogOffset ||
		status == FILTER_UPDATE_STATUS_TRACK + TRACK_GPSLogOffset
	){
		SetupLogOffset( filter );
	}
	
	bReEnter = FALSE;
	
	return TRUE;
}

/*** �t�@�C�����C�g���擾 *************************************************/

#ifdef PUBLIC_MODE
BOOL IsFileWriteEnabled( void ){
	if( g_Vsd->filter->exfunc->ini_load_int( g_Vsd->filter, "accept_file_wite", 0 )){
		return TRUE;
	}
	
	if(
		MessageBox( NULL,
			"VSD �� JavaScript ���t�@�C���������݂��邱�Ƃ������܂���?\n"
			"�͂� ��I�ԂƁC�Ȍ�̃t�@�C���������݂͂��ׂċ�����܂��D",
			"VSD for GPS",
			MB_YESNO | MB_ICONQUESTION
		) == IDYES
	){
		g_Vsd->filter->exfunc->ini_save_int( g_Vsd->filter, "accept_file_wite", 1 );
		return TRUE;
	}
	
	return FALSE;
}
#endif
