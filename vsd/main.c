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

/*** ���ԡ��ɡ������Τ߽��� *************************************************/

INLINE void OutputSerialS( void ){
	UCHAR c = 0xFF;
	
	SerialPack( g_Tacho.uVal );
	SerialPack( g_Speed.uVal );
	sci_write( &c, 1 );
}

/*** main *******************************************************************/

#ifdef MONITOR_ROM
	#pragma entry( main )
#else
	__entry( vect = 0 )
#endif
int main( void ){
	
	UCHAR	bProcessUIOFlag = 0;
	
	#ifdef MONITOR_ROM
		if( !IO.PDR5.BIT.B4 ) IR_Flasher();
	#else
		InitSector( __sectop( "B" ), __secend( "B" ));
	#endif
	
	InitMain();
	
//	Print( g_szMsgOpening );
	
	for(;;){
		WaitStateChange();
		
		/* 
		//�ǥХå��ѥ�����
		g_Tacho.Time.dw = g_Speed.Time.dw = GetTimerW32();
		g_Tacho.uPulseCnt = (( GetRTC() >> 7 ) & 0x7 ) + 1;
		g_Speed.uPulseCnt = (( GetRTC() >> 6 ) & 0xF ) + 1;
		if( IO.PDR5.BYTE & ( 1 << 6 )){
			g_Tacho.uPulseCnt = 8;
		}
		*/
		
		ComputeMeter();			// speed, tacho, gear �׻�
		DispLED_Carib();		// LED ɽ���ǡ�������
		CheckStartByGSensor();	// G���󥵡��ˤ�륹�����ȸ���
		ProcessAutoMode();		// �����ȥ⡼��
		
		/*** ���ꥢ����Ͻ��� ***/
		// SIO, sw ���� UserIO ����
		if( bProcessUIOFlag = ~bProcessUIOFlag ){
			ProcessUIO();
		//}else{
		//	// ���ԡ��ɡ����� only ����
		//	if( g_Flags.bOutputSerial )	OutputSerialS();
		}
	}
}

#else
__entry( vect = 0 ) int main( void ){
	SoftReset(); // RAM �� ROM �������Ѥ��ʤ���Ф���
}
#endif
