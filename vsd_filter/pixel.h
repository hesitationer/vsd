/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	pixel.h - PIXEL_YC / PIXEL_YCA structure
	
*****************************************************************************/

#ifndef _PIXEL_H
#define _PIXEL_H

/*** new type ***************************************************************/

#ifdef AVS_PLUGIN
typedef UCHAR	PIXEL_t;

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
typedef short	PIXEL_t;

class PIXEL_YCA {
  public:
	short	y;		//	��f(�P�x    )�f�[�^ (     0 �` 4096 )
	short	cb;		//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
	short	cr;		//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
					//	��f�f�[�^�͔͈͊O�ɏo�Ă��邱�Ƃ�����܂�
					//	�܂��͈͓��Ɏ��߂Ȃ��Ă����܂��܂���
	USHORT	alfa;	// 0-256
	
	PIXEL_YCA(){}
	
	PIXEL_YCA( UINT uColor ){
		Set( uColor );
	}
	
	void Set( UINT uColor ){
		int a = ( uColor >> 24 ) & 0xFF;
		int r = ( uColor >> 16 ) & 0xFF;
		int g = ( uColor >>  8 ) & 0xFF;
		int b = ( uColor       ) & 0xFF;
		a += a >> 7;	// �l�̌ܓ��̂���
		
		double dAlfa = ( 256 - a ) * ( 4096.0 / 256 / 255 );
		
		y		= ( int )(( 0.299 * r + 0.587 * g + 0.114 * b ) * dAlfa );
		cb		= ( int )((-0.169 * r - 0.331 * g + 0.500 * b ) * dAlfa );
		cr		= ( int )(( 0.500 * r - 0.419 * g - 0.081 * b ) * dAlfa );
		alfa	= a;
	}
};
#endif // !AVS_PLUGIN

#endif // _PIXEL_H
