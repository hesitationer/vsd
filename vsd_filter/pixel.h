/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	pixel.h - PIXEL_YC / PIXEL_YCA structure
	
*****************************************************************************/

#ifndef _PIXEL_H
#define _PIXEL_H

#ifdef AVS_PLUGIN
	typedef UCHAR	PIXEL_t;
	
	#define RGB2YCA( r, g, b, a ) { \
		( int )(( 0.299 * ( r ) + 0.587 * ( g ) + 0.114 * ( b )        ) / 16 * ( 256 - ( a )) / 256 ), \
		( int )((-0.169 * ( r ) - 0.331 * ( g ) + 0.500 * ( b ) + 2048 ) / 16 * ( 256 - ( a )) / 256 ), \
		( int )(( 0.299 * ( r ) + 0.587 * ( g ) + 0.114 * ( b )        ) / 16 * ( 256 - ( a )) / 256 ), \
		( int )(( 0.500 * ( r ) - 0.419 * ( g ) - 0.081 * ( b ) + 2048 ) / 16 * ( 256 - ( a )) / 256 ), \
		( a ) \
	}
#else
	typedef short	PIXEL_t;
	
	#define RGB2YCA( r, g, b, a ) { \
		( int )(( 0.299 * ( r ) + 0.587 * ( g ) + 0.114 * ( b )) * ( 256 - ( a )) / 256 ), \
		( int )((-0.169 * ( r ) - 0.331 * ( g ) + 0.500 * ( b )) * ( 256 - ( a )) / 256 ), \
		( int )(( 0.500 * ( r ) - 0.419 * ( g ) - 0.081 * ( b )) * ( 256 - ( a )) / 256 ), \
		( a ) \
	}
#endif
#define RGB2YC( r, g, b )	RGB2YCA(( r ), ( g ), ( b ), 0 )

/*** new type ***************************************************************/

#ifdef AVS_PLUGIN
typedef struct {
	union {
		// yuv �ʎw��
		struct {
			UCHAR	y;
			UCHAR	cb;
			UCHAR	y1;
			UCHAR	cr;
		};
		
		// ycb / ycr �A�N�Z�X
		struct {
			USHORT	ycb;
			USHORT	ycr;
		};
		
		// yuv �S���w��
		UINT	ycbcr;
	};
	USHORT	alfa;
} PIXEL_YCA;

static inline Color2YCA( PIXEL_YCA &yca, int iColor ){
	hogefuga
}

#else // !AVS_PLUGIN
typedef	struct {
	short	y;					//	��f(�P�x    )�f�[�^ (     0 �` 4096 )
	short	cb;					//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
	short	cr;					//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
								//	��f�f�[�^�͔͈͊O�ɏo�Ă��邱�Ƃ�����܂�
								//	�܂��͈͓��Ɏ��߂Ȃ��Ă����܂��܂���
	USHORT	alfa;
} PIXEL_YCA;

static inline void Color2YCA( PIXEL_YCA &yca, int iColor ){
	int r = (( iColor & 0xFF0000 ) >> ( 16 - 12 )) / 255;
	int g = (( iColor & 0x00FF00 ) << ( 12 -  8 )) / 255;
	int b = (( iColor & 0x0000FF ) << ( 12      )) / 255;
	int a = ( iColor >> 24 ) & 0xFF;
	a += a >> 7;	// �l�̌ܓ��̂���
	
	yca.y		= ( int )(( 0.299 * ( r ) + 0.587 * ( g ) + 0.114 * ( b )) * ( 256 - ( a )) / 256 );
	yca.cb		= ( int )((-0.169 * ( r ) - 0.331 * ( g ) + 0.500 * ( b )) * ( 256 - ( a )) / 256 );
	yca.cr		= ( int )(( 0.500 * ( r ) - 0.419 * ( g ) - 0.081 * ( b )) * ( 256 - ( a )) / 256 );
	yca.alfa	= a;
}
#endif // !AVS_PLUGIN

#endif // _PIXEL_H
