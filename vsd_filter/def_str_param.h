//			enum name
//				|					変数名
//				|					|						初期値
//				|					|						|			config 名
//				|					|						|				|
#ifndef GPS_ONLY
DEF_STR_PARAM(	STRPARAM_LOGFILE,	m_szLogFile,			"",				"log_file"	)	// VSD ログ
#endif
DEF_STR_PARAM(	STRPARAM_GPSFILE,	m_szGPSLogFile,			"",				"gps_file"	)	// GPS ログ

#undef DEF_STR_PARAM
