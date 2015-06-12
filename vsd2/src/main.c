/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	main.c -- ���C��
	
*****************************************************************************/

#include "dds.h"
#include <stdio.h>
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_nvic.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_iwdg.h"
#include "hw_config.h"
#include "main.h"
#include "usart.h"

/*** gloval vars ************************************************************/

#ifndef EXEC_SRAM
BOTTOM VSD_DATA_t	*g_pVsd;
#endif

/*** NVIC IE/ID *************************************************************/

INLINE void NvicIntEnable( UINT IRQChannel ){
	NVIC->ISER[(IRQChannel >> 0x05)] = (u32)0x01 << (IRQChannel & (u8)0x1F);
}

INLINE void NvicIntDisable( UINT IRQChannel ){
	NVIC->ICER[(IRQChannel >> 0x05)] = (u32)0x01 << (IRQChannel & (u8)0x1F);
}

/*** S ���R�[�h���[�_ *******************************************************/

#ifndef EXEC_SRAM
INLINE __noreturn void JumpTo( u32 uJmpAddr, u32 uSP ){
	asm( "MSR MSP, r1\nBX r0\n" );
}

// LoadS ��p�� GetcharWait(), z �ł�蒼��
int LoadSGetcharWait(){
	WdtReload();
	int c = UsartGetcharWaitUnbuffered();
	if( c == 'z' ) LoadSRecord();
	return c;
}

UINT GetHex( UINT uBytes ){
	
	uBytes <<= 1;;
	UINT	uRet = 0;
	UINT	c;
	
	do{
		uRet <<= 4;
		c = LoadSGetcharWait();
		
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
		while( LoadSGetcharWait() != 'S' );
		
		// �I���w�b�_�Ȃ� break;
		if(( c = LoadSGetcharWait()) == '7' ) break;
		
		if( c == '3' ){
			// �f�[�^����������
			uLen	= GetHex( 1 ) - 5;
			uAddr	= GetHex( 4 );
			
			DbgMsg(( "Addr:%X Len:%X\n", uAddr, uLen ));
			while( uLen-- ) *( UCHAR *)( uAddr++ ) = GetHex( 1 );
		}
	}
	
	// ':' �� vsdroid ���̂��߂ɕK�v
	UsartPutstrUnbuffered( "starting program:::\n" );
	JumpTo( GetHex( 5 ), INIT_SP );
}

__noreturn void LoadSRecord( void ){
	// �S���荞�݋֎~
	NVIC->ICER[ 0 ] = -1;
	NVIC->ICER[ 1 ] = -1;
	
	LedOn();
	
	UsartInit( USART_BAUDRATE, NULL );
	UsartPutstrUnbuffered( "\nWaiting for S record:\n" );
	JumpTo(( u32 )LoadSRecordSub, SRAM_END );
}

// �o�C�i�����[�_
__noreturn void LoadBinSub( void ){
	UINT uCnt;
	UINT uSize = UsartGetcharWaitUnbuffered() | ( UsartGetcharWaitUnbuffered() << 8 );
	
	DbgMsg(( "\nloading %d bytes\n", uSize ));
	
	for( uCnt = 0; uCnt < uSize; ++uCnt ){
		*( UCHAR *)( SRAM_TOP + uCnt ) = UsartGetcharWaitUnbuffered();
	}
	
	UsartPutstrUnbuffered( "starting program:::\n" );
	JumpTo( *( u32 *)SRAM_TOP, INIT_SP );
}

__noreturn void LoadBin( void ){
	// �S���荞�݋֎~
	NVIC->ICER[ 0 ] = -1;
	NVIC->ICER[ 1 ] = -1;
	
	LedOn();
	
	UsartInit( USART_BAUDRATE, NULL );
	UsartPutstrUnbuffered( "\nWaiting for bin file:\n" );
	JumpTo(( u32 )LoadBinSub, SRAM_END );
}
#endif

/*** ������ *****************************************************************/
// { TIM3, TIM2 } �� 32bit �^�C�}�Ƃ��ĘA������

#ifndef EXEC_SRAM
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
#endif

/*** ���ݎ����擾 *************************************************/

#ifndef EXEC_SRAM
UINT GetCurrentTime( void ){
	UINT uTimeL = TIM2->CNT;
	UINT uTimeH = TIM3->CNT;
	
	if( uTimeL > TIM2->CNT ){
		uTimeH = TIM3->CNT - 1;
	}
	return ( uTimeH << 16 ) | uTimeL;
}

INLINE UINT GetCurrentTime16( void ){
	return TIM2->CNT;
}
#endif

/*** AD *********************************************************************/

#ifndef EXEC_SRAM
void AdcInit( void ){
	GPIO_InitTypeDef	GPIO_InitStruct;
	ADC_InitTypeDef		ADC_InitStructure;
	
	ADC_DeInit( ADC1 );
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE );
	
	GPIO_StructInit( &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init( GPIOC, &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init( GPIOC, &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init( GPIOC, &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init( GPIOA, &GPIO_InitStruct );
	
	// G �Z���T�t���X�P�[���ݒ� (PA8)
	GPIO_StructInit( &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOA, &GPIO_InitStruct );
	//GPIOA->ODR |= ( 1 << 8 );
	
	// ADC1 Configuration ------------------------------------------------------
	ADC_StructInit( &ADC_InitStructure );
	ADC_InitStructure.ADC_Mode					= ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode			= ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode 	= DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv   	= ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign          	= ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel       	= 1;
	ADC_Init( ADC1, &ADC_InitStructure );
	ADC_ExternalTrigInjectedConvConfig( ADC1, ADC_ExternalTrigInjecConv_None );
	
	// ADC1 regular channel14 configuration to sample time = 55.5 cycles
	ADC_InjectedSequencerLengthConfig( ADC1, 4 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_4 , 1, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_14, 2, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_15, 3, ADC_SampleTime_55Cycles5 );
	ADC_InjectedChannelConfig( ADC1, ADC_Channel_13, 4, ADC_SampleTime_55Cycles5 );
	
	// Enable ADC1 
	ADC_Cmd( ADC1, ENABLE );
	
	// Enable ADC1 reset calibaration register
	ADC_ResetCalibration( ADC1 );
	
	// Check the end of ADC1 reset calibration register
	while( ADC_GetResetCalibrationStatus( ADC1 ));
	
	// Start ADC1 calibaration
	ADC_StartCalibration( ADC1 );
	// Check the end of ADC1 calibration
	while( ADC_GetCalibrationStatus( ADC1 ));
}
#endif

/*** ADC �ϊ� ***************************************************************/

#ifndef zzzEXEC_SRAM
INLINE void AdcConversion( void ){
	// Start ADC1 Software Conversion
	ADC_SoftwareStartInjectedConvCmd( ADC1, ENABLE );
}

INLINE BOOL AdcConversionCompleted( void ){
	return ADC_GetFlagStatus( ADC1, ADC_FLAG_JEOC ) == SET;
}
#endif

/*** �X�s�[�h�E�^�R�E���C�Z���T�[ ������ ************************************/
// PD0: speed
// PD1: tacho
// PD2: ���C�Z���T�[

#ifndef zzzEXEC_SRAM
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
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;	// pull up
	GPIO_Init( GPIOD, &GPIO_InitStruct );
	
	// GPIO ���荞�݃C�l�[�u��
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource0 );
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource1 );
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOD, GPIO_PinSource2 );
	EXTI->IMR  |=  0x7;
	EXTI->RTSR |=  0x3;	// ���C�Z���T [2] �̂� fall edge
	EXTI->FTSR &= ~0x3;
	
	// NVIC �ݒ�
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQChannel;
	NVIC_Init( &NVIC_InitStructure );
}
#endif

/*** �X�s�[�h�p���X *********************************************************/

#ifndef EXEC_SRAM
void EXTI0_IRQHandler( void ){
	EXTI->PR = 1 << 0;
	g_pVsd->Speed.uLastTime	= GetCurrentTime16();
	++g_pVsd->Speed.uPulseCnt;
	
	// Millage �������e������������CNewLap�N��
	if( g_pVsd->uRemainedMileage && !--g_pVsd->uRemainedMileage ){
		g_pVsd->uLapTime = GetCurrentTime();
		g_pVsd->Flags.bNewLap = TRUE;
		
		if( g_pVsd->Flags.uLapMode == MODE_ZERO_FOUR ){
			// 0-400���[�h�Ȃ�C������400m�ɐݒ�
			g_pVsd->uRemainedMileage = g_pVsd->uMileage_0_400;
			g_pVsd->Flags.uLapMode = MODE_LAPTIME;
		}else if( g_pVsd->Flags.uLapMode == MODE_ZERO_ONE ){
			// 0-100 ���[�h�Ȃ�C0-100�S�[���҂����[�h�Ɉڍs
			g_pVsd->Flags.uLapMode = MODE_ZERO_ONE_WAIT;
		}
	}
}
#endif

/*** �^�R�p���X *************************************************************/

#ifndef EXEC_SRAM
void EXTI1_IRQHandler( void ){
	EXTI->PR = 1 << 1;
	g_pVsd->Tacho.uLastTime	= GetCurrentTime16();
	++g_pVsd->Tacho.uPulseCnt;
}
#endif

/*** ���C�Z���T *************************************************************/

#ifndef EXEC_SRAM
void EXTI2_IRQHandler( void ){
	UINT	uNowTime = GetCurrentTime();
	
	EXTI->PR = 1 << 2;
	
	// ���O�� NewLap ���� 3�b�ȏ゠���Ă���
	if( uNowTime - g_pVsd->uLapTime >= 3 * TIMER_HZ ){
		g_pVsd->uLapTime = uNowTime;
		g_pVsd->Flags.bNewLap = TRUE;
	}
}
#endif

/*** Tacho / Speed �v�Z *****************************************************/

// 0rpm �ɐ؂艺���� EG ��]���̃p���X�� = 200rpm (clk��@16MHz)
#define TACHO_0RPM_TH	(( UINT )( TIMER_HZ / ( 200 / 60.0 * 2 )))

#ifndef EXEC_SRAM
void ComputeMeterTacho( VSD_DATA_t *pVsd ){
	UINT	uPrevTime, uTime;
	UINT	uPulseCnt;
	
	// �p�����[�^���[�h
	NvicIntDisable( EXTI1_IRQChannel );	// Tacho IRQ disable
	uTime				= pVsd->Tacho.uLastTime;
	uPulseCnt			= pVsd->Tacho.uPulseCnt;
	pVsd->Tacho.uPulseCnt	= 0;
	NvicIntEnable( EXTI1_IRQChannel );	// Tacho IRQ enable
	uPrevTime			= pVsd->Tacho.uPrevTime;
	
	// Tacho �v�Z
	if( uPulseCnt ){
		pVsd->Tacho.uPrevTime = uTime;
		pVsd->Tacho.uVal = TIMER_HZ * 30 * uPulseCnt / (( uTime - uPrevTime ) & 0xFFFF );
	}else if(
		uTime = GetCurrentTime16(),
		// 0.2�b�� ��0�� (0.2s = 150rpm)
		(( uTime - uPrevTime ) & 0xFFFF ) >= TACHO_0RPM_TH
	){
		pVsd->Tacho.uVal		= 0;
		pVsd->Tacho.uPrevTime	= uTime - TACHO_0RPM_TH;
	}
}
#endif

// 0km/h �ɐ؂艺���� speed �p���X�� = 1km/h (clk��@16MHz)
#define SPEED_0KPH_TH	(( UINT )( TIMER_HZ / ( PULSE_PER_1KM / 3600.0 )))

#ifndef EXEC_SRAM
void ComputeMeterSpeed( VSD_DATA_t *pVsd ){
	UINT	uTime;
	UINT	uPulseCnt;
	UINT	uPulseCntTmp;
	UINT	uSpeed = 0;
	
	// �p�����[�^���[�h
	NvicIntDisable( EXTI0_IRQChannel );	// Speed IRQ disable
	uTime				= pVsd->Speed.uLastTime;
	uPulseCnt			= pVsd->Speed.uPulseCnt;
	pVsd->Speed.uPulseCnt	= 0;
	NvicIntEnable( EXTI0_IRQChannel );	// Speed IRQ disable
	
	uPulseCntTmp = uPulseCnt;
	
	// Speed �v�Z
	if( uPulseCnt || pVsd->Speed.uVal ){
		UINT uPrevTime = pVsd->Speed.uPrevTime;
		
		if( uPulseCnt ){
			pVsd->uMileage += uPulseCnt;
			pVsd->Speed.uPrevTime = uTime;
		}else{
			// �p���X�������ĂȂ�������C�p���X��1�񂾂����������̂Ƃ��đ��x�v�Z
			uPulseCnt		= 1;
			uTime			= GetCurrentTime16();
		}
		
		uSpeed = pVsd->uComputeMeterConst * uPulseCnt / (( uTime - uPrevTime ) & 0xFFFF );
		
		// 0-100�S�[���҂����[�h��100km/h�ɒB������NewLap�N��
		if( pVsd->Flags.uLapMode == MODE_ZERO_ONE_WAIT && uSpeed >= 10000 ){
			pVsd->uLapTime		    = GetCurrentTime();
			pVsd->Flags.bNewLap		= TRUE;
			pVsd->Flags.uLapMode	= MODE_LAPTIME;
		}
	}
	
	if( uPulseCntTmp ){
		// �p���X���������Ƃ��͕K�� 1km/h �ȏ�
		if( uSpeed < 100 ) uSpeed = 100;
	}else if( uSpeed < 100 ){
		// �p���X�Ȃ� && 1km/h ������ 0km/h ����
		uSpeed = 0;
		pVsd->Speed.uPrevTime = uTime - SPEED_0KPH_TH;
	}else if( uSpeed > pVsd->Speed.uVal ){
		// �p���X�Ȃ��ŁC�O��̃X�s�[�h��葬��������
		// �O��X�s�[�h�̂܂܂ɂ���
		return;
	}
	
	pVsd->Speed.uVal = uSpeed;
}
#endif

/*** SD �J�[�h (���� card detect �X�C�b�`�̂� ) *****************************/

#ifndef EXEC_SRAM
void SdcInit( void ){
	// APB �N���b�N
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC, ENABLE );
	
	// GPIO�ݒ�
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_StructInit( &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init( GPIOC, &GPIO_InitStruct );
}

INLINE UINT SdcInserted( void ){
	return !( GPIOC->IDR & ( 1 << 9 ));
}
#endif

/*** WDT ********************************************************************/

#ifndef zzzEXEC_SRAM
void WdtInitSub( UINT uMillisec, UINT uPsc ){
	IWDG_WriteAccessCmd( IWDG_WriteAccess_Enable );
	IWDG_SetPrescaler( uPsc );
	IWDG_SetReload( uMillisec );
	IWDG_Enable();
}

INLINE void WdtInit( UINT ms ){
	UINT uCnt = ms * 40;
	UINT uPsc;
	
	if     ( uCnt >= ( 0x1000 << 6 )){ uCnt >>= 7; uPsc = 7; }
	else if( uCnt >= ( 0x1000 << 5 )){ uCnt >>= 6; uPsc = 6; }
	else if( uCnt >= ( 0x1000 << 4 )){ uCnt >>= 5; uPsc = 5; }
	else if( uCnt >= ( 0x1000 << 3 )){ uCnt >>= 4; uPsc = 4; }
	else if( uCnt >= ( 0x1000 << 2 )){ uCnt >>= 3; uPsc = 3; }
	else if( uCnt >= ( 0x1000 << 1 )){ uCnt >>= 2; uPsc = 2; }
	else if( uCnt >= ( 0x1000 << 0 )){ uCnt >>= 1; uPsc = 1; }
	else                             {             uPsc = 0; }
	
	WdtInitSub( uCnt, uPsc );
}

INLINE void WdtReload( void ){
	IWDG_ReloadCounter();
}
#endif

/*** �o�C�i���o�� ***********************************************************/

#ifndef EXEC_SRAM
INLINE void SerialOutchar( UINT c ){
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
#endif

/*** �V���A���o�� ***********************************************************/

#ifndef EXEC_SRAM
void OutputSerial( VSD_DATA_t *pVsd ){
	SerialPack( pVsd->Tacho.uVal, 2 );
	SerialPack( pVsd->Speed.uVal, 2 );
	SerialPack( pVsd->uMileage, 2 );
	SerialPack( GetCurrentTime() >> 10, 2 );	// �Ă��Ƃ�
	SerialPack( pVsd->uGy, 2 );
	SerialPack( pVsd->uGx, 2 );
	
	/*** ���b�v�^�C���\�� ***/
	if( pVsd->Flags.bNewLap ){
		pVsd->Flags.bNewLap = FALSE;
		SerialPack( pVsd->uLapTime, 4 );
	}
	
	putchar( 0xFF );
}
#endif

/*** �V���A������ ***********************************************************/

#ifndef EXEC_SRAM
void InputSerial( VSD_DATA_t *pVsd ){
	
	UINT c = getchar();
	if( c == EOF ) return;
	
	WdtReload();
	
	if( 'A' <= c && c <= 'F' ){
		pVsd->uInputParam = ( pVsd->uInputParam << 4 ) + c - ( 'A' - 10 );
	}else if( '0' <= c && c <= '9' ){
		pVsd->uInputParam = ( pVsd->uInputParam << 4 ) + c - '0';
	}else if( !pVsd->Flags.bConnected ){
		if( c == '*' && pVsd->uInputParam == 0xF15EF117 ) pVsd->Flags.bConnected = 1;
		else pVsd->uInputParam = 0;
	}else{
		switch( c ){
			case 'l': pVsd->Flags.uLapMode	= MODE_LAPTIME;		pVsd->uRemainedMileage = 0;
			Case 'g': pVsd->Flags.uLapMode	= MODE_LAPTIME;		pVsd->uRemainedMileage = pVsd->uInputParam;
			Case 'f': pVsd->Flags.uLapMode	= MODE_ZERO_FOUR;	pVsd->uRemainedMileage = 1; pVsd->uStartGTh = pVsd->uInputParam;
			Case 'o': pVsd->Flags.uLapMode	= MODE_ZERO_ONE;	pVsd->uRemainedMileage = 1; pVsd->uStartGTh = pVsd->uInputParam;
			Case 'c': pVsd->uCalibTimer = 6 * pVsd->uLogHz;	// �L�����u���[�V����
			Case 'z': LoadSRecord();
		}
		pVsd->uInputParam = 0;
	}
}
#endif

/*** G�Z���T�[�ɂ��X�^�[�g���o ********************************************/

#ifndef EXEC_SRAM
void CheckStartByGSensor( VSD_DATA_t *pVsd, UINT uGx ){
	if( pVsd->Flags.uLapMode == MODE_ZERO_FOUR || pVsd->Flags.uLapMode == MODE_ZERO_ONE ){
		
		if(
			(( pVsd->uGx > uGx ) ? ( pVsd->uGx - uGx ) : ( uGx - pVsd->uGx ))
			>= pVsd->uStartGTh
		){
			pVsd->uLapTime = GetCurrentTime();
			pVsd->Flags.bNewLap = TRUE;
			
			if( pVsd->Flags.uLapMode == MODE_ZERO_FOUR ){
				// 0-400���[�h�Ȃ�C������400m�ɐݒ�
				pVsd->uRemainedMileage = pVsd->uMileage_0_400;
				pVsd->Flags.uLapMode = MODE_LAPTIME;
			}else /*if( pVsd->Flags.uLapMode == MODE_ZERO_ONE )*/ {
				// 0-100 ���[�h�Ȃ�C0-100�S�[���҂����[�h�Ɉڍs
				pVsd->Flags.uLapMode = MODE_ZERO_ONE_WAIT;
				pVsd->uRemainedMileage = 0;
			}
		}
	}
}
#endif

/*** �L�����u���[�V���� *****************************************************/

#ifndef EXEC_SRAM
void Calibration( VSD_DATA_t *pVsd ){
	// �L�����u���[�V����
	if( pVsd->uCalibTimer ){
		if( pVsd->uCalibTimer <= 4 * pVsd->uLogHz ){
			pVsd->Speed.uVal	= -1;
			pVsd->Tacho.uVal	= 0;
		}
		--pVsd->uCalibTimer;
	}
}
#endif

/*** ���������� *************************************************************/

#ifndef zzzEXEC_SRAM
INLINE void Initialize( USART_BUF_t *pBuf ){
	#ifndef EXEC_SRAM
		Set_System();
		
		// SD �J�[�h���}������Ă�����C�J���p�ɑ� LoadSRecord()
		SdcInit();
		if( SdcInserted()) LoadSRecord();
	#else
		// �x�N�^�e�[�u���� SRAM �ɍĐݒ�
		//NVIC_SetVectorTable( 0, __vector_table );
	#endif
	
	WdtInit( 3000 );
	LedOff();
	UsartInit( USART_BAUDRATE, pBuf );
	AdcInit();
	TimerInit();
	PulseInit();
}
#endif

/*** �X�e�[�g�ω��҂� & LED �\�� ********************************************/

#ifndef zzzEXEC_SRAM
void WaitStateChange( VSD_DATA_t *pVsd ){
	UINT	uSumGx 		= 0;
	UINT	uSumGy 		= 0;
	UINT	uGy;
	UINT	uThrottle	= 0;
	UINT	uCnt		= 0;
	
	/*** �X�e�[�g�ω��҂� ***/
	UINT	uWaitCnt = TIMER_HZ / pVsd->uLogHz;
	
	// ���O�����҂�
	AdcConversion();
	while((( GetCurrentTime16() - pVsd->uOutputPrevTime ) & 0xFFFF ) < uWaitCnt ){
		if( AdcConversionCompleted()){
			uSumGx		+= G_SENSOR_X;
			uSumGy		+= uGy = G_SENSOR_Y;
			uThrottle	+= ADC_THROTTLE;
			++uCnt;
			
			CheckStartByGSensor( pVsd, uGy );	// G�Z���T�[�ɂ��X�^�[�g���o
			
			// ���̕ϊ��J�n
			ADC1->SR &= ~( 1 << 2 );	// clr JEOC
			AdcConversion();
		}
		
		InputSerial( pVsd );	// serial ����
	}
	pVsd->uOutputPrevTime += uWaitCnt;
	
	// G �̌v�Z
	// ADC �l�� 0-0x7FFF �� 15bit�D
	// G �̌��ʂ� 4.12bit fixed point�D
	pVsd->uGx = ( UINT )(
		(( ULLONG )uSumGx * ( UINT )( 0x100000000L * 4096 * 2 / GX_1G )) >> 32
	) / uCnt + GX_CENTER;
	
	pVsd->uGy = ( UINT )(
		(( ULLONG )uSumGy * ( UINT )( 0x100000000L * 4096 * 2 / GY_1G )) >> 32
	) / uCnt + GY_CENTER;
	
	pVsd->uThrottle	= ( uThrottle << 1 ) / uCnt;
}
#endif

/*** ���C�����[�v ***********************************************************/

//#define MINIMUM_FIRMWARE

__noreturn void main( void ){
#ifdef MINIMUM_FIRMWARE
	NVIC_GenerateSystemReset();
#else
	/*** ������ *************************************************************/
	
	// USART buf
	USART_BUF_t	UsartBuf	= { 0 };
	VSD_DATA_t	Vsd			= { 0 };
	g_pVsd = &Vsd;
	
	Vsd.uComputeMeterConst	= ( UINT )( TIMER_HZ * 3600.0 * 100 / PULSE_PER_1KM );
	Vsd.uMileage_0_400		= ( UINT )( PULSE_PER_1KM * 400 / 1000 + 0.5 );
	Vsd.uLogHz				= LOG_HZ;
	Vsd.uOutputPrevTime		= GetCurrentTime16();
	
	Initialize( &UsartBuf );
	
	/*** ���C�����[�v *******************************************************/
	
	while( 1 ){
		WaitStateChange( &Vsd );
		ComputeMeterSpeed( &Vsd );
		ComputeMeterTacho( &Vsd );
		Calibration( &Vsd );
		
		if( Vsd.Flags.bConnected ){
			OutputSerial( &Vsd );
			LedToggle();
		}else{
			WdtReload();
			LedOn();
		}
	}
#endif
}
