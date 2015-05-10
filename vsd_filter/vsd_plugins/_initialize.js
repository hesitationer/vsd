"use strict";

//****************************************************************************

Include( "_user_config.js" );

var SEEK_SET	= 0;
var SEEK_CUR	= 1;
var SEEK_END	= 2;

// MessageBox
var MB_ABORTRETRYIGNORE				= 0x00000002;
var MB_CANCELTRYCONTINUE			= 0x00000006;
var MB_HELP							= 0x00004000;
var MB_OK							= 0x00000000;
var MB_OKCANCEL						= 0x00000001;
var MB_RETRYCANCEL					= 0x00000005;
var MB_YESNO						= 0x00000004;
var MB_YESNOCANCEL					= 0x00000003;
var MB_ICONEXCLAMATION				= 0x00000030;
var MB_ICONWARNING					= 0x00000030;
var MB_ICONINFORMATION				= 0x00000040;
var MB_ICONQUESTION					= 0x00000020;
var MB_ICONSTOP						= 0x00000010;
var MB_ICONERROR					= 0x00000010;
var MB_ICONHAND						= 0x00000010;
var MB_DEFBUTTON1					= 0x00000000;
var MB_DEFBUTTON2					= 0x00000100;
var MB_DEFBUTTON3					= 0x00000200;
var MB_DEFBUTTON4					= 0x00000300;
var MB_APPLMODAL					= 0x00000000;
var MB_SYSTEMMODAL					= 0x00001000;
var MB_TASKMODAL					= 0x00002000;
var MB_DEFAULT_DESKTOP_ONLY			= 0x00020000;
var MB_RIGHT						= 0x00080000;
var MB_RTLREADING					= 0x00100000;
var MB_SETFOREGROUND				= 0x00010000;
var MB_TOPMOST						= 0x00040000;
var MB_SERVICE_NOTIFICATION			= 0x00200000;
var MB_SERVICE_NOTIFICATION_NT3X	= 0x00040000;
var IDABORT							= 0x00000003;
var IDCANCEL						= 0x00000002;
var IDCONTINUE						= 0x0000000B;
var IDIGNORE						= 0x00000005;
var IDNO							= 0x00000007;
var IDOK							= 0x00000001;
var IDRETRY							= 0x00000004;
var IDTRYAGAIN						= 0x0000000A;
var IDYES							= 0x00000006;

if( typeof( Log ) == 'undefined' ){
	Include( "_initialize_skin.js" );
}else{
	Include( "_initialize_logreader.js" );
}
