/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	pixel_base.h - PIXEL_YC / PIXEL_YCA structure
	
*****************************************************************************/

/*** new type ***************************************************************/

class PIXEL {
  public:
	#ifdef PIXEL_AVU
		short	y;		//	��f(�P�x    )�f�[�^ (     0 �` 4096 )
		short	cb;		//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
		short	cr;		//	��f(�F��(��))�f�[�^ ( -2048 �` 2048 )
						//	��f�f�[�^�͔͈͊O�ɏo�Ă��邱�Ƃ�����܂�
						//	�܂��͈͓��Ɏ��߂Ȃ��Ă����܂��܂���
		USHORT	alfa;	// 0-256
		
		void Set( int a, int r, int g, int b ){
			double dAlfa = ( 255 - a ) * ( 4096 / 256 / 255.0 );
			
			y	= ( int )( GetY(  r, g, b ) * dAlfa );
			cb	= ( int )( GetCb( r, g, b ) * dAlfa );
			cr	= ( int )( GetCr( r, g, b ) * dAlfa );
			alfa= a + ( a >> 7 );
		}
		
		PIXEL( PIXEL_RABY yc ){
			int iy = yc.y;
			int ib = ( char )( yc.cb ^ 0x80 );
			int ir = ( char )( yc.cr ^ 0x80 );
			int ia = yc.alfa;
			
			y		= ( iy << 4 ) + ( iy >> 4 );
			cb		= ( ib << 4 ) + ( ib >> 4 );
			cr		= ( ir << 4 ) + ( ir >> 4 );
			alfa	= ia + ( ia >> 7 );
		}
	#else
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
		
		void Set( int a, int r, int g, int b ){
			double dAlfa = ( 255 - a ) / 255.0;
			
			y	= ( int )( GetY(  r, g, b ) * dAlfa );
			cb	= ( int )( GetCb( r, g, b ) * dAlfa ) ^ 0x80;
			cr	= ( int )( GetCr( r, g, b ) * dAlfa ) ^ 0x80;
			alfa= a;
		}
	#endif
	
	PIXEL(){}
	PIXEL( UINT uColor ){ Set( uColor ); }
	
	~PIXEL(){}
	
	static double GetY(  int r, int g, int b ){ return  0.299 * r + 0.587 * g + 0.114 * b; }
	static double GetCb( int r, int g, int b ){ return -0.169 * r - 0.331 * g + 0.500 * b; }
	static double GetCr( int r, int g, int b ){ return  0.500 * r - 0.419 * g - 0.081 * b; }
	
	void Set( UINT uColor ){
		int a = ( uColor >> 24 );
		int r = ( uColor >> 16 ) & 0xFF;
		int g = ( uColor >>  8 ) & 0xFF;
		int b = ( uColor       ) & 0xFF;
		
		return Set( a, r, g, b );
	}
	
	// new & �u�����h
	PIXEL( PIXEL yc0, PIXEL yc1, UINT uAlfa, UINT uAlfaMax ){
		UINT uAlfaM = uAlfaMax - uAlfa;
		
		y	= ( yc0.y  * uAlfaM + yc1.y  * uAlfa ) / uAlfaMax;
		cb	= ( yc0.cb * uAlfaM + yc1.cb * uAlfa ) / uAlfaMax;
		cr	= ( yc0.cr * uAlfaM + yc1.cr * uAlfa ) / uAlfaMax;
		alfa= ( yc0.alfa * uAlfaM + yc1.alfa * uAlfa ) / uAlfaMax;
	}
};

#undef PIXEL_AVU
#undef PIXEL
