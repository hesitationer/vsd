/*****************************************************************************
	
	$Id$
	
	VSD - poor VSD
	Copyright(C) by DDS
	
	main.h -- main / main2 common header
	
*****************************************************************************/

/*** common macros **********************************************************/

#define H8HZ			16030000
#define SERIAL_DIVCNT	16		// �V���A���o�͂��s������
#define LOG_FREQ		( H8HZ / 65536 / SERIAL_DIVCNT )

// ���݃M�A�����߂�Ƃ��̃X���b�V�����h�ݒ�l
// 10%�}�[�W���͔p�~
#define GEAR_TH( g )	(( UINT )( GEAR_RATIO ## g * 256 + 0.5 ))

// �X�s�[�h * 100/Taco ��
/* FTO
#define GEAR_RATIO1	0.726043277
#define GEAR_RATIO2	1.34676007
#define GEAR_RATIO3	1.900649599
#define GEAR_RATIO4	2.544512663
*/
// ELISE
// �M�A�� * �}�[�W������Ȃ��āCave( �M�An, �M�An+1 ) �ɕύX
#define GEAR_RATIO1 1.2381712993947
#define GEAR_RATIO2 1.82350889069989
#define GEAR_RATIO3 2.37581451065366
#define GEAR_RATIO4 2.95059529470571

//#define PULSE_PAR_1KM	( 637 * 4 )		// FTO
// ���Ԃ�C�z�C�������30�p���X
//#define PULSE_PAR_1KM	(( double )172155 / 11.410 )	// ELISE
#define PULSE_PAR_1KM	(( double )68774.48913 / 4.597593609 )	// ELISE(�␳��)

#define ITOA_RADIX_BIT	7
#define ITOA_DIGIT_NUM	(( 32 + ITOA_RADIX_BIT - 1 ) / ITOA_RADIX_BIT )

//#define ACC_1G_X	6762.594337	// X��
#define ACC_1G_X	6659.691379	// Z��
#define ACC_1G_Y	6591.556755

/*** macros *****************************************************************/

#ifndef _WIN32
//#define RISE_EDGE_CAR_SIGNAL
//#define TEST_CLK	19

//#define IR_FLASHER
//#define TEST1SEC

// �V�t�g�_�E���x�����C���̃M�A�ł����̉�]�������������x��
//#define SHIFTDOWN_WARN	8000	// FTO
#define SHIFTDOWN_WARN	6500	// ELISE

#define SH_DOWN_TH( g )	(( UINT )( SHIFTDOWN_WARN * GEAR_RATIO ## g ))

// �x�N�^�Z�b�g�A�b�v
#define SetVector( v, f )	( *( void **)(( v ) * 2 + 0xFF4C ) = ( f ))

// ���j�^�[�N��
#define MONITOR_ENTRY	( *(( void (*)( void ))0x10E ))
//#define GoMonitor	INT000

// �\�t�g���Z�b�g
#define SoftReset	( *(( void (*)( void ))0x100 ))

// LED �A�j���[�V�����f�[�^��`�p
#define LED_ANIME_PARAM_NUM	6
#define LED_ANIME( a, b, c, d, bar, time ) \
	( time ) / ( CALC_DIVCNT ), FONT_ ## a, FONT_ ## b, FONT_ ## c, FONT_ ## d, bar

// VRAM �p�f�[�^�ꊇ����
#define MakeDisp( a, b, c, d ) \
	(( FONT_ ## a << 24 ) | ( FONT_ ## b << 16 ) | ( FONT_ ## c << 8 ) | FONT_ ## d )

// AD �ϊ��@���W�X�^
#define	G_SENSOR_X	AD.ADDRC
#define	G_SENSOR_Y	AD.ADDRB
#define	G_SENSOR_Z	AD.ADDRA

#define BEEP_OFF	0xFFFF

#define BZero( v )	bzero(( UCHAR *)( &v ), sizeof( v ))

/*** const ******************************************************************/

#define BLINK_RATE	( 1 << 4 )	// �u�����NRate

#define CALC_DIVCNT		8		// Taco �v�Z�����s������
#define SPD_DIVCNT		16		// speed �\�����s������

#define SPEED_ADJ		(( ULONG )(( double )H8HZ * 3600 * 100 / PULSE_PAR_1KM ))
#define TACO_ADJ		(( ULONG )(( double )H8HZ * 60 / 2 ))

// ���O��NewLap���炱�ꂾ���󂩂Ȃ���NewLap�Ƃ��ĔF�߂Ȃ�(3�b)
#define NEWLAP_MIN_INTERVAL	(( ULONG )(( double )H8HZ * 3 / 65536 ))

#define VOLATILE( t, v )	( *( volatile t *)&v )

/* FTO
#define LED_BAR_STEP_1ST	500
#define LED_BAR_REDZONE_1ST	( 8000 - LED_BAR_STEP )
#define LED_BAR_STEP		200
#define LED_BAR_REDZONE		( 8000 - LED_BAR_STEP )
*/
//ELISE
#define LED_BAR_STEP_1ST	400
#define LED_BAR_REDZONE_1ST	( 6800 - LED_BAR_STEP )
#define LED_BAR_STEP		200
#define LED_BAR_REDZONE		( 6800 - LED_BAR_STEP )

#define BLINK_RATE	( 1 << 4 )	// �u�����NRate

//#define EOL		"\n"
#define EOL		"\r\n"

// LED �\���֌W
enum {
	MODE_LAPTIME,
	MODE_ZERO_FOUR,
	MODE_ZERO_ONE,
	MODE_ZERO_ONE_WAIT,	// 0-100 �S�[���҂�
};

enum {	// g_Flags �� bit���ɒ���
	DISPMODE_OPENING,
	DISPMODE_MSG,
	DISPMODE_MSG_LOOP,
	DISPMODE_MSG_PC,
	DISPMODE_ANIME_PC,
	DISPMODE_SPEED,
	DISPMODE_TACHO,
	DISPMODE_MILEAGE,
	DISPMODE_IR,
};

// �M�A�E�^�R�\���֌W
enum {
	GM_TOWN,			// 0 town ���[�h�̃^�R�o�[
	GM_CIRCUIT,			// 1 �T�[�L�b�g���[�h�̃^�R�o�[
	GM_BL_MAIN,			// 2 +���u���~�b�g���� Main �u�����N
	GM_GEAR,			// 3 +�M�A�x��
	GM_DESIRED_GEAR,	// 4 +�œK�M�A�\��
};

#define DISP_FINISH		0xFFFF
#define DBL_CLICK_TIME	8

/*** new type ***************************************************************/

typedef struct {
	UCHAR	uDispModeNext	:4;
	UCHAR	uLapMode		:2;
	BOOL	bBlinkMain		:1;
	BOOL	bBlinkSub		:1;
	BOOL	bBeep			:1;
	BOOL	bNewLap			:1;
	UCHAR	uGearMode		:5;
	UCHAR	uDispMode		:5;
} Flags_t;

typedef struct {
	UCHAR	uPrev		:4;
	UCHAR	uTrig		:4;
} PushSW_t;

typedef struct {
	union {
		ULONG	dw;
		struct	{ UINT	h, l; } w;
	} Time;					// timre W �̃J�E���g
	
	union {
		ULONG	dw;
		struct	{ UINT	h, l; } w;
	} PrevTime;				// �O��� timre W �̃J�E���g
	
	UINT	uPulseCnt;		// �p���X���͉�
	UINT	uVal;			// �X�s�[�h���̌v�Z�l
} PULSE;

typedef union {
	ULONG	dw;
	struct	{ UINT	h, l; } w;
} UNI_LONG;

typedef union {
	ULONG	lDisp;
	UCHAR	cDisp[ 4 ];
} VRAM;

typedef struct {
	UCHAR	uPushElapsed;
	UCHAR	uPushCnt;
} TouchPanel_t;

typedef struct{
	ULONG	uTacho;
	ULONG	uSpeed;
	ULONG	uGx, uGy;
	UINT	uPrevGx;
	UINT	uCnt;
} DispVal_t;

/*** extern *****************************************************************/
#endif
