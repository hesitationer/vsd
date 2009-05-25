#!/usr/bin/perl -w

# G ���󥵡��ͤ� raw ���� G ñ�̤��Ѵ�

$CaribCntMax = 30;
$CaribCnt = 0;
$GCx = 0;
$GCy = 0;

#$ACC_1G_X = 6762.594337;
$ACC_1G_Y = 6591.556755;
$ACC_1G_Z = 6659.691379;

while( <> ){
	
	s/[\x0D\x0A]//g;
	
	@_ = split( /\t/, $_ );
	
	if( $#_ >= 4 ){
		$_[ 1 ] += 0;
		$_[ 2 ] += 0;
		
		splice( @_, 5, 1 );
		
		if( $CaribCnt < $CaribCntMax ){
			# G ���󥵡������֥졼�����
			
			$GCy += $_[ 3 ];
			$GCx += $_[ 4 ];
			
			++$CaribCnt;
			
			$_[ 3 ] = $_[ 4 ] = 0;
			
			if( $CaribCnt == $CaribCntMax ){
				$GCy /= $CaribCntMax;
				$GCx /= $CaribCntMax;
			}
		}else{
			$_[ 3 ] = sprintf( "%.3f", ( $_[ 3 ] - $GCy ) / $ACC_1G_Z );
			$_[ 4 ] = sprintf( "%.3f", ( $_[ 4 ] - $GCx ) / $ACC_1G_Y );
		}
		
		print join( "\t", @_ ) . "\r\n";
	}
}
