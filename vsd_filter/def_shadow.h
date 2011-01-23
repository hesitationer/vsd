//			enum name
//				|				�����l
//				|				|		config ��
//				|				|		|

#ifdef GPS_ONLY
DEF_SHADOW(	PARAM_VSt,			0,		"video_start"	)
DEF_SHADOW(	PARAM_VEd,			0,		"video_end"		)
DEF_SHADOW(	PARAM_GSt,			0,		"gps_start"		)
DEF_SHADOW(	PARAM_GEd,			0,		"gps_end"		)
#endif

DEF_SHADOW(	SHADOW_METER_CX,	-1,		"meter_cx"		)	// ���[�^�[���S���W X (-1:default)
DEF_SHADOW(	SHADOW_METER_CY,	-1,		"meter_cy"		)	// ���[�^�[���S���W Y
DEF_SHADOW(	SHADOW_METER_R,		-1,		"meter_r"		)	// ���[�^�[���a

DEF_SHADOW(	SHADOW_G_SCALE,		1500,	"g_scale"		)	// G �X�P�[�� * 1000
DEF_SHADOW(	METER_ADJUST,		1000,	"meter_adjust"	)	// ���[�^�[�␳�W��
DEF_SHADOW(	SHADOW_FONT_SIZE,	-1,		"font_size"		)	// �t�H���g�T�C�Y
DEF_SHADOW(	SHADOW_FONT_ATTR,	0,		"font_attr"		)	// +1: ���� +2:�Α�
DEF_SHADOW(	SHADOW_LAP_START,	0,		"lap_start"		)	// ���b�v�J�n
DEF_SHADOW(	SHADOW_LAP_END,		INT_MAX,"lap_end"		)	// ���b�v�I��


#undef DEF_SHADOW
