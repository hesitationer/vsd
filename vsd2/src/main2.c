/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	speed.c -- speed, tacho, ���b�v�Z���T
	
*****************************************************************************/

#include "dds.h"
#include <stdio.h>
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_nvic.h"
#include "stm32f10x_tim.h"
#include "main2.h"

/*** NVIC IE/ID *************************************************************/

void NvicIntEnable( UINT IRQChannel ){
	NVIC->ISER[(IRQChannel >> 0x05)] = (u32)0x01 << (IRQChannel & (u8)0x1F);
}

void NvicIntDisable( UINT IRQChannel ){
	NVIC->ICER[(IRQChannel >> 0x05)] = (u32)0x01 << (IRQChannel & (u8)0x1F);
}

/*** ������ *****************************************************************/
// { TIM3, TIM2 } �� 32bit �^�C�}�Ƃ��ĘA������

void TimerInit( void ){
	// APB �N���b�N
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE );
	
	TIM_DeInit( TIM2 );
	TIM_DeInit( TIM3 );
	
	// TimeBase ������
	TIM2->ARR = -1; TIM3->ARR = -1;	// �J�E���^����l
	TIM2->PSC = 72000000 / TIMER_HZ - 1;
	TIM3->PSC = 0;	// �v���X�P�[��
	TIM2->CR2 = ( 2 << 4 );			// TIM2 �X�V�C�x���g��I��
	TIM2->SMCR = 0;
	TIM3->SMCR = ( 1 << 4 ) |		// TIM3 �͓��͂Ƃ��� TIM2 ��I��
				7;					// TRGI rise edge �ŃJ�E���g
	TIM2->EGR = 1; TIM3->EGR = 1;	// UG (�v���X�P�[���l�X�V)
	TIM2->CR1 = TIM3->CR1 =
		( 0 << 7 ) |	// auto preload
		1;				// counter enable
}

/*** ���ݎ����擾 *************************************************/

UINT GetCurrentTime( void ){
	UINT uTimeL, uTimeL2, uTimeH;
	
	uTimeL2 = TIM2->CNT;
	do{
		uTimeL = uTimeL2;
		uTimeH = TIM3->CNT;
	}while( uTimeL > ( uTimeL2 = TIM2->CNT ));
	
	return ( uTimeH << 16 ) | uTimeL;
}

UINT GetCurrentTime16( void ){
	return TIM2->CNT;
}

/*** ������ *****************************************************************/
// PD0: speed
// PD1: tacho
// PD2: ���C�Z���T�[

void PulseInit( void ){
	// APB �N���b�N
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOD, ENABLE );
	
	// GPIO �ݒ�
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit( &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0;
	GPIO_Init( GPIOD, &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_1;
	GPIO_Init( GPIOD, &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_2;
	GPIO_Init( GPIOD, &GPIO_InitStruct );
	
	// GPIO ���荞�݃C�l�[�u��
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource0 );
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource1 );
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource2 );
	EXTI->IMR  |= 0x7;
	EXTI->RTSR |= 0x7;
	EXTI->FTSR &= ~0x7;
	
	// NVIC �ݒ�
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
}

/*** �X�s�[�h�p���X *********************************************************/

UINT	g_uMileage;
PULSE_t	g_Speed;

void EXTI0_IRQHandler( void ){
	EXTI->PR = 1 << 0;
	g_Speed.uLastTime	= GetCurrentTime16();
	++g_Speed.uPulseCnt;
}

/*** �^�R�p���X *************************************************************/

PULSE_t	g_Tacho;

void EXTI1_IRQHandler( void ){
	EXTI->PR = 1 << 1;
	g_Tacho.uLastTime	= GetCurrentTime16();
	++g_Tacho.uPulseCnt;
}

/*** ���C�Z���T *************************************************************/

UINT g_uLapTime;

void EXTI2_IRQHandler( void ){
	EXTI->PR = 1 << 2;
	++g_uLapTime;
}

/*** Tacho / Speed �v�Z *****************************************************/

UINT g_uSpeedCalcConst = ( UINT )( 3600.0 * 100.0 / PULSE_PER_1KM * ( 1 << 11 ));

// 0rpm �ɐ؂艺���� EG ��]���̃p���X�� = 200rpm (clk��@16MHz)
#define TACHO_0RPM_TH	(( UINT )( H8HZ / ( 200 / 60.0 * 2 )))

// 0km/h �ɐ؂艺���� speed �p���X�� = 1km/h (clk��@16MHz)
#define SPEED_0KPH_TH	(( UINT )( TIMER_HZ / ( PULSE_PER_1KM / 3600.0 )))

#if 0
void ComputeMeterTacho( void ){
	ULONG	uPrevTime, uTime;
	UINT	uPulseCnt;
	
	// �p�����[�^���[�h
	IENR1.BIT.IEN2 = 0;	// Tacho IRQ disable
	uTime				= g_Tacho.uLastTime;
	uPulseCnt			= g_Tacho.uPulseCnt;
	g_Tacho.uPulseCnt	= 0;
	IENR1.BIT.IEN2 = 1;	// Tacho IRQ enable
	uPrevTime			= g_Tacho.uPrevTime;
	
	// Tacho �v�Z
	if( uPulseCnt ){
		// 30 = 60[min/sec] / 2[pulse/roll]
		// << 7 �́C���� >> 1 �� g_uHz �� << 8 ���镪
		g_Tacho.uVal = (
			( UINT )(
				( ULONG )g_uHz * (( 30 << 7 ) * uPulseCnt ) /
				(( uTime - uPrevTime ) >> 1 )
			) +
			g_Tacho.uVal
		) >> 1;
		
		g_Tacho.uPrevTime = uTime;
	}else if(
		uTime = GetTimerW32(),
		// 0.2�b�� ��0�� (0.2s = 150rpm)
		uTime - uPrevTime >= TACHO_0RPM_TH
	){
		g_Tacho.uVal		= 0;
		g_Tacho.uPrevTime	= uTime - TACHO_0RPM_TH;
	}
}
#endif

void ComputeMeterSpeed( void ){
	UINT	uTime;
	UINT	uPulseCnt;
	UINT	uPulseCntTmp;
	
	// �p�����[�^���[�h
	NvicIntDisable( EXTI0_IRQChannel );	// Speed IRQ disable
	uTime				= g_Speed.uLastTime;
	uPulseCnt			= g_Speed.uPulseCnt;
	g_Speed.uPulseCnt	= 0;
	NvicIntEnable( EXTI0_IRQChannel );	// Speed IRQ disable
	
	uPulseCntTmp = uPulseCnt;
	
	// Speed �v�Z
	if( uPulseCnt || g_Speed.uVal ){
		UINT uPrevTime = g_Speed.uPrevTime;
		
printf( "%d\n", uPulseCnt );
		if( uPulseCnt ){
			g_uMileage += uPulseCnt;
			g_Speed.uPrevTime = uTime;
		}else{
			// �p���X�������ĂȂ�������C�p���X��1�񂾂����������̂Ƃ��đ��x�v�Z
			uPulseCnt		= 1;
			uTime			= GetCurrentTime16();
		}
		
		UINT uTimeDiff = uTime - uPrevTime;
		if(( int )uTimeDiff < 0 ) uTimeDiff += 0x10000;
		
		g_Speed.uVal = ( UINT )(
			TIMER_HZ * 3600.0 * 100 / PULSE_PER_1KM
		) * uPulseCnt / uTimeDiff;
	}
	
	if( uPulseCntTmp ){
		// �p���X���������Ƃ��͕K�� 1km/h �ȏ�
		if( g_Speed.uVal < 100 ) g_Speed.uVal = 100;
	}else{
		// 1km/h ������ 0km/h ����
		if( g_Speed.uVal < 100 ){
			g_Speed.uVal = 0;
			g_Speed.uPrevTime = uTime - SPEED_0KPH_TH;
		}
	}
	
	// 0-100�S�[���҂����[�h��100km/h�ɒB������NewLap�N��
	#if 0
	// �����Ƃ�
	if( g_Flags.uLapMode == MODE_ZERO_ONE_WAIT && g_Speed.uVal >= 10000 ){
		g_IR.uLastTime = GetRTC();
		g_Flags.bNewLap = TRUE;
		g_Flags.uLapMode = MODE_LAPTIME;
	}
	#endif
}
