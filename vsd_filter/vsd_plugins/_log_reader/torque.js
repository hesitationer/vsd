// torque log

LogReaderInfo.push( "Torque log (*.csv)", "*.csv;*.csv.gz", "ReadTorqueLog" );

function ReadTorqueLog( Files ){
	
	var ParamDef = {
		// csv 項目名							Vsd 変数名		乗数
		"Speed (OBD)(km/h)"					: [ "Speed",		1 ],
		"Engine RPM(rpm)"					: [ "Tacho",		1 ],
		"Trip Distance(km)"					: [	"Distance",		1000 ],
		"Throttle Position(%)"				: [	"Accel",		1 ],
		"Longitude"							: [ "Longitude",	1 ],
		"Latitude"							: [ "Latitude",		1 ],
		"Acceleration Sensor(Z axis)(g)"	: [ "Gy",			-1 ],
		"Acceleration Sensor(X axis)(g)"	: [ "Gx",			1 ],
	};
	
	var ParamUsed = [];
	
	var	Cnt = 0;
	var GPSValid = true;
	
	for( var i = 0; i < Files.length; ++i ){
		var file = new File();
		if( file.Open( Files[ i ], "zr" )){
			MessageBox( "ファイルが開けません: " + Files[ i ] );
			return 0;
		}
		
		// ヘッダリード
		var Header = file.ReadLine().split( /\s*,\s*/ );
		
		for( var i = 0; i < Header.length; ++i ){
			if( Header[ i ] == 'Device Time' ){
				IdxTime = i;
				Log.Time = [];
			}else if( typeof ParamDef[ Header[ i ]] != "undefined" ){
				ParamDef[ Header[ i ]][ 2 ] = i;
				ParamUsed.push( ParamDef[ Header[ i ]] );
				Log[ ParamDef[ Header[ i ]][ 0 ]] = [];
				
				if( ParamDef[ Header[ i ]][ 0 ] == 'Longitude' ) GPSValid = false;
			}
		}
		
		while( !file.IsEOF()){
			var Param = file.ReadLine().split( "," );
			
			for( var j = 0; j < ParamUsed.length; ++j ){
				Log[ ParamUsed[ j ][ 0 ]][ Cnt ] = Param[ ParamUsed[ j ][ 2 ]] * ParamUsed[ j ][ 1 ];
			}
			
			// GPS が補足できるまでのデータを補正
			if( !GPSValid && !isNaN( Log.Longitude[ Cnt ] )){
				GPSValid = true;
				
				for( var j = 0; j < Cnt; ++j ){
					Log.Longitude[ j ] = Log.Longitude[ Cnt ];
					Log.Latitude[ j ]  = Log.Latitude[ Cnt ];
				}
			}
			
			// 08-9-2012 11:02:36 (JST)
			if( typeof Param[ IdxTime ] == "string" ){
			Param[ IdxTime ].match( /(\d+)-(\d+)-(\d+) (\d+):(\d+):(\d+)(\.\d+)/ );
				Log.Time[ Cnt ]	= Date.UTC(
					RegExp.$3, RegExp.$2 - 1, RegExp.$1,
					RegExp.$4, RegExp.$5, RegExp.$6, RegExp.$7 * 1000
				) - 9 * 3600 * 1000;
				++Cnt;
			}
		}
		file.Close();
	}
	
	return Cnt;
}
