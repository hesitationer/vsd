/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	main.c -- main routine
	
*****************************************************************************/

#include "dds.h"
#include <stdio.h>
#include <ST\iostm32f10xxB.h>
#include "stm32f10x_nvic.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "hw_config.h"
#include "main2.h"
#include "usart.h"

/*** macros *****************************************************************/
/*** const ******************************************************************/
/*** new type ***************************************************************/
/*** prototype **************************************************************/
/*** extern *****************************************************************/
/*** gloval vars ************************************************************/
/****************************************************************************/

__noreturn void main( void ){
	/*** ������ *************************************************************/
	
	// USART buf
	USART_BUF_t	UsartBuf	= { 0 };
	
	#ifndef EXEC_SRAM
		Set_System();
	#else
		// �x�N�^�e�[�u���Đݒ�
		NVIC_SetVectorTable( NVIC_VectTab_RAM, 0 );
	#endif
	
	UsartInit( USART_BAUDRATE, &UsartBuf );
	
	AdcInit();
	TimerInit();
	PulseInit();
	
	/*** ���C�����[�v *******************************************************/
	
	VSD_DATA_t	Vsd	= { 0 };
	g_pVsd = &Vsd;
	
	Vsd.uComputeMeterConst	= ( UINT )( TIMER_HZ * 3600.0 * 100 / PULSE_PER_1KM );
	Vsd.uMileage_0_400		= ( UINT )( PULSE_PER_1KM * 400 / 1000 + 0.5 );
	Vsd.uLogHz				= LOG_HZ;
	Vsd.uOutputPrevTime		= GetCurrentTime16();
	
	while( 1 ){
		LedToggle();
		WaitStateChange( &Vsd );
		ComputeMeterSpeed( &Vsd );
		ComputeMeterTacho( &Vsd );
		Calibration( &Vsd );
		
		if( Vsd.Flags.bConnected ){
			OutputSerial( &Vsd );
			
			// 3�b�ڑ��f�Ȃ�ċN��
			if( ++Vsd.uConnectWDT > Vsd.uLogHz * 3 ){
				// ���Z�b�g
			}
		}else{
			putchar( 0xFF );
		}
	}
}
