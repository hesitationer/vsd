//			enum name
//				|				�����l
//				|				|		config ��
//				|				|		|

#ifdef PUBLIC_MODE
DEF_SHADOW(	PARAM_VSt,			0,		"video_start"	)
DEF_SHADOW(	PARAM_VEd,			0,		"video_end"		)
DEF_SHADOW(	PARAM_GSt,			0,		"gps_start"		)
DEF_SHADOW(	PARAM_GEd,			0,		"gps_end"		)
#endif

DEF_SHADOW(	METER_ADJUST,		1000,	"meter_adjust"	)	// ���[�^�[�␳�W��
DEF_SHADOW(	SHADOW_LAP_START,	0,		"lap_start"		)	// ���b�v�J�n
DEF_SHADOW(	SHADOW_LAP_END,		INT_MAX,"lap_end"		)	// ���b�v�I��

#undef DEF_SHADOW
