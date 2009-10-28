#ifndef DEF_TRACKBAR_N
	#define DEF_TRACKBAR_N DEF_TRACKBAR
#endif

//			enum name
//				|				�g���b�N�o�[�̏����l
//				|					|	�g���b�N�o�[�̉����l
//				|					|		|	�g���b�N�o�[�̏���l
//				|					|		|		|	�g���b�N�o�[�̖��O
//				|					|		|		|		|			config ��
//				|					|		|		|		|			|
DEF_TRACKBAR(	TRACK_VSt,			0,		0,		9999,	"vid�擪",	"video_start"	)
DEF_TRACKBAR_N(	TRACK_VSt2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_VEd,			0,		0,		9999,	"vid�Ō�",	"video_end"		)
DEF_TRACKBAR_N(	TRACK_VEd2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_LSt,			0,		0,		9999,	"log�擪",	"log_start"		)
DEF_TRACKBAR_N(	TRACK_LSt2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_LEd,			0,		0,		9999,	"log�Ō�",	"log_end"		)
DEF_TRACKBAR_N(	TRACK_LEd2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_LineTrace,	0,		0,		1000,	"Map�b",	"map_length"	)
DEF_TRACKBAR(	TRACK_MapSize,		400,	0,		1000,	"Map�T�C�Y","map_size"		)
DEF_TRACKBAR(	TRACK_MapAngle,		0,		0,		3600,	"Map��]",	"map_angle"		)
DEF_TRACKBAR(	TRACK_G_Len,		45,		-1,		200,	"G����",	"g_length"		)

#undef DEF_TRACKBAR
#undef DEF_TRACKBAR_N
