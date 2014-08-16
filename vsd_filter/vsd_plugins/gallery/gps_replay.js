//*** 初期化処理 ************************************************************

function Initialize(){
	Scale = 1;
	
	//////////////////////////////////////////////////////////////////////////
	/// ↓↓↓↓↓Google Maps の設定 ここから↓↓↓↓↓ //////////////////////
	//////////////////////////////////////////////////////////////////////////
	
	// 設定を行うためには，以下の設定値を直接書き換えてください．
	
	// ★補足説明
	// このスキンは Google Maps にアクセスし地図データを得ています．
	//   Google Maps にアクセスするためには「API キー」が必要です．
	//   キーは無料で取得出来ます．キー取得方法は
	//   https://developers.google.com/maps/documentation/staticmaps/?hl=ja#api_key
	//   を参照してください．
	// また，Google によって
	//   マップデータの取得は 1日あたり 25,000 枚
	//   ジオコーディングの取得は 1日あたり 2,500 回
	//   に制限されています．
	
	MapParam = {
		// Google Maps の API キーを指定します．
		// 例: APIKey: "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		APIKey: "AIzaSyABCDEFGHIJKLMNOPQRSTUVWXYZabcdefg",
		
		// ズームレベルを 0～21 で指定します
		Zoom: 14,
		
		// 地図タイプ
		// roadmap:地図  satellite:航空写真  terrain:地形図  hybrid:地図+航空写真
		Maptype: "roadmap",
		//Maptype: "hybrid",
		//Maptype: "satellite",
		
		// 地図表示位置，サイズ(最大 640x640)
		X:		0,
		Y:		0,
		Width:	Vsd.Width,
		Height:	Vsd.Height,
		
		// 自車インジケータ
		IndicatorSize:	12 * Scale,		// サイズ
		IndicatorColor:	0x0080FF,		// 色
		
		// 地図更新間隔
		// 前回地図更新時から指定秒以上経過し，
		// かつ指定距離以上移動した場合のみ地図を更新します
		UpdateTime:		1000,	// [ミリ秒]
		UpdateDistance:	160,	// [ピクセル]
		
		// 1に設定すると，地図をスムースにスクロールします．
		// ★重要★
		//   地図から Google の権利帰属表示表示が消え，Google Maps の利用規約
		//   違反になりますので，SmoothScrollMap:1 で作成した動画は絶対にネッ
		//   ト等で公開しないでください．
		SmoothScrollMap:	0,
	};
	
	// Geocoding の設定
	GeocodingParam = {
		// Geocoding 更新間隔
		UpdateTime:	10000,	// [ミリ秒]
	};
	
	//////////////////////////////////////////////////////////////////////////
	/// ↑↑↑↑↑Google Maps の設定 ここまで↑↑↑↑↑ //////////////////////
	//////////////////////////////////////////////////////////////////////////
	
	function min( a, b ){ return ( a < b ) ? a : b; }
	
	if( MapParam.APIKey == '' ){
		MessageBox(
			"google_maps.js スキンを使用するためには初期設定が必要です．詳しくは\n" +
			Vsd.SkinDir + "google_maps.js\n" +
			"をメモ帳等で開き，その先頭に書かれている説明をお読みください．\n" +
			"(設定なしでも短時間なら使えるようです)"
		);
	}
	
	MeterRight = 1;
	
	// 使用する画像・フォントの宣言
	FontM = new Font( "Impact", 20, FONT_FIXED | FONT_OUTLINE );
	FontS = new Font( "Impact", 18 * Scale );
	FontL = new Font( "Impact", 48 * Scale );
	
	// 座標等を予め計算しておく
	MeterR  = 100 * Scale;
	MeterX	= MeterRight ? Vsd.Width  - MeterR * 2: 0;
	MeterY	= Vsd.Height - MeterR * 2 * 0.88;
	MeterCx = MeterX + MeterR;
	MeterCy = MeterY + MeterR;
	
	// スピードメータ用最高速計算
	MaxSpeed = ~~( Vsd.MaxSpeed / 10 ) * 10;
	
	FontColor   = 0;
	FontColorOL = 0xFFFFFF;
	BGColor = 0x80001020;
}

//*** メーター描画処理 ******************************************************

function Draw(){
	// Google マップ表示
	//Vsd.DrawGoogleMaps( MapParam );
	Vsd.DrawRoadMap( MapParam );
	
	// メーター画像描画
	Vsd.DrawCircle( MeterCx, MeterCy, MeterR, BGColor, DRAW_FILL );
	
	// スピードメーター目盛り描画
	Vsd.DrawMeterScale(
		MeterCx, MeterCy, MeterR,
		MeterR * 0.1,  2, 0xFFFFFF,
		MeterR * 0.05, 1, 0xFFFFFF,
		2, 135, 45,
		MeterR * 0.78,
		MaxSpeed, 12, 0xFFFFFF,
		FontS
	);
	
	// スピード数値表示
	Vsd.DrawTextAlign(
		MeterCx, MeterCy + MeterR * 0.25, 
		ALIGN_HCENTER | ALIGN_VCENTER,
		~~Vsd.Speed, FontL, 0xFFFFFF
	);
	
	Vsd.DrawTextAlign(
		MeterCx, MeterCy + MeterR * 0.5,
		ALIGN_HCENTER | ALIGN_VCENTER,
		"km/h", FontS, 0xFFFFFF
	);
	
	// スピードメーター針
	Vsd.DrawNeedle(
		MeterCx, MeterCy, MeterR * 0.95, MeterR * -0.1,
		135, 45, Vsd.Speed / MaxSpeed, 0xFF0000, 3
	);
	
	// 文字データ
	var Y = 0;
	var X = 0;
	
	var date = new Date();
	date.setTime( Vsd.DateTime );
	
	Vsd.DrawText( X, Y,
			date.getFullYear() + "/" +
			( date.getMonth() < 9 ? "0" : "" ) + ( date.getMonth() + 1 ) + "/" +
			( date.getDate() < 10 ? "0" : "" ) + date.getDate() + " " +
			( date.getHours() < 10 ? "0" : "" ) + date.getHours() + ":" +
			( date.getMinutes() < 10 ? "0" : "" ) + date.getMinutes() + ":" +
			( date.getSeconds() < 10 ? "0" : "" ) + date.getSeconds(),
		FontM, FontColor, FontColorOL
	);
	Y += FontM.Height;
	
	Vsd.DrawText( X, Y, "Alt.: " + ( Vsd.Altitude !== undefined ? Vsd.Altitude.toFixed( 1 ) + "m" : "---" ), FontM, FontColor, FontColorOL );
	Y += FontM.Height;
	Vsd.DrawText( X, Y, "Dist.:" + ( Vsd.Distance / 1000 ).toFixed( 2 ) + "km", FontM, FontColor, FontColorOL );
}