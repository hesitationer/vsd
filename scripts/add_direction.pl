#!/bin/perl -w

$ToRAD		= 3.14159265358979323 / 180;
$LAT_M_DEG	= 110949.769;	# 35-36N   �ε�Υ @ 135E
$LNG_M_DEG	= 111441.812;	# 135-136E �ε�Υ @ 35N / cos(35��)


$PrevLong = 0;
$PrevLati = 0;

while( <> ){
	
	if( /GPRMC/ ){
		@_ = split( /,/, $_ );
		
		$Long = int( $_[ 5 ] / 100 );
		$Long += ( $_[ 5 ] - $Long * 100) / 60;
		$Lati = int( $_[ 3 ] / 100 );
		$Lati += ( $_[ 3 ] - $Lati * 100 ) / 60;
		
		# long, lat --> �᡼�ȥ� �Ѵ�
		$x = ( $Long - $PrevLong ) * $LNG_M_DEG * cos( $Lati* $ToRAD );
		$y = ( $Lati - $PrevLati ) * $LAT_M_DEG;
		
		( $PrevLati, $PrevLong ) = ( $Lati, $Long );
		
		# ����
		$_ = atan2( $x, $y );
		$_ += 360 if( $_ < 0 );
		$_[ 8 ] = sprintf( '%.2f', $_ );
		
		$_[ 12 ] =~ s/\*.*//s;
		$_ = join( ',', @_ );
		
		# �����å�����
		$Sum = 0;
		for( $i = 1; $i < length( $_ ); ++$i ){
			$Sum ^= ord( substr( $_, $i, 1 ));
		}
		
		printf( "$_*%02X\n", $Sum );
	}else{
		print;
	}
}
