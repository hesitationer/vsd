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
#include "stm32f10x_adc.h"
#include "main2.h"
#include "usart.h"

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

/*** AD *********************************************************************/

void AdcInit( void ){
	GPIO_InitTypeDef	GPIO_InitStructure;
	ADC_InitTypeDef		ADC_InitStructure;
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE );
	
	GPIO_StructInit( &GPIO_InitStructure );
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init( GPIOA, &GPIO_InitStructure );
	
	/* ADC1 Configuration ------------------------------------------------------*/
	ADC_StructInit( &ADC_InitStructure );
	ADC_InitStructure.ADC_Mode					= ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode			= ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode 	= DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv   	= ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign          	= ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel       	= 1;
	ADC_Init( ADC1, &ADC_InitStructure );
	ADC_ExternalTrigInjectedConvConfig( ADC1, ADC_ExternalTrigInjecConv_None );
	
	/* ADC1 regular channel14 configuration to sample time = 55.5 cycles */
	ADC_InjectedSequencerLengthConfig( ADC1, 4 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_15, 1, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_14, 2, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_4,  3, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_13, 4, ADC_SampleTime_55Cycles5 );
	
	/* Enable ADC1  */
	ADC_Cmd( ADC1, ENABLE );
	
	/* Enable ADC1 reset calibaration register */  
	ADC_ResetCalibration( ADC1 );
	
	/* Check the end of ADC1 reset calibration register */
	while( ADC_GetResetCalibrationStatus( ADC1 ));
	
	/* Start ADC1 calibaration */
	ADC_StartCalibration( ADC1 );
	/* Check the end of ADC1 calibration */
	while( ADC_GetCalibrationStatus( ADC1 ));
}

/*** ADC �ϊ� ***************************************************************/

void AdcConversion( void ){
	// Start ADC1 Software Conversion
	ADC_SoftwareStartInjectedConvCmd( ADC1, ENABLE );
	
	// Wait until conversion completion
	while( ADC_GetFlagStatus( ADC1, ADC_FLAG_JEOC ) == RESET );
}

/*** �X�s�[�h�E�^�R�E���C�Z���T�[ ������ ************************************/
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
#define TACHO_0RPM_TH	(( UINT )( TIMER_HZ / ( 200 / 60.0 * 2 )))

// 0km/h �ɐ؂艺���� speed �p���X�� = 1km/h (clk��@16MHz)
#define SPEED_0KPH_TH	(( UINT )( TIMER_HZ / ( PULSE_PER_1KM / 3600.0 )))

void ComputeMeterTacho( void ){
	ULONG	uPrevTime, uTime;
	UINT	uPulseCnt;
	
	// �p�����[�^���[�h
	NvicIntDisable( EXTI1_IRQChannel );	// Tacho IRQ disable
	uTime				= g_Tacho.uLastTime;
	uPulseCnt			= g_Tacho.uPulseCnt;
	g_Tacho.uPulseCnt	= 0;
	NvicIntEnable( EXTI1_IRQChannel );	// Tacho IRQ enable
	uPrevTime			= g_Tacho.uPrevTime;
	
	// Tacho �v�Z
	if( uPulseCnt ){
		g_Tacho.uPrevTime = uTime;
		g_Tacho.uVal = TIMER_HZ * 30 * uPulseCnt / (( uTime - uPrevTime ) & 0xFFFF );
	}else if(
		uTime = GetCurrentTime16(),
		// 0.2�b�� ��0�� (0.2s = 150rpm)
		(( uTime - uPrevTime ) & 0xFFFF ) >= TACHO_0RPM_TH
	){
		g_Tacho.uVal		= 0;
		g_Tacho.uPrevTime	= uTime - TACHO_0RPM_TH;
	}
}

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
		
		if( uPulseCnt ){
			g_uMileage += uPulseCnt;
			g_Speed.uPrevTime = uTime;
		}else{
			// �p���X�������ĂȂ�������C�p���X��1�񂾂����������̂Ƃ��đ��x�v�Z
			uPulseCnt		= 1;
			uTime			= GetCurrentTime16();
		}
		
		g_Speed.uVal = ( UINT )(
			TIMER_HZ * 3600.0 * 100 / PULSE_PER_1KM
		) * uPulseCnt / (( uTime - uPrevTime ) & 0xFFFF );
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

/*** S ���R�[�h���[�_ *******************************************************/

#ifndef EXEC_SRAM
static __noreturn void JumpTo( u32 uJmpAddr, u32 uSP ){
	asm( "MSR MSP, r1\nBX r0\n" );
}

UINT GetHex( UINT uBytes ){
	
	uBytes <<= 1;;
	UINT	uRet = 0;
	UINT	c;
	
	do{
		uRet <<= 4;
		c = UsartGetcharWaitUnbuffered();
		
		if( '0' <= c && c <= '9' )	uRet |= c - '0';
		else						uRet |= c - ( 'A' - 10 );
	}while( --uBytes );
	
	//DbgMsg(( "%02X ", uRet ));
	return uRet;
}

__noreturn void LoadSRecordSub( void ){
	
	UINT	uAddr, uLen;
	UINT	c;
	
	while( 1 ){
		// 'S' �܂ŃX�L�b�v
		while( UsartGetcharWaitUnbuffered() != 'S' );
		
		// �I���w�b�_�Ȃ� break;
		if(( c = UsartGetcharWaitUnbuffered()) == '7' ) break;
		
		if( c == '3' ){
			// �f�[�^����������
			uLen	= GetHex( 1 ) - 5;
			uAddr	= GetHex( 4 );
			
			DbgMsg(( "Addr:%X Len:%X\n", uAddr, uLen ));
			while( uLen-- ) *( UCHAR *)( uAddr++ ) = GetHex( 1 );
		}
	}
	
	UsartPutstrUnbuffered( "starting program\n" );
	JumpTo( *( u32 *)0x20000004, *( u32 *)0x08003000 );
}

__noreturn void LoadSRecord( void ){
	// �S���荞�݋֎~
	NVIC->ICER[ 0 ] = -1;
	NVIC->ICER[ 1 ] = -1;
	
	UsartInit( USART_BAUDRATE, NULL );
	UsartPutstrUnbuffered( "\nWaiting for S record...\n" );
	JumpTo(( u32 )LoadSRecordSub, SRAM_END );
}
#endif

/*** �o�C�i���o�� ***********************************************************/

void SerialOutchar( UINT c ){
	if( c == 0xFE ){
		putchar( 0xFE ); putchar( 0x00 );
	}else if( c == 0xFF ){
		putchar( 0xFE ); putchar( 0x01 );
	}else{
		putchar( c );
	}
}
void SerialPack( UINT uVal, UINT uBytes ){
	do{
		SerialOutchar( uVal & 0xFF );
		uVal >>= 8;
	}while( --uBytes );
}

/*** �V���A���o�� ***********************************************************/

#if 0
void OutputSerial( void ){
	SerialPack( g_Tacho.uVal, 2 );
	SerialPack( g_Speed.uVal, 2 );
	SerialPack( g_uMileage, 2 );
	//SerialPack(( TA.TCA << 8 ) | g_IR.uVal & 0xFF, 2 );
	//SerialPack( g_DispVal.uGy, 2 );
	//SerialPack( g_DispVal.uGx, 2 );
	SerialPack( 0, 6 );
	
	/*** ���b�v�^�C���\�� ***/
	if( g_Flags.bNewLap ){
		g_Flags.bNewLap = FALSE;
		SerialPack( g_uLapTime, 4 );
	}
	
	putchar( 0xFF );
}

/*** �V���A������ ***********************************************************/

UINT g_uParam;

void DoInputSerial( char c ){
	
	if( 'A' <= c && c <= 'F' ){
		g_uParam = ( g_uParam << 4 ) + c - 'A' + 10;
	}else if( '0' <= c && c <= '9' ){
		g_uParam = ( g_uParam << 4 ) + c - '0';
	}else if( !g_Flags.bOpenCmd ){
		if( c == '*' && g_uParam == 0xF15EF117 ) g_Flags.bOpenCmd = 1;
	}else{
		switch( c ){
			case 'l': g_Flags.uLapMode	= MODE_LAPTIME;		g_uRemainedMillage = 0;
			Case 'g': g_Flags.uLapMode	= MODE_LAPTIME;		g_uRemainedMillage = g_uParam;
			Case 'f': g_Flags.uLapMode	= MODE_ZERO_FOUR;	g_uRemainedMillage = 1; g_uStartGTh = g_uParam;
			Case 'o': g_Flags.uLapMode	= MODE_ZERO_ONE;	g_uRemainedMillage = 1; g_uStartGTh = g_uParam;
			Case 'c': g_uVideoCaribCnt = 96;	// �L�����u���[�V����
			Case 'z': GoMonitor();
			Case 'S': g_Flags.bOutputSerial = g_uParam;
			Case 'P': g_cLEDBar = g_uParam;
		}
		g_uParam = 0;
	}
}
#endif
