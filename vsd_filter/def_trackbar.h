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
DEF_TRACKBAR(	TRACK_VSt,			0,		0,		10000,	"vid�擪",	"video_start"	)
DEF_TRACKBAR_N(	TRACK_VSt2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_VEd,			0,		0,		10000,	"vid�Ō�",	"video_end"		)
DEF_TRACKBAR_N(	TRACK_VEd2,			0,		-200,	200,	"",			NULL			)
#ifndef CIRCUIT_TOMO
DEF_TRACKBAR(	TRACK_LSt,			0,		0,		10000,	"log�擪",	"log_start"		)
DEF_TRACKBAR_N(	TRACK_LSt2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_LEd,			0,		0,		10000,	"log�Ō�",	"log_end"		)
DEF_TRACKBAR_N(	TRACK_LEd2,			0,		-200,	200,	"",			NULL			)
DEF_TRACKBAR(	TRACK_LineTrace,	0,		0,		1000,	"Map�b",	"map_length"	)
DEF_TRACKBAR(	TRACK_MapSize,		400,	0,		1000,	"Map�T�C�Y","map_size"		)
DEF_TRACKBAR(	TRACK_MapAngle,		0,		0,		3600,	"Map��]",	"map_angle"		)
#else
DEF_TRACKBAR(	TRACK_LSt,			0,		0,		10000,	"log�擪",	"log_start"		)
DEF_TRACKBAR_N(	TRACK_LSt2,			0,		0,		1000,	"   (ms)",	NULL			)
DEF_TRACKBAR(	TRACK_LEd,			0,		0,		10000,	"log�Ō�",	"log_end"		)
DEF_TRACKBAR_N(	TRACK_LEd2,			0,		0,		1000,	"   (ms)",	NULL			)
DEF_TRACKBAR(	TRACK_GEAR1,		1315,	1,		9999,	"1��",		"gear1"			)
DEF_TRACKBAR(	TRACK_GEAR2,		2030,	1,		9999,	"2��",		"gear2"			)
DEF_TRACKBAR(	TRACK_GEAR3,		2810,	1,		9999,	"3��",		"gear3"			)
DEF_TRACKBAR(	TRACK_GEAR4,		3569,	1,		9999,	"4��",		"gear4"			)
DEF_TRACKBAR(	TRACK_GEAR5,		4544,	1,		9999,	"5��",		"gear5"			)
DEF_TRACKBAR(	TRACK_GEAR6,		5741,	1,		9999,	"6��",		"gear6"			)
DEF_TRACKBAR(	TRACK_PULSE_TACHO,	2000,	1,		9999,	"�p���XTA",	"tacho_pulse"	)
DEF_TRACKBAR(	TRACK_PULSE_SPEED,	2548,	1,		9999,	"�p���XSP",	"speed_pulse"	)
DEF_TRACKBAR(	TRACK_TACHO,		8,		1,		  20,	"���[�^TA",	"tacho_meter"	)
DEF_TRACKBAR(	TRACK_SPEED,		180,	50,		 400,	"���[�^SP",	"speed_meter"	)
#endif

#undef DEF_TRACKBAR
#undef DEF_TRACKBAR_N
