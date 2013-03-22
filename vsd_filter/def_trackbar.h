#ifndef DEF_TRACKBAR_N
	#define DEF_TRACKBAR_N DEF_TRACKBAR
#endif

//			enum name
//				|				�g���b�N�o�[�̏����l
//				|					|	�g���b�N�o�[�̉����l
//				|					|		|	�g���b�N�o�[�̏���l
//				|					|		|		|	�g���b�N�o�[�̖��O
//				|					|		|		|		|						config ��
//				|					|		|		|		|						|
DEF_TRACKBAR_N(	TRACK_LogOffset,	0,		-9999,	9999,	"���O�ʒu����",		NULL	)
#ifndef PUBLIC_MODE
DEF_TRACKBAR(	PARAM_VSt,			0,		0,		9999,	"Video�擪",		"video_start"	)
DEF_TRACKBAR(	PARAM_VEd,			0,		0,		9999,	"Video�Ō�",		"video_end"		)
DEF_TRACKBAR(	PARAM_LSt,			0,		0,		9999,	"���O�擪",			"log_start"		)
DEF_TRACKBAR(	PARAM_LEd,			0,		0,		9999,	"���O�Ō�",			"log_end"		)
DEF_TRACKBAR(	PARAM_GSt,			0,		0,		9999,	"GPS�擪",			"gps_start"		)
DEF_TRACKBAR(	PARAM_GEd,			0,		0,		9999,	"GPS�Ō�",			"gps_end"		)
#endif
DEF_TRACKBAR(	TRACK_LineTrace,	240,	0,		1000,	"���s�O�Ւ���",		"map_length"	)
DEF_TRACKBAR(	TRACK_MapAngle,		0,		0,		3600,	"���s�O�Չ�]",		"map_angle"		)
DEF_TRACKBAR(	TRACK_SLineWidth,	200,	-1,		1000,	"�v���n�_��",		"start_width"	)

#undef DEF_TRACKBAR
#undef DEF_TRACKBAR_N
