﻿#ifndef DEF_CHECKBOX_N
	#define DEF_CHECKBOX_N DEF_CHECKBOX
#endif

//			enum name
//				|				チェックボックスの初期値 (値は0か1)
//				|				|	チェックボックスの名前
//				|				|	|						config 名
//				|				|	|						|
DEF_CHECKBOX(	CHECK_LAP,		1,	"ラップタイム表示",		"lap_time"	)
DEF_CHECKBOX(	CHECK_GRAPH,	0,	"グラフ表示",			"graph"		)
DEF_CHECKBOX_N(	CHECK_GPS_PRIO,	0,	"GPSログ優先",			NULL		)
DEF_CHECKBOX_N(	CHECK_LOGPOS,	1,	"半自動同期",			NULL		)

#undef DEF_CHECKBOX
#undef DEF_CHECKBOX_N
