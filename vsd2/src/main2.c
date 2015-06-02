/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	speed.c -- speed, tacho, ���b�v�Z���T
	
*****************************************************************************/

#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_timer.h"
#include "dds.h"
#include "main2.h"

/*** ������ *****************************************************************/
// { TIM3, TIM2 } �� 32bit �^�C�}�Ƃ��ĘA������

void TimerInit( void ){
	// APB �N���b�N
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE );
	
	TIM_DeInit( TIM2 );
	TIM_DeInit( TIM3 );
	
	// TimeBase ������
	TIM2->APR = 0; TIM3->APR = 0;	// �J�E���^�����[�h�l
	TIM2->PSC = 0; TIM3->PSC = 0;	// �v���X�P�[��
	TIM2->CR2 = ( 2 << 4 );			// TIM2 �X�V�C�x���g��I��
	TIM3->SMCR = ( 1 << 4 ) |		// TIM3 �͓��͂Ƃ��� TIM2 ��I��
				7;					// TRGI rise edge �ŃJ�E���g
	TIM2->CR1 = TIM3->CR1 =
		( 1 << 7 ) |	// auto preload
		1;				// counter enable
}

/*** ���ݎ����擾 *************************************************/

UINT GetCurrentTime( void ){
	UINT uTimeL, uTimeL2, uTimeH;
	
	uTimeL2 = TIM2->CNT;
	do{
		uTimeL = uTimeL2;
		uTimeH = TIM3->CNT;
	}while( uTimeL > ( uTimeL2 = TIM2->CNT ))
	
	return ( uTimeH << 16 ) | uTimeL;
}
/*** ������ *****************************************************************/
// { TIM3, TIM2 } �� 32bit �^�C�}�Ƃ��ĘA������

void PulseInit( void ){
	// APB �N���b�N
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE );
	
	TIM_DeInit( TIM2 );
	TIM_DeInit( TIM3 );
	
	// TimeBase ������
	TIM2->APR = 0; TIM3->APR = 0;	// �J�E���^�����[�h�l
	TIM2->PSC = 0; TIM3->PSC = 0;	// �v���X�P�[��
	TIM2->CR2 = ( 2 << 4 );			// TIM2 �X�V�C�x���g��I��
	TIM3->SMCR = ( 1 << 4 ) |		// TIM3 �͓��͂Ƃ��� TIM2 ��I��
				7;					// TRGI rise edge �ŃJ�E���g
	TIM2->CR1 = TIM3->CR1 =
		( 1 << 7 ) |	// auto preload
		1;				// counter enable
}

typedef struct PULSE_t {
	UINT	uPrevTime;
	UINT	uLastTime;
	USHORT	uPulseCnt;
	USHORT	uVal;
}

/*** �X�s�[�h�p���X *********************************************************/

PULSE_t	SpeedPulse;

void EXTI0_IRQHandler( void ){
	SpeedPulse.uLastTime	= GetCurrentTime();
	++SpeedPulse.uPulseCnt;
}

/*** �^�R�p���X *************************************************************/

PULSE_t	TachoPulse;

void EXTI0_IRQHandler( void ){
	TachoPulse.uLastTime	= GetCurrentTime();
	++TachoPulse.uPulseCnt;
}

