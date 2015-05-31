#include <stdio.h>
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "dds.h"
#include "usart.h"

#define	DEFAULT_PORT	USART1

/*** ������ *****************************************************************/

void UsartInit( UINT uBaudRate ){
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
	Param.USART_Clock		= USART_Clock_Enable;
	
	// USART �n�[�h������
	USART_DeInit( DEFAULT_PORT );
	USART_Init( DEFAULT_PORT, &Param );
	USART_Cmd( DEFAULT_PORT, ENABLE );
	
	// ���荞�ݐݒ�
	//USART_ITConfig( DEFAULT_PORT
}

/*** �o�� *******************************************************************/

void UsartPutchar( UCHAR c ){
	if( c == '\n' ) UsartPutchar( '\r' );
	USART_SendData( DEFAULT_PORT, c );
}

void UsartPutstr( char *szMsg ){
	while( *szMsg ){
		while( USART_GetFlagStatus( DEFAULT_PORT, USART_FLAG_TXE ) == RESET );
		UsartPutchar( *szMsg );
		++szMsg;
	}
}
