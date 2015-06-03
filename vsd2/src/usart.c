/*****************************************************************************
	
	VSD2 - vehicle data logger system2
	Copyright(C) by DDS
	
	uart.c -- USART1 driver
	
*****************************************************************************/

#include <stdio.h>
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_nvic.h"
#include "dds.h"
#include "usart.h"

#define	DEFAULT_PORT	USART1

//#define NO_INT

/*** �o�b�t�@ ***************************************************************/

#ifndef EXEC_SRAM
#pragma location = ".ButtomOfSram"
USART_BUF_t	*g_pUsartBuf;
#endif

/*** ������ *****************************************************************/
// pBuf == NULL �̏ꍇ�́C���荞�݁E�o�b�t�@�����O�Ȃ��œ��삷��

void UsartInit( UINT uBaudRate, USART_BUF_t *pBuf ){
	g_pUsartBuf = pBuf;
	
	// APB �N���b�N
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE );
	
	// GPIO�ݒ�
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_StructInit( &GPIO_InitStruct );
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOA, &GPIO_InitStruct );
	
	GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init( GPIOA, &GPIO_InitStruct );
	
	// �p�����[�^������
	USART_InitTypeDef	Param;
	USART_StructInit( &Param );
	Param.USART_BaudRate	= uBaudRate;
	
	// USART �n�[�h������
	USART_DeInit( DEFAULT_PORT );
	USART_Init( DEFAULT_PORT, &Param );
	USART_Cmd( DEFAULT_PORT, ENABLE );
	
	// ���荞�ݐݒ�
	if( pBuf ){
		// NVIC �ݒ�
		NVIC_InitTypeDef NVIC_InitStructure;
		
		// �������ɂ���̂͂�������
		NVIC_PriorityGroupConfig( NVIC_PriorityGroup_1 );
		
		/* Enable USART1 Interrupt */
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init( &NVIC_InitStructure );
		
		USART_ITConfig( DEFAULT_PORT, USART_IT_RXNE, ENABLE );
	}
}

/*** ���荞�݃n���h�� *******************************************************/

void USART1_IRQHandler( void ){
	
	// ��M�o�b�t�@�t��
	if( DEFAULT_PORT->SR & ( 1 << 5 )){
		UINT uWp = g_pUsartBuf->uRxBufWp;
		g_pUsartBuf->cRxBuf[ uWp ] = USART_ReceiveData( DEFAULT_PORT );
		
		uWp = ( uWp + 1 ) & ( USART_RXBUF_SIZE - 1 );
		g_pUsartBuf->uRxBufWp = uWp;
		
		// RxBuf �t���Ȃ�C�����ݒ�~
		/*
		uWp = ( uWp + 1 ) & ( USART_RXBUF_SIZE - 1 );
		if( uWp == g_pUsartBuf->uRxBufRp ){
			USART_ITConfig( DEFAULT_PORT, USART_IT_RXNE, DISABLE );
		}*/
	}
	
	// ���M�o�b�t�@�G���v�e�B
	if( DEFAULT_PORT->SR & ( 1 << 7 )){
		UINT uRp = g_pUsartBuf->uTxBufRp;
		
		if( uRp != g_pUsartBuf->uTxBufWp ){
			USART_SendData( DEFAULT_PORT, g_pUsartBuf->cTxBuf[ uRp ]);
			uRp = ( uRp + 1 ) & ( USART_TXBUF_SIZE - 1 );
			g_pUsartBuf->uTxBufRp = uRp;
			
			// ���M�f�[�^�������Ȃ����̂Ŋ��荞�݋֎~
			if( uRp == g_pUsartBuf->uTxBufWp ){
				USART_ITConfig( DEFAULT_PORT, USART_IT_TXE, DISABLE );
			}
		}
	}
}

/*** 1�������o�� ************************************************************/

int putchar( int c ){
	// �o�b�t�@�����O�Ȃ�
	if( !g_pUsartBuf ){
		while( !( DEFAULT_PORT->SR & ( 1 << 7 )));
		USART_SendData( DEFAULT_PORT, c );
		return c;
	}
	
	// �o�b�t�@�����O����
	UINT uWp = g_pUsartBuf->uTxBufWp;
	UINT uNextWp = ( uWp + 1 ) & ( USART_TXBUF_SIZE - 1 );
	
	// ���M�o�b�t�@ Full �Ȃ̂ő҂�
	while( uNextWp == g_pUsartBuf->uTxBufRp );
	
	g_pUsartBuf->cTxBuf[ uWp ] = c;
	g_pUsartBuf->uTxBufWp = uNextWp;
	
	// tx ���荞�݋���
	USART_ITConfig( DEFAULT_PORT, USART_IT_TXE, ENABLE );
	
	return c;
}

int getchar( void ){
	
	// �o�b�t�@�����O�Ȃ�
	if( !g_pUsartBuf ){
		if( DEFAULT_PORT->SR & ( 1 << 5 )){
			return USART_ReceiveData( DEFAULT_PORT );
		}
		return EOF;
	}
	
	// �o�b�t�@�����O����
	UINT uRp = g_pUsartBuf->uRxBufRp;
	if( uRp == g_pUsartBuf->uRxBufWp ) return EOF;
	
	int iRet = g_pUsartBuf->cRxBuf[ uRp ];
	g_pUsartBuf->uRxBufRp = ( uRp + 1 ) & ( USART_RXBUF_SIZE - 1 );
	
	// rx �����݋���
	//USART_ITConfig( DEFAULT_PORT, USART_IT_RXNE, ENABLE );
	return iRet;
}

int GetcharWait( void ){
	int c;
	while(( c = getchar()) == EOF ) /*_WFI*/;
	return c;
}

/*** ������o�� *************************************************************/

void UsartPutstr( char *szMsg ){
	while( *szMsg ) putchar( *szMsg++ );
}
