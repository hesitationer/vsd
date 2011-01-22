#ifndef DEF_TRACKBAR_N
	#define DEF_TRACKBAR_N DEF_TRACKBAR
#endif

//			enum name
//				|				�g���b�N�o�[�̏����l
//				|					|	�g���b�N�o�[�̉����l
//				|					|		|	�g���b�N�o�[�̏���l
//				|					|		|		|	�g���b�N�o�[�̖��O
//				|					|		|		|		|				config ��
//				|					|		|		|		|				|
DEF_TRACKBAR_N(	TRACK_LogOffset,	0,		-9999,	9999,	"GPS����",		NULL	)
#ifndef GPS_ONLY
DEF_TRACKBAR(	PARAM_VSt,			0,		0,		9999,	"Vid�擪",		"video_start"	)
DEF_TRACKBAR(	PARAM_VEd,			0,		0,		9999,	"Vid�Ō�",		"video_end"		)
DEF_TRACKBAR(	PARAM_LSt,			0,		0,		9999,	"log�擪",		"log_start"		)
DEF_TRACKBAR(	PARAM_LEd,			0,		0,		9999,	"log�Ō�",		"log_end"		)
DEF_TRACKBAR(	PARAM_GSt,			0,		0,		9999,	"GPS�擪",		"gps_start"		)
DEF_TRACKBAR(	PARAM_GEd,			0,		0,		9999,	"GPS�Ō�",		"gps_end"		)
#endif
DEF_TRACKBAR(	TRACK_SPEED,		180,	50,		400,	"�X�s�[�h",		"speed_meter"	)
DEF_TRACKBAR(	TRACK_LineTrace,	240,	0,		1000,	"Map�b",		"map_length"	)
DEF_TRACKBAR(	TRACK_MapSize,		400,	0,		1000,	"Map�T�C�Y",	"map_size"		)
DEF_TRACKBAR(	TRACK_MapAngle,		0,		0,		3600,	"Map��]",		"map_angle"		)
DEF_TRACKBAR(	TRACK_G_Len,		30,		-1,		300,	"G����",		"g_length"		)
DEF_TRACKBAR(	TRACK_SLineWidth,	200,	0,		1000,	"Start��",		"start_width"	)
#ifdef GPS_ONLY
DEF_TRACKBAR(	TRACK_Aspect,		1000,	500,	2000,	"Aspect��",		"aspect_ratio"	)
#endif

#undef DEF_TRACKBAR
#undef DEF_TRACKBAR_N
