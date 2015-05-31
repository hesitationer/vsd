#include <stdio.h>
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "dds.h"
#include "usart.h"

#define	DEFAULT_PORT	USART1

/*** �o�b�t�@ ***************************************************************/

#define TXBUF_SIZE	64
#define RXBUF_SIZE	64

UCHAR	g_cTxBuf[ TXBUF_SIZE ];
volatile USHORT	g_uTxBufRp = 0, g_uTxBufWp = 0;
UCHAR	g_cRxBuf[ TXBUF_SIZE ];
volatile USHORT	g_uRxBufRp = 0, g_uRxBufWp = 0;

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
	//USART_ITConfig( DEFAULT_PORT, USART_RXNE, ENABLE );
}

/*** ���荞�݃n���h�� *******************************************************/

void USART1_IRQHandler( void ){
	
	// ��M�o�b�t�@�t��
	
	// ���M�o�b�t�@�G���v�e�B
	if( USART_GetITStatus( DEFAULT_PORT, USART_IT_TXE )){
		UINT uRp = g_uTxBufRp;
		USART_SendData( DEFAULT_PORT, g_cTxBuf[ uRp ]);
		if( ++uRp >= TXBUF_SIZE ) uRp = 0;
		g_uTxBufRp = uRp;
		
		// ���M�f�[�^�������Ȃ����̂Ŋ��荞�݋֎~
		if( uRp == g_uTxBufWp ){
			USART_ITConfig( DEFAULT_PORT, USART_IT_TXE, DISABLE );
		}
	}
}

/*** �o�� *******************************************************************/

void UsartPutchar( UCHAR c ){
	UINT uWp = g_uTxBufWp;
	UINT uNextWp = uWp + 1;
	if( uNextWp >= TXBUF_SIZE ) uNextWp = 0;
	
	// ���M�o�b�t�@ Full �Ȃ̂ő҂�
	while( uNextWp == g_uTxBufRp );
	
	g_cTxBuf[ uWp ] = c;
	g_uTxBufWp = uNextWp;
	
	// tx ���荞�݋���
	USART_ITConfig( DEFAULT_PORT, USART_IT_TXE, ENABLE );
}

void UsartPutstr( char *szMsg ){
	while( *szMsg ){
		UsartPutchar( *szMsg );
		++szMsg;
	}
}
