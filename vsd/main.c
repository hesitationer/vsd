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
 #define MINIMIZE	// �Ǿ� FIRMWARE
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
	
	#ifdef MONITOR_ROM
		uSrc	= __sectop( "D" );
		uDst	= __sectop( "R" );
		uDstEnd	= __secend( "R" );
		
		do{
			*uDst++ = *uSrc++;
		}while( uDst < uDstEnd );
	#else
		_INITSCT_ROM();
	#endif
	
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
	
	UINT			uTWovf;
	DispVal_t		DispVal;
	TouchPanel_t	TP;
	
	set_imask_ccr( 1 );
	if( !IO.PDR5.BIT.B4 ) IR_Flasher();
	_INITSCT();
	InitMain();
	set_imask_ccr( 0 );			/* CPU permit interrupts */
	
	Print( g_szMsgOpening );
	
	BZero( DispVal );
	BZero( TP );
	
	uTWovf = g_TimerWovf.w.l;
	
	for(;;){
		
		/*** ���ơ����Ѳ��Ԥ� ***/
		
		while( uTWovf == VOLATILE( UINT, g_TimerWovf.w.l )){
			//DispVal.uGx += G_SENSOR_X;
			DispVal.uGx += G_SENSOR_Z;	// ���� G �θ��м��ѹ�
			DispVal.uGy += G_SENSOR_Y;
			++DispVal.uCnt;
		}
		
		++uTWovf;
		
		if( !( uTWovf & ( CALC_DIVCNT - 1 ))){
			// ��30Hz
			
			ComputeMeter( uTWovf );
			
			DispVal.uTacho += g_Tacho.uVal;
			DispVal.uSpeed += g_Speed.uVal;
			
			/*** �����׻� ***/
			ComputeGear2();
			
			/*** LED ɽ�� ***/
			DispLED_Carib( &DispVal, uTWovf );
			
			/*** G���󥵡��ˤ�륹�����ȸ��� ***/
			/*** ̤���ڡ����ޤ������ʤ��ä����� ***/
			CheckStartByGSensor( &DispVal );
			
			/*** ���ꥢ����Ͻ��� ***/
			
			if(
				( CALC_DIVCNT == SERIAL_DIVCNT ) ||
				!( uTWovf & ( SERIAL_DIVCNT - 1 ))
			){
				// key ����
				UCHAR c;
				while( sci_read( &c, 1 )) DoInputSerial( c );
				
				OutputSerialSmooth( &DispVal );
				
				// sw ����
				ProcessPushSW( &TP );
			}
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
