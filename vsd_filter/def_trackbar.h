//			enum name
//				|				�g���b�N�o�[�̏����l
//				|					|	�g���b�N�o�[�̉����l
//				|					|		|	�g���b�N�o�[�̏���l
//				|					|		|		|	�g���b�N�o�[�̖��O
//				|					|		|		|		|
DEF_TRACKBAR( TRACK_VSt,			0,		0,		10000,	"vid�擪"	)
DEF_TRACKBAR( TRACK_VSt2,			0,		-200,	200,	""			)
DEF_TRACKBAR( TRACK_VEd,			0,		0,		10000,	"vid�Ō�"	)
DEF_TRACKBAR( TRACK_VEd2,			0,		-200,	200,	""			)
#ifndef CIRCUIT_TOMO
DEF_TRACKBAR( TRACK_LSt,			0,		0,		10000,	"log�擪"	)
DEF_TRACKBAR( TRACK_LSt2,			0,		-200,	200,	""			)
DEF_TRACKBAR( TRACK_LEd,			0,		0,		10000,	"log�Ō�"	)
DEF_TRACKBAR( TRACK_LEd2,			0,		-200,	200,	""			)
DEF_TRACKBAR( TRACK_LineTrace,		0,		0,		1000,	"Map�b"	)
DEF_TRACKBAR( TRACK_MapSize,		400,	0,		1000,	"Map�T�C�Y"	)
//DEF_TRACKBAR( TRACK_MapRot,			0,		0,		360,	"Map��]"	)
#else
DEF_TRACKBAR( TRACK_LSt,			0,		0,		10000,	"log�擪"	)
DEF_TRACKBAR( TRACK_LSt2,			0,		0,		1000,	"   (ms)"	)
DEF_TRACKBAR( TRACK_LEd,			0,		0,		10000,	"log�Ō�"	)
DEF_TRACKBAR( TRACK_LEd2,			0,		0,		1000,	"   (ms)"	)
DEF_TRACKBAR( TRACK_GEAR1,			1315,	1,		9999,	"1��"		)
DEF_TRACKBAR( TRACK_GEAR2,			2030,	1,		9999,	"2��"		)
DEF_TRACKBAR( TRACK_GEAR3,			2810,	1,		9999,	"3��"		)
DEF_TRACKBAR( TRACK_GEAR4,			3569,	1,		9999,	"4��"		)
DEF_TRACKBAR( TRACK_GEAR5,			4544,	1,		9999,	"5��"		)
DEF_TRACKBAR( TRACK_GEAR6,			5741,	1,		9999,	"6��"		)
DEF_TRACKBAR( TRACK_PULSE_TACHO,	2000,	1,		9999,	"�p���XTA"	)
DEF_TRACKBAR( TRACK_PULSE_SPEED,	2548,	1,		9999,	"�p���XSP"	)
DEF_TRACKBAR( TRACK_TACHO,			8,		1,		  20,	"���[�^TA"	)
DEF_TRACKBAR( TRACK_SPEED,			180,	50,		 400,	"���[�^SP"	)
#endif

#undef DEF_TRACKBAR
