#!/usr/bin/perl

while( <> ){
	s/[\x0D\x0A]//g;
	push( @Data, [ split( /,/, $_ ) ] );
}

for( $Param = 0; $Param <= $#{ $Data[ 0 ] }; ++$Param ){
	next if( $Data[ 0 ][ $Param ] =~ /Time/ );
	
	$LastValid = -1;	# �Ǹ��ͭ�����ä�����ǥå���
	for( $Idx = 1; $Idx <= $#Data; ++$Idx ){
		
		if( $Data[ $Idx ][ $Param ] =~ /\d/ ){
			# ͭ���ʿ��ͤ����äƤ���
			
			# ľ����Ʊ���ͤϡ�̵�������ˤ���
			if( $Idx >= 2 && $Data[ $Idx ][ $Param ] == $Data[ $Idx - 1 ][ $Param ] ){
				next;
			}
			
			# �ǽ��ͭ�����ͤʤΤǡ�����ޤǤ�̵���ͤˤ���򥳥ԡ�����
			if( $LastValid < 0 ){
				for( $i = 1; $i < $Idx; ++$i ){
					$Data[ $i ][ $Param ] = $Data[ $Idx ][ $Param ];
				}
				$LastValid = $Idx;
				next;
			}
			
			# ͭ�����ʹ֤�����ʬ�䤹��
			if( $Idx - $LastValid > 4 ){
				$LastValid = $Idx - 4;
			}
			
			$Cnt = $Idx - $LastValid;
			for( $i = 1; $i < $Cnt; ++$i ){
				$Data[ $LastValid + $i ][ $Param ] =
					$Data[ $LastValid ][ $Param ] + (
						$Data[ $Idx ][ $Param ] - 
						$Data[ $LastValid ][ $Param ]
					) * $i / $Cnt;
			}
			$LastValid = $Idx;
		}
	}
}

foreach $_ ( @Data ){
	print join( ',', @{ $_ } ) . "\n";
}
