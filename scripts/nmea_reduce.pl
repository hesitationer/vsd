#!/usr/bin/perl -w

$Mod = 10;	# 0.1��ñ��

while( <> ){
	print if( /([\d+]+.\d)/ && (( $1 * 10 ) % $Mod ) == 0 );
}