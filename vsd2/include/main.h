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
#define PULSE_PER_1KM	(( double )15473.76689 )	// CE28N
//#define PULSE_PER_1KM	(( double )14958.80127 )	// �m�[�}��

#define SRAM_TOP	0x20000000
#define SRAM_END	0x20005000

// AD �ϊ��@���W�X�^
#define	G_SENSOR_X		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_1 )
#define	G_SENSOR_Y		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_2 )
#define	G_SENSOR_Z		ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_3 )
#define	ADC_THROTTLE	ADC_GetInjectedConversionValue( ADC1, ADC_InjectedChannel_4 )
//#define	ADC_BRAKE	

// G �Z���T�� 1G �l
#define GX_1G			5400
#define GY_1G			5400
#define GZ_1G			5400

#define LedOn()		( GPIOC->ODR |= 0x40 )
#define LedOff()	( GPIOC->ODR &= ~0x40 )
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
	
	USHORT	uRemainedMileage;				// ���b�v�J�n�܂ł̋���
	USHORT	uMileage_0_400;					// 0-400m �̃p���X��
	
	UCHAR	uCalibTimer;					// �L�����u���[�V�����^�C�}�[
	UCHAR	Reserved1;						// ��ROM ���̎�����
	UCHAR	uLogHz;							// ���O Hz
	
} VSD_DATA_t;

/*** prototype **************************************************************/

void EXTI0_IRQHandler( void );
void EXTI1_IRQHandler( void );
void EXTI2_IRQHandler( void );

/*** extern *****************************************************************/

extern VSD_DATA_t	*g_pVsd;

/*** gloval vars ************************************************************/

#endif	// _MAIN2_H
