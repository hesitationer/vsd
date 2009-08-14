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

INLINE void InitSector(
/*	UINT	*uDStart,
	UINT	*uRStart,
	UINT	*uREnd,		*/
	UINT	*uBStart,
	UINT	*uBEnd
){
	// RAM �¹Ԥˤ� R ���ʤ�
	/*
	while( uRStart < uREnd ){
		*uRStart++ = *uDStart++;
	}
	*/
	
	while( uBStart < uBEnd ){ *uBStart++ = 0; };
}

/*** �桼�� IO ���� *********************************************************/

void ProcessUIO( void ){
	UCHAR c;
	while( sci_read( &c, 1 )) DoInputSerial( c );	// serial ����
	OutputSerialSmooth( &g_DispVal );				// serial ����
	ProcessPushSW( &g_TP );							// sw ����
}

/*** Tacho / Speed �׻� *****************************************************/

// 0rpm ���ڤ겼���� EG ��ž���Υѥ륹�� = 200rpm (clk��@16MHz)
#define TACHO_0RPM_TH	(( ULONG )( H8HZ / ( 200 / 60.0 * 2 )))

// 0km/h ���ڤ겼���� speed �ѥ륹�� = 1km/h (clk��@16MHz)
#define SPEED_0KPH_TH	(( ULONG )( H8HZ / ( PULSE_PER_1KM / 3600.0 )))

INLINE void ComputeMeterTacho( void ){
	ULONG	uPrevTime, uTime;
	UINT	uPulseCnt;
	
	// �ѥ�᡼������
	IENR1.BIT.IEN2 = 0;	// Tacho IRQ disable
	uTime				= g_Tacho.Time.dw;
	uPulseCnt			= g_Tacho.uPulseCnt;
	g_Tacho.uPulseCnt	= 0;
	IENR1.BIT.IEN2 = 1;	// Tacho IRQ enable
	uPrevTime			= g_Tacho.PrevTime.dw;
	
	// Tacho �׻�
	if( uPulseCnt ){
		// 30 = 60[min/sec] / 2[pulse/roll]
		// << 7 �ϡ�ʬ�� >> 1 �� g_uHz �� << 8 ����ʬ
		g_Tacho.uVal = (
			( UINT )(
				( ULONG )g_uHz * (( 30 << 7 ) * uPulseCnt ) /
				(( uTime - uPrevTime ) >> 1 )
			) +
			g_Tacho.uVal
		) >> 1;
		
		g_Tacho.PrevTime.dw = uTime;
	}else if(
		uTime = GetTimerW32(),
		// 0.2�ø� ��0�� (0.2s = 150rpm)
		uTime - uPrevTime >= TACHO_0RPM_TH
	){
		g_Tacho.uVal		= 0;
		g_Tacho.PrevTime.dw	= uTime - TACHO_0RPM_TH;
	}
}

UINT g_uSpeedCalcConst = ( UINT )( 3600.0 * 100.0 / PULSE_PER_1KM * ( 1 << 11 ));

INLINE void ComputeMeterSpeed( void ){
	ULONG	uPrevTime, uTime;
	UINT	uPulseCnt;
	UINT	uPulseCntTmp;
	
	// �ѥ�᡼������
	IENR1.BIT.IEN2 = 0;	// Speed IRQ disable
	uTime				= g_Speed.Time.dw;
	uPulseCnt			= g_Speed.uPulseCnt;
	g_Speed.uPulseCnt	= 0;
	IENR1.BIT.IEN2 = 1;	// Speed IRQ enable
	uPrevTime			= g_Speed.PrevTime.dw;
	
	uPulseCntTmp = uPulseCnt;
	
	// Speed �׻�
	if( uPulseCnt || g_Speed.uVal ){
		if( uPulseCnt ){
			g_uMileage += uPulseCnt;
			g_Speed.PrevTime.dw = uTime;
		}else{
			// �ѥ륹�����äƤʤ��ä��顤�ѥ륹��1��������ä���ΤȤ���®�ٷ׻�
			uPulseCnt		= 1;
			uTime			= GetTimerW32();
		}
		
		// �֥����׻��Ȥ�.xls�׻���
		// 5 = 13(��������) - 8(g_uHz �Υ��ե�ʬ)
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
		// �ѥ륹�����ä��Ȥ���ɬ�� 1km/h �ʾ�
		if( g_Speed.uVal < 100 ) g_Speed.uVal = 100;
	}else{
		// 1km/h ̤���� 0km/h ����
		if( g_Speed.uVal < 100 ){
			g_Speed.uVal = 0;
			g_Speed.PrevTime.dw = uTime - SPEED_0KPH_TH;
		}
	}
	
	// 0-100�������Ԥ��⡼�ɤ�100km/h��ã������NewLap��ư
	if( g_Flags.uLapMode == MODE_ZERO_ONE_WAIT && g_Speed.uVal >= 10000 ){
		g_IR.Time.dw = GetRTC();
		g_Flags.bNewLap = TRUE;
		g_Flags.uLapMode = MODE_LAPTIME;
	}
}

#undef ComputeMeter
INLINE void ComputeMeter( void ){
	
	if( g_uHz < (( H8HZ - 500000 ) >> 8 )){
		g_uHz = H8HZ >> 8;
	}
	
	ComputeMeterTacho();
	ComputeMeterSpeed();
	
	if( g_Flags.uDispMode >= DISPMODE_SPEED ){
		ComputeGear( g_uTachoBar );
	}else{
		g_Flags.bBlinkMain	= 0;
		g_Flags.bBlinkSub	= 0;
	}
}

/*** �����ȥ⡼�ɽ��� *******************************************************/

#undef ProcessAutoMode
INLINE UINT ProcessAutoMode( void ){
	if( g_Tacho.uVal >= 4500 ){
		// 4500rpm �ʾ�ǡ�Circuit �⡼�ɤ˰ܹ�
		g_Flags.uGearMode	= GM_GEAR;
		g_cAutoModeTimer	= g_uRTC;
	}else if(( UCHAR )g_uRTC - g_cAutoModeTimer >= 120 ){
		// 2ʬ�֡�4500rpm �ʲ��ʤ顤�����⡼�ɤ˰ܹ�
		g_Flags.uGearMode	= GM_TOWN;
	}
	
	if( g_Flags.uAutoMode == AM_DISP ){
		// AM_DISP �ΤȤ���Speed��Tacho �μ�ư�ڴ���
		if( g_Tacho.uVal < 1500 ){
			g_Flags.uDispModeNext = DISPMODE_TACHO;
		}else if( g_Speed.uVal >= 70 * 100 ){
			g_Flags.uDispModeNext = DISPMODE_SPEED;
		}
	}
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
		InitSector( __sectop( "B" ), __secend( "B" ));
	#endif
	
	g_szLEDMsg = g_LEDAnimeOpening;	// �� ROM���κݤ˺��
	InitMain();
	set_imask_ccr( 0 );			/* CPU permit interrupts */
	
//	Print( g_szMsgOpening );
	g_Flags.uAutoMode	= AM_DISP;	// �� AM_DISP = 0 �ˤ��ƺ��
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
			
			ComputeMeter();						// speed, tacho, gear �׻�
			DispLED_Carib( &g_DispVal );		// LED ɽ���ǡ�������
			CheckStartByGSensor( &g_DispVal );	// G���󥵡��ˤ�륹�����ȸ���
			ProcessAutoMode();					// �����ȥ⡼��
			
			/*** ���ꥢ����Ͻ��� ***/
			if( !( cTimerA & ( SERIAL_DIVCNT - 1 ))){
				// SIO, sw ���� UserIO ����
				ProcessUIO();
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
