/*****************************************************************************
	
	$Id$
	
	VSD - poor VSD
	Copyright(C) by DDS
	
	main.c -- main routine
	
*****************************************************************************/

#include <machine.h>
#include "dds.h"
#include "3664s.h"
#include "sci.h"
#include "main.h"
#include "led_charcode.h"

#ifdef MONITOR_ROM
 #include "main2.c"
#else
 #include "rom_entry.h"
 //#define MINIMIZE	// �Ǿ� FIRMWARE
#endif

#ifndef MINIMIZE
/*** macros *****************************************************************/
/*** const ******************************************************************/
/*** new type ***************************************************************/
/*** prototype **************************************************************/
/*** extern *****************************************************************/
/*** gloval vars ************************************************************/
/*** init sector ************************************************************/

INLINE void _INITSCT( void ){
	unsigned *uSrc, *uDst, *uDstEnd;
	
	uSrc	= __sectop( "D" );
	uDst	= __sectop( "R" );
	uDstEnd	= __secend( "R" );
	
	do{
		*uDst++ = *uSrc++;
	}while( uDst < uDstEnd );
	
	uDst	= __sectop( "B" );
	uDstEnd = __secend( "B" );
	do{ *uDst++ = 0; }while(( unsigned )uDst < ( unsigned )uDstEnd );
}

/*** main *******************************************************************/

#ifdef MONITOR_ROM
	#pragma entry( main )
#else
	__entry( vect = 0 )
#endif
int main( void ){
	
	UCHAR			cTimerA;
	
	set_imask_ccr( 1 );
#ifdef MONITOR_ROM
	if( !IO.PDR5.BIT.B4 ) IR_Flasher();
#else
	_INITSCT();
#endif
	
	InitMain();
	set_imask_ccr( 0 );			/* CPU permit interrupts */
	
	Print( g_szMsgOpening );
	g_Flags.uAutoMode	= AM_DISP;
	cTimerA = TA.TCA;
	
	for(;;){
		
		/*** ���ơ����Ѳ��Ԥ� ***/
		
		while( cTimerA == TA.TCA ){
			g_DispVal.uGx += G_SENSOR_Z;	// ���� G �θ��м��ѹ�
			g_DispVal.uGy += G_SENSOR_Y;
			++g_DispVal.uCnt;
			if( !( g_DispVal.uCnt & ( 128 - 1 ))) LED_Driver();
		}
		
		++cTimerA;
		
		if( !( cTimerA & ( CALC_DIVCNT - 1 ))){	// 32Hz
			/* 
			//�ǥХå��ѥ�����
			g_Tacho.Time.dw = g_Speed.Time.dw = GetTimerW32();
			g_Tacho.uPulseCnt = (( GetRTC() >> 7 ) & 0x7 ) + 1;
			g_Speed.uPulseCnt = (( GetRTC() >> 6 ) & 0xF ) + 1;
			if( IO.PDR5.BYTE & ( 1 << 6 )){
				g_Tacho.uPulseCnt = 8;
			}
			*/
			
			ComputeMeter();							// speed, tacho �׻�
			ComputeGear2();							// �����׻�
			DispLED_Carib( &g_DispVal );	// LED ɽ���ǡ�������
			
			/*** G���󥵡��ˤ�륹�����ȸ��� ***/
			CheckStartByGSensor( &g_DispVal );
			
			/*** ���ꥢ����Ͻ��� ***/
			if(
				( CALC_DIVCNT == SERIAL_DIVCNT ) ||
				!( cTimerA & ( SERIAL_DIVCNT - 1 ))
			){
				// key ����
				UCHAR c;
				while( sci_read( &c, 1 )) DoInputSerial( c );
				
				OutputSerialSmooth( &g_DispVal );
				
				// sw ����
				ProcessPushSW( &g_TP );
			}
			
			/*** �����ȥ⡼�� ***/
			ProcessAutoMode();
		}
		
		/*** WDT ***/
		
		WDT.TCSRWD.BYTE = ( 1 << 6 );	// TCWE
		WDT.TCWD		= 0;
	}
}

#else
__entry( vect = 0 ) int main( void ){
	SoftReset(); // RAM �� ROM �������Ѥ��ʤ���Ф���
}
#endif
