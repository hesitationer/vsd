#ifndef DEF_CHECKBOX_N
	#define DEF_CHECKBOX_N DEF_CHECKBOX
#endif

//			enum name
//				|				�`�F�b�N�{�b�N�X�̏����l (�l��0��1)
//				|				|	�`�F�b�N�{�b�N�X�̖��O
//				|				|	|						config ��
//				|				|	|						|
DEF_CHECKBOX(	CHECK_LAP,		1,	"���b�v�^�C���\��",		"lap_time"	)
DEF_CHECKBOX_N(	CHECK_GPS_PRIO,	0,	"GPS���O�D��",			NULL		)
DEF_CHECKBOX_N(	CHECK_LOGPOS,	1,	"����������",			NULL		)
DEF_CHECKBOX(	CHECK_GRAPH,	0,	"�O���t�\��",			"graph"		)
DEF_CHECKBOX_N(	CHECK_SYNCINFO,	0,	"�������\��",			NULL		)

#undef DEF_CHECKBOX
#undef DEF_CHECKBOX_N
