/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	timer.h -- timer
	
*****************************************************************************/

#ifndef _MAIN2_H
#define _MAIN2_H

#include "dds.h"

/*** macros *****************************************************************/

#define TIMER_HZ	200000
#define LOG_HZ		16

#define USART_BAUDRATE	38400

// ���Ԃ�C�z�C�������30�p���X
#define PULSE_PER_1KM_NORMAL	(( double )14958.80127 )	// �m�[�}��
#define PULSE_PER_1KM			(( double )15473.76689 )	// CE28N

#define SRAM_TOP	0x20000000
#define SRAM_END	0x20005000

// AD �ϊ��@���W�X�^
#define	G_SENSOR_X		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_1 )
#define	G_SENSOR_Y		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_2 )
#define	G_SENSOR_Z		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_3 )
#define	ADC_THROTTLE	ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_4 )
//#define	ADC_BRAKE	

#define LedOn()		( GPIOC->ODR |= 0x40 )
#define LedOff)		( GPIOC->ODR &= ~0x40 )
#define LedToggle()	( GPIOC->ODR ^= 0x40 )

/*** const ******************************************************************/

enum {
	MODE_LAPTIME,
	MODE_ZERO_FOUR,
	MODE_ZERO_ONE,
	MODE_ZERO_ONE_WAIT,	// 0-100 �S�[���҂�
};

/*** new type ***************************************************************/

typedef struct {
	volatile USHORT	uPulseCnt;
	volatile USHORT	uLastTime;
	USHORT	uPrevTime;
	USHORT	uVal;
} PULSE_t;

typedef struct {
	UINT	uLapTime;						// ���b�v�^�C��
	UINT	uInputParam;					// �V���A�����͒l
	UINT	uComputeMeterConst;				// �X�s�[�h�v�Z�萔
	
	// ��l�߂Ȃ̂ŉ��ɒǋL���� ���ق�܂�?
	struct {
		UCHAR	uLapMode		:2;
		
		BOOL	bNewLap			:1;
		BOOL	bConnected		:1;
	} Flags;
	
	PULSE_t	Tacho;							// �X�s�[�h�p���X
	PULSE_t	Speed;							// �^�R�p���X
	USHORT	uMileage;						// ���s����
	USHORT	uGx, uGy;						// G
	USHORT	uThrottle;						// �A�N�Z���y�_��
	
	USHORT	uStartGTh;						// ���i G �����X���b�V�����h
	USHORT	uOutputPrevTime;				// �V���A���o�� prev time
	
	USHORT	uRemainedMillage;				// ���b�v�J�n�܂ł̋���
	USHORT	uMillage_0_400;					// 0-400m �̃p���X��
	
	UCHAR	uCaribTimer;					// �L�����u���[�V�����^�C�}�[
	UCHAR	uConnectWDT;					// Android �ڑ� WDT
	UCHAR	uLogHz;							// ���O Hz
	
} VSD_DATA_t;

/*** prototype **************************************************************/

void NvicIntEnable( UINT IRQChannel );
void NvicIntDisable( UINT IRQChannel );
UINT GetHex( UINT uBytes );
__noreturn void LoadSRecord( void );
void TimerInit( void );
UINT GetCurrentTime( void );
UINT GetCurrentTime16( void );
void AdcInit( void );
void AdcConversion( void );
void PulseInit( void );
void EXTI0_IRQHandler( void );
void EXTI1_IRQHandler( void );
void EXTI2_IRQHandler( void );
void ComputeMeterTacho( VSD_DATA_t *pVsd );
void ComputeMeterSpeed( VSD_DATA_t *pVsd );
void SerialOutchar( UINT c );
void SerialPack( UINT uVal, UINT uBytes );
void OutputSerial( VSD_DATA_t *pVsd );
void InputSerial( VSD_DATA_t *pVsd, char c );
void Calibration( VSD_DATA_t *pVsd );
void WaitStateChange( VSD_DATA_t *pVsd );

/*** extern *****************************************************************/

extern VSD_DATA_t	*g_pVsd;

/*** gloval vars ************************************************************/

#endif	// _MAIN2_H
