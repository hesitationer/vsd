//			enum name
//				|					�ϐ���
//				|					|						�����l
//				|					|						|			config ��
//				|					|						|				|
#ifndef GPS_ONLY
DEF_STR_PARAM(	STRPARAM_LOGFILE,	m_szLogFile,			"",				"log_file"	)	// VSD ���O
#endif
DEF_STR_PARAM(	STRPARAM_GPSFILE,	m_szGPSLogFile,			"",				"gps_file"	)	// GPS ���O
DEF_STR_PARAM(	STRPARAM_FONT,		m_logfont.lfFaceName,	DEFAULT_FONT,	"font"		)	// �t�H���g��

#undef DEF_STR_PARAM
