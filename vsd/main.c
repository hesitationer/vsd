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

/*** ���ߥ�������� *******************************************************/

const UINT g_uTachoBar[] = {
	334, 200, 150, 118, 97
};

#undef ComputeGear
#undef ComputeGear2

INLINE void ComputeGear( UINT uTachoBar[] ){
	UINT	uGearRatio;
	UCHAR	cGear, cBestGear;
	int		i;
	
	uGearRatio = ( UINT )((( ULONG )g_Speed.uVal << 8 ) / g_Tacho.uVal );
	
	if     ( uGearRatio < GEAR_TH( 1 ))	cGear = 1;
	else if( uGearRatio < GEAR_TH( 2 )) cGear = 2;
	else if( uGearRatio < GEAR_TH( 3 ))	cGear = 3;
	else if( uGearRatio < GEAR_TH( 4 ))	cGear = 4;
	else								cGear = 5;
	
	/*** LED�С�ɽ�� ********************************************************/
	
	// LED �С�ɽ���׻�
	
	if( g_Flags.uGearMode == GM_TOWN ){
		// �����
		g_cLEDBar = (( ULONG )g_Tacho.uVal << 4 ) / 1000;
	}else if( g_Tacho.uVal >= REV_LIMIT ){
		g_cLEDBar = 0x50 :
	}else{
		i = 3 - ( int )( REV_LIMIT - g_Tacho.uVal ) / ( int )uTachoBar[ cGear - 1 ];
		g_cLEDBar =	i < 0 ? 0x00 : i << 4;
	}
	
	// ���եȥ��å׷ٹ�
	g_cLEDBar >= 0x50 ? SetBeep( 30578 >> 3 ) : SetBeep( BEEP_OFF );
	
	g_Flags.bBlinkMain = 0;
	g_Flags.bBlinkSub  = 0;
	
	if( g_Flags.uGearMode >= GM_GEAR ){	// ���եȷٹ����Ѥ���?
		if( g_cLEDBar >= 0x50 ){
			// ���եȥ��å׷ٹ�(����)
			g_Flags.bBlinkSub  = 1;
			g_Flags.bBlinkMain = ( g_Flags.uGearMode >= GM_BL_MAIN );
			
			if( g_Flags.uGearMode >= GM_DESIRED_GEAR ) ++cGear;
		}else{
			// ��Ŭ�ʥ�����׻�����
			if     ( g_Speed.uVal < SH_DOWN_TH( 1 )) cBestGear = 1;
			else if( g_Speed.uVal < SH_DOWN_TH( 2 )) cBestGear = 2;
			else if( g_Speed.uVal < SH_DOWN_TH( 3 )) cBestGear = 3;
			else if( g_Speed.uVal < SH_DOWN_TH( 4 )) cBestGear = 4;
			else									 cBestGear = 5;
			
			// ���եȥ�����ٹ�
			if( cBestGear != cGear ){
				if( g_Flags.uGearMode >= GM_DESIRED_GEAR ) cGear = cBestGear;
				g_Flags.bBlinkSub = 1;
			}
		}
	}
	
	// LED �С� ��С���
	//if( g_Flags.bReverse ) g_cLEDBar = -g_cLEDBar;
	
	// LED �˥���ɽ��
	g_VRAM.cDisp[ 3 ] = ( 0/*g_Flags.bReverse*/ ? g_cFontR : g_cFont )[ cGear ];
}

INLINE void ComputeGear2( void ){
	if( g_Flags.uDispMode >= DISPMODE_SPEED ){
		ComputeGear( g_uTachoBar );
	}else{
		g_Flags.bBlinkMain	= 0;
		g_Flags.bBlinkSub	= 0;
	}
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
