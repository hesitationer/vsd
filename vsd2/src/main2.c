/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	speed.c -- speed, tacho, ���b�v�Z���T
	
*****************************************************************************/

#include "dds.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "main2.h"

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

/*** ������ *****************************************************************/
// { TIM3, TIM2 } �� 32bit �^�C�}�Ƃ��ĘA������

void PulseInit( void ){
}

#if 0
/*** �X�s�[�h�p���X *********************************************************/

PULSE_t	g_Speed;

void EXTI0_IRQHandler( void ){
	g_Speed.uLastTime	= GetCurrentTime();
	++g_Speed.uPulseCnt;
}

/*** �^�R�p���X *************************************************************/

PULSE_t	g_Tacho;

void EXTI0_IRQHandler( void ){
	g_Tacho.uLastTime	= GetCurrentTime();
	++g_Tacho.uPulseCnt;
}

/*** Tacho / Speed �v�Z *****************************************************/

UINT g_uSpeedCalcConst = ( UINT )( 3600.0 * 100.0 / PULSE_PER_1KM * ( 1 << 11 ));

// 0rpm �ɐ؂艺���� EG ��]���̃p���X�� = 200rpm (clk��@16MHz)
#define TACHO_0RPM_TH	(( ULONG )( H8HZ / ( 200 / 60.0 * 2 )))

// 0km/h �ɐ؂艺���� speed �p���X�� = 1km/h (clk��@16MHz)
#define SPEED_0KPH_TH	(( ULONG )( H8HZ / ( PULSE_PER_1KM / 3600.0 )))

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

void ComputeMeterSpeed( void ){
	ULONG	uPrevTime, uTime;
	UINT	uPulseCnt;
	UINT	uPulseCntTmp;
	
	// �p�����[�^���[�h
	IENR1.BIT.IEN2 = 0;	// Speed IRQ disable
	uTime				= g_Speed.uLastTime;
	uPulseCnt			= g_Speed.uPulseCnt;
	g_Speed.uPulseCnt	= 0;
	IENR1.BIT.IEN2 = 1;	// Speed IRQ enable
	uPrevTime			= g_Speed.uPrevTime;
	
	uPulseCntTmp = uPulseCnt;
	
	// Speed �v�Z
	if( uPulseCnt || g_Speed.uVal ){
		if( uPulseCnt ){
			g_uMileage += uPulseCnt;
			g_Speed.uPrevTime = uTime;
		}else{
			// �p���X�������ĂȂ�������C�p���X��1�񂾂����������̂Ƃ��đ��x�v�Z
			uPulseCnt		= 1;
			uTime			= GetTimerW32();
		}
		
		// �u�M�A�v�Z�Ƃ�.xls�v�Q��
		// 5 = 13(�{���̒萔) - 8(g_uHz �̃V�t�g��)
		g_Speed.uVal = (
			( UINT )(
				((( ULONG )g_uHz * uPulseCnt ) >> 5 ) *
				//( UINT )( 3600.0 * 100.0 / PULSE_PER_1KM * ( 1 << 11 )) /
				g_uSpeedCalcConst /
				(( uTime - uPrevTime ) >> 2 )
			) +
			g_Speed.uVal
		) >> 1;
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
	if( g_Flags.uLapMode == MODE_ZERO_ONE_WAIT && g_Speed.uVal >= 10000 ){
		g_IR.uLastTime = GetRTC();
		g_Flags.bNewLap = TRUE;
		g_Flags.uLapMode = MODE_LAPTIME;
	}
}

#endif
