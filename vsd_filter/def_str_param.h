//			enum name
//				|							�ϐ���
//				|							|						�����l
//				|							|						|				config ��
//				|							|						|				|
#ifndef GPS_ONLY
DEF_STR_PARAM(	STRPARAM_LOGFILE,			m_szLogFile,			NULL,			"log_file"	)			// VSD ���O
DEF_STR_PARAM(	STRPARAM_LOGFILE_READER,	m_szLogFileReader,		NULL,			"log_file_reader"	)	// VSD ���O
#endif
DEF_STR_PARAM(	STRPARAM_GPSFILE,			m_szGPSLogFile,			NULL,			"gps_file"	)			// GPS ���O
DEF_STR_PARAM(	STRPARAM_GPSFILE_READER,	m_szGPSLogFileReader,	NULL,			"gps_file_reader"	)	// GPS ���O
DEF_STR_PARAM(	STRPARAM_SKINFILE,			m_szSkinFile,			DEFAULT_SKIN,	"skin_file"	)			// �X�L����

#undef DEF_STR_PARAM
