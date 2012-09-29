/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	pixel.h - PIXEL_YC / PIXEL_YCA structure
	
*****************************************************************************/

#pragma once 

/*** new type ***************************************************************/

#ifdef AVS_PLUGIN
typedef UCHAR	PIXEL_t;

class PIXEL_YCA {
  public:
	union {
		// yuv �S���w��
		UINT	ycbcr;
		
		// ycb / ycr �A�N�Z�X
		struct {
			USHORT	ycb;
			USHORT	ycr;
		};
		
		// yuv �ʎw��
		struct {
			UCHAR	y;
			UCHAR	cb;		// -128�`127 �ł͂Ȃ��� 0�`255
			UCHAR	alfa;
			UCHAR	cr;		// -128�`127 �ł͂Ȃ��� 0�`255
		};
	};
	
	PIXEL_YCA(){};
	
	PIXEL_YCA( UINT uColor ){
		Set( uColor );
	}
	
	void Set( UINT uColor ){
		ycbcr = uColor;
	}
};

#define PIXEL_YCA_ARG	PIXEL_YCA

#else // !AVS_PLUGIN
typedef short	PIXEL_t;

#define PIXEL_YCA_ARG	PIXEL_YCA&

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
		uColor ^= 0x80008000;
		
		int ir = ( char )( uColor >> 24 );
		int ia = ( uColor >> 16 ) & 0xFF;
		int ib = ( char )( uColor >>  8 );
		int iy = ( uColor       ) & 0xFF;
		
		y		= ( iy << 4 ) + ( iy >> 4 );
		cb		= ( ib << 4 ) + ( ib >> 4 );
		cr		= ( ir << 4 ) + ( ir >> 4 );
		alfa	= ia + ( ia >> 7 );
	}
};
#endif // !AVS_PLUGIN
