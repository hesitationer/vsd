#!/usr/bin/perl

use Socket;
use Time::HiRes qw(sleep);

$LOG_HZ = 32;

if( $ARGV[ 0 ] =~ /\.gz$/ ){
	open( fpIn, "gunzip -c $ARGV[ 0 ] |" );
}else{
	open( fpIn, "< $ARGV[ 0 ]" );
}

# �إå�����

$_ = <fpIn>;
s/[\x0D\x0A]//g;
@_ = split( /\t/, $_ );

$IdxDate		= 0x7FFFFFFF;
$IdxTacho		= 0x7FFFFFFF;
$IdxSpeed		= 0x7FFFFFFF;
$IdxDistance	= 0x7FFFFFFF;
$IdxGx			= 0x7FFFFFFF;
$IdxGy			= 0x7FFFFFFF;
$IdxThrottle	= 0x7FFFFFFF;
$IdxAuxInfo		= 0x7FFFFFFF;
$IdxLapTime		= 0x7FFFFFFF;
$IdxSectorTime	= 0x7FFFFFFF;

for( $i = 0; $i <= $#_; ++$i ){
	if(     $_[ $i ] eq "Date/Time"		){ $IdxDate			= $i;
	}elsif( $_[ $i ] eq "Tacho"			){ $IdxTacho		= $i;
	}elsif( $_[ $i ] eq "Speed"			){ $IdxSpeed		= $i;
	}elsif( $_[ $i ] eq "Distance"		){ $IdxDistance		= $i;
	}elsif( $_[ $i ] eq "Gx"			){ $IdxGx			= $i;
	}elsif( $_[ $i ] eq "Gy"			){ $IdxGy			= $i;
	}elsif( $_[ $i ] eq "Throttle(raw)"	){ $IdxThrottle		= $i;
	}elsif( $_[ $i ] eq "AuxInfo"		){ $IdxAuxInfo		= $i;
	}elsif( $_[ $i ] eq "LapTime"		){ $IdxLapTime		= $i;
	}elsif( $_[ $i ] eq "SectorTime"	){ $IdxSectorTime	= $i;
	}
}

if( 0 ){
	print( "IdxDate = $IdxDate\n" );
	print( "IdxTacho = $IdxTacho\n" );
	print( "IdxSpeed = $IdxSpeed\n" );
	print( "IdxDistance = $IdxDistance\n" );
	print( "IdxGx = $IdxGx\n" );
	print( "IdxGy = $IdxGy\n" );
	print( "IdxThrottle = $IdxThrottle\n" );
	print( "IdxAuxInfo = $IdxAuxInfo\n" );
	print( "IdxLapTime = $IdxLapTime\n" );
	print( "IdxSectorTime = $IdxSectorTime\n" );
}

### ������

# 1. �����ѥ����åȤκ���
my $SockListen;
socket( $SockListen, PF_INET, SOCK_STREAM, getprotobyname( 'tcp' ))
	or die "Cannot create socket: $!";

setsockopt( $SockListen, SOL_SOCKET, SO_REUSEADDR, 1 );

# 2. �����ѥ����åȾ���κ���
my $pack_addr = sockaddr_in( 12345, INADDR_ANY );

# 3. �����ѥ����åȤȼ����ѥ����åȾ�����ӤĤ���
bind( $SockListen, $pack_addr ) or die "Cannot bind: $!";

# 4. ��³������դ�������򤹤롣
listen( $SockListen, SOMAXCONN ) or die "Cannot listen: $!";

# 5. ��³������դ��Ʊ������롣
my $SockClient; # ���饤����ȤȤ��̿��ѤΥ����å�

# ��³�ޤ�
accept( $SockClient, $SockListen );
print "Connected\n";

# unbuffered
select( $SockClient );
$| = 1;
select( STDOUT );

$Buf = '';

WaitCmd( 'z' ); SendData( ':' );
WaitCmd( 'S7' ); SendData( ':' );	# l
# GetData();

$PULSE_PER_1KM	= 15473.76689;	# ELISE(CE28N)

#$ACC_1G_X	= 6762.594337;
$ACC_1G_Y	= 6667.738702;
$ACC_1G_Z	= 6842.591839;

$iCnt = 0;
$PrevTime = 1;

while( <fpIn> ){
	
	s/[\x0D\x0A]//g;
	@_ = split( /\t/, $_ );
	
	$_ = pack( 'S7',
		$_[ $IdxTacho ],
		int( $_[ $IdxSpeed ] * 100 ),
		int( $_[ $IdxDistance ] / 1000 * $PULSE_PER_1KM ),
		$iCnt++ * 200000 / 256 / $LOG_HZ,	# TSC
		int( -$_[ $IdxGy ] * $ACC_1G_Y + 32000 ),
		int(  $_[ $IdxGx ] * $ACC_1G_Z + 32000 ),
		$_[ $IdxThrottle ] > 0 ? $_[ $IdxThrottle ] : 0x8000
	);
	
	# ��åץ�����
	if( defined( $_[ $IdxLapTime ] ) && $_[ $IdxLapTime ] =~ /(.+):(.+)/){
		# ��åץ����൭Ͽȯ��
		$PrevTime += int(( $1 * 60 + $2 ) * 256 );
		$_ .= pack( 'I', $PrevTime );
	}
	
	# 0xFE, FF ����
	s/\xFE/\xFE\x00/g;
	s/\xFF/\xFE\x01/g;
	
	last if( !defined( send( $SockClient, $_ . "\xFF", 0 )));
	
	sleep( 1 / $LOG_HZ );
	
	#GetData( MSG_DONTWAIT );
}

sub GetData {
	my( $param ) = @_;
	local( $_ );
	my( $tmp );
	
	recv( $SockClient, $_, 1024, defined( $param ) ? $param : 0 );
	
	return $_ if( !$_ );
	$tmp = $_;
	
	s/([\x00-\x1F\[\x7E\-\xFF])/sprintf( '[%02X]', ord( $1 ))/ge;
	print "Recv:$_\n";
	
	$tmp;
}

sub SendData {
	local( $_ ) = @_;
	
	send( $SockClient, $_, 0 );
	#print <$SockClient>;
	
	s/([\x00-\x1F\[\x7E\-\xFF])/sprintf( '[%02X]', ord( $1 ))/ge;
	print "Send:$_\n";
}

sub WaitCmd {
	local( $_ ) = @_;
	
	print( "Waiting $_\n" );
	
	while( $Buf !~ /$_/ ){
		$Buf .= GetData();
	}
	print( "OK\n" );
	
	$Buf = defined( $' ) ? $' : '';
}
