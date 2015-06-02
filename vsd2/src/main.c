/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	main.c -- main routine
	
*****************************************************************************/

// ���b��
#define _TIM2
#define _TIM3

#include <stdio.h>
#include <ST\iostm32f10xxB.h>
#include "stm32f10x_nvic.h"
#include "stm32f10x_tim.h"
#include "hw_config.h"
//#define DEBUG
#include "dds.h"
#include "main2.h"
#include "usart.h"

/*** macros *****************************************************************/

#define SRAM_TOP	0x20000000
#define SRAM_END	0x20005000

/*** const ******************************************************************/
/*** new type ***************************************************************/
/*** prototype **************************************************************/
/*** extern *****************************************************************/
/*** gloval vars ************************************************************/
/*** S ���R�[�h���[�_ *******************************************************/

UINT GetHex( UINT uBytes ){
	
	uBytes <<= 1;;
	UINT	uRet = 0;
	UINT	c;
	
	do{
		uRet <<= 4;
		c = GetcharWait();
		
		if( '0' <= c && c <= '9' )	uRet |= c - '0';
		else						uRet |= c - ( 'A' - 10 );
	}while( --uBytes );
	
	//DbgMsg(( "%02X ", uRet ));
	return uRet;
}

__noreturn void JumpTo( u32 uJmpAddr, u32 uSP ){
	asm( "MSR MSP, r1\nBX r0\n" );
}

__noreturn void LoadSRecord( void ){
	
	UINT	uAddr, uLen;
	UINT	c;
	
	while( 1 ){
		// 'S' �܂ŃX�L�b�v
		while( GetcharWait() != 'S' );
		
		// �I���w�b�_�Ȃ� break;
		if(( c = GetcharWait()) == '7' ) break;
		
		if( c == '3' ){
			// �f�[�^����������
			uLen	= GetHex( 1 ) - 5;
			uAddr	= GetHex( 4 );
			
			DbgMsg(( "Addr:%X Len:%X\n", uAddr, uLen ));
			while( uLen-- ) *( UCHAR *)( uAddr++ ) = GetHex( 1 );
		}
	}
	
	printf( "starting %X...\n", *( u32 *)0x20000004 );
	JumpTo( *( u32 *)0x20000004, *( u32 *)0x08003000 );
}

/*** �o�C�i�����[�_ *********************************************************/

__noreturn void LoadBin( void ){
	UINT uCnt;
	UINT uSize = GetcharWait() | ( GetcharWait() << 8 );
	
	DbgMsg(( "\nloading %d bytes\n", uSize ));
	
	for( uCnt = 0; uCnt < uSize; ++uCnt ){
		*( UCHAR *)( 0x20000000 + uCnt ) = GetcharWait();
	}
	
	DbgMsg(( "starting %X...\n", *( u32 *)0x20000004 ));
	JumpTo( *( u32 *)0x20000004, *( u32 *)0x08003000 );
}

/****************************************************************************/

__noreturn void main( void ){
	
	#ifndef EXEC_SRAM
		Set_System();
		GPIOC_ODR ^= 0x40;    // LED�̏o�͂𔽓]������B
		UsartInit( 38400, NULL );
		printf( "Waiting for S record...\n" );
		LoadSRecord();
	#endif
	
	// USART buf
	USART_BUF_t	UsartBuf = { 0 };
	
	// �x�N�^�e�[�u���Đݒ�
	NVIC_SetVectorTable( NVIC_VectTab_RAM, 0 );
	
	UsartInit( 38400, &UsartBuf );
	
	TimerInit();
	while( 1 ) printf( "%08X\n", GetCurrentTime());
	
	UINT uPrevTime = 0;
	while( 1 ){
		while( GetCurrentTime() - uPrevTime < 72000000 );
		printf( "%d\n", uPrevTime / 72000000 );
		uPrevTime += 72000000;
	}
}
