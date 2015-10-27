/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.cpp - JavaScript
	
*****************************************************************************/

#include "StdAfx.h"

#include "CScript.h"
#include "ScriptIF.h"

/*** static �����o�i�G�L�t���j***********************************************/

LPCWSTR CScript::m_szErrorMsgID[] = {
	#define DEF_ERROR( id, msg )	L##msg,
	#include "def_error.h"
};

/*** �R���X�g���N�^ *********************************************************/

CScript::CScript( CVsdFilter *pVsd ){
	DebugMsgD( ":CScript::CScript():%X\n", GetCurrentThreadId());
	
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	
	DebugMsgD( ":CScript::CScript():Isolate::GetCurrent() = %X\n", Isolate::GetCurrent());
	DebugMsgD( ":CScript::CScript():m_pContext\n" );
	
	if( pVsd ) m_pVsd = pVsd;
	
	m_szErrorMsg	= NULL;
	m_uError		= ERR_OK;
}

/*** �f�X�g���N�^ ***********************************************************/

CScript::~CScript(){
	DebugMsgD( ":CScript::~CScript():%X\n", GetCurrentThreadId());
	DebugMsgD( ":CScript::~CScript():Isolate::GetCurrent() = %X\n", Isolate::GetCurrent());
	
	Dispose();
	
	{
		Isolate::Scope IsolateScope( Isolate::GetCurrent());
		m_Context.Reset();
		while( !Isolate::GetCurrent()->IdleNotificationDeadline( 1000 ));
	}
	
	delete [] m_szErrorMsg;
}

/*** Exception ���b�Z�[�W�\�� ***********************************************/

#define MSGBUF_SIZE	( 4 * 1024 )

LPWSTR CScript::ReportException( LPWSTR pMsg, TryCatch& try_catch ){
	HandleScope handle_scope( Isolate::GetCurrent());
	String::Value exception( try_catch.Exception());
	Local<Message> message = try_catch.Message();
	
	if( !pMsg ) pMsg = new WCHAR[ MSGBUF_SIZE ];
	UINT	u = 0;
	
	if ( message.IsEmpty()){
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		swprintf( pMsg, MSGBUF_SIZE, L"%s\n", ( wchar_t *)*exception );
		for( ; u < MSGBUF_SIZE && pMsg[ u ]; ++u );
	}else{
		// Print ( filename ):( line number ): ( message ).
		String::Value filename( message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		swprintf( pMsg, MSGBUF_SIZE - u, L"%s:%i: %s\n", ( wchar_t *)*filename, linenum, ( wchar_t *)*exception );
		
		for( ; u < MSGBUF_SIZE && pMsg[ u ]; ++u );
		
		// Print line of source code.
		String::Value sourceline( message->GetSourceLine());
		swprintf( pMsg + u, MSGBUF_SIZE - u, L"%s\n", ( wchar_t *)*sourceline );
		
		// TAB->SP �ϊ��ƁCp �� '\0' ���w���悤�ɂ���
		for( ; u < MSGBUF_SIZE && pMsg[ u ]; ++u ) if( pMsg[ u ] == '\t' ) pMsg[ u ] = ' ';
		
		// Print wavy underline ( GetUnderline is deprecated ).
		int start = message->GetStartColumn();
		for ( int i = 0; i < start && u < MSGBUF_SIZE; i++ ){
			pMsg[ u++ ] = L' ';
		}
		int end = message->GetEndColumn();
		for ( int i = start; i < end; i++ ){
			pMsg[ u++ ] = L'^';
		}
	}
	
	if( u > MSGBUF_SIZE - 2 ) u = MSGBUF_SIZE - 2;
	
	pMsg[ u++ ] = L'\n';
	pMsg[ u   ] = L'\0';
	
	return pMsg;
}

/*** JavaScript �I�u�W�F�N�g�f�B�X�|�[�U ************************************/

void CScript::Dispose( void ){
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	HandleScope handle_scope( Isolate::GetCurrent());
	Context::Scope context_scope( Isolate::GetCurrent()->GetCurrentContext());
	
	// global obj �擾
	Local<Object> hGlobal = Isolate::GetCurrent()->GetCurrentContext()->Global();
	
	// Log �� key �擾
	Local<Array> Keys = hGlobal->GetPropertyNames();
	
	for( UINT u = 0; u < Keys->Length(); ++u ){
		#ifdef DEBUG
			String::Utf8Value strKey( Keys->Get( u ));
			char *pKey = *strKey;
			DebugMsgD( "js var %s = undef\n", pKey );
		#endif
		hGlobal->Set( Keys->Get( u ), Undefined( Isolate::GetCurrent()));
	}
	
	while( !Isolate::GetCurrent()->IdleNotificationDeadline( 1000 ));
}

/*** sprintf ****************************************************************/

#define SPRINTF_BUF_SIZE	1024

LPWSTR CScript::SprintfSub( const FunctionCallbackInfo<Value>& args ){
	
	if( CScript::CheckArgs( args.Length() >= 1 )){
		V8ErrorNumOfArg();
		return NULL;
	}
	
	// arg �p buf
	UINT *puArgBuf	= new UINT[ args.Length() * 2 ];
	UINT uArgBufPtr	= 0;
	int iArgNum	= 1;
	LPWSTR wszBuf	= NULL;
	std::vector<String::Value *> vecStrValue;
	
	// Fmt ������擾
	String::Value str( args[ 0 ] );
	LPCWSTR wszFmt = ( LPCWSTR )*str;
	LPCWSTR p = wszFmt;
	
	// % �������
	for( ;; ++iArgNum, ++p ){
		// % �T�[�`
		if(( p = wcschr( p, L'%' )) == NULL ) break;
		
		// �^�t�B�[���h�T�[�`
		for( ++p; *p && strchr( "+-0123456789.# ", *p ); ++p );
		
		// %% �������玟�����Ă݂�[
		if( *p == L'%' ){
			--iArgNum;
			continue;
		}
		
		// arg ���`�F�b�N
		if( args.Length() <= iArgNum ){
			//  ����������Ȃ��G���[
			V8ErrorNumOfArg();
			goto ErrorExit;
		}
		
		if( *p == L's' ){
			// strint �^
			String::Value *pvalue = new String::Value( args[ iArgNum ]);
			
			vecStrValue.push_back( pvalue );
			*( LPCWSTR *)&puArgBuf[ uArgBufPtr++ ] = ( LPCWSTR )**pvalue;
		}else if( strchr( "cCdiouxX", *p )){
			// int �^
			puArgBuf[ uArgBufPtr++ ] = args[ iArgNum ]->Int32Value();
		}else if( strchr( "eEfgGaA", *p )){
			// double �^
			*( double *)&puArgBuf[ uArgBufPtr ] = args[ iArgNum ]->NumberValue();
			uArgBufPtr += 2;
		}else{
			// ��Ή��̌^
			V8SyntaxError( "unknown printf type field" );
			goto ErrorExit;
		}
	}
	
	// �������܂�`�F�b�N
	if( args.Length() != iArgNum ){
		V8ErrorNumOfArg();
		goto ErrorExit;
	}
	
	// vsprintf �N��
	va_list vargs;
	va_start( vargs, puArgBuf[ -1 ] );
	
	wszBuf = new WCHAR[ SPRINTF_BUF_SIZE ];
	vswprintf( wszBuf, SPRINTF_BUF_SIZE - 1, wszFmt, vargs );
	
	va_end( vargs );
	
	// �㏈��
  ErrorExit:
	delete [] puArgBuf;
	
	for( UINT u = 0; u < vecStrValue.size(); ++u ){
		delete vecStrValue[ u ];
	}
	
	return wszBuf;
}

void CScript::Sprintf( const FunctionCallbackInfo<Value>& args ){
	HandleScope handle_scope( Isolate::GetCurrent());
	
	LPWSTR str = CScript::SprintfSub( args );
	if( !str ){
		args.GetReturnValue().SetUndefined();
	}
	
	args.GetReturnValue().Set( String::NewFromTwoByte( Isolate::GetCurrent(), ( uint16_t *)str ));
	delete [] str;
	
	//return handle_scope.Escape( v8str );
}

/*** Print ******************************************************************/

void CScript::Printf( const FunctionCallbackInfo<Value>& args ){
	LPCWSTR wsz = SprintfSub( args );
	if( wsz ){
		CVsdFilter::Print( wsz );
		delete [] wsz;
	}
}

void CScript::Print( LPCWSTR szMsg ){
	CVsdFilter::Print( szMsg );
}

int CScript::MessageBox(
	LPCWSTR szMsg,
	LPCWSTR szCaption,
	UINT	uType
){
	return MessageBoxW( NULL, szMsg,
		szCaption ? szCaption : L"VSD filter JavaScript message",
		uType
	);
}

void CScript::DebugPrint( const FunctionCallbackInfo<Value>& args ){
	LPCWSTR wsz = SprintfSub( args );
	OutputDebugStringW( wsz );
	OutputDebugStringW( L"\n" );
	delete [] wsz;
}

/*** include ****************************************************************/

void CScript::Include( LPCWSTR wszFileName ){
	HandleScope handle_scope( Isolate::GetCurrent());
	
	UINT uRet = RunFileCore( wszFileName );
	
	if( uRet == ERR_CANT_OPEN_FILE ){
		V8Error( ERR_CANT_OPEN_FILE );
	}
}

/*** JavaScript interface �̃Z�b�g�A�b�v ************************************/

UINT CScript::Initialize( LPCWSTR wszFileName ){
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	
	// ����
	HandleScope handle_scope( Isolate::GetCurrent());
	
	// �O���[�o���I�u�W�F�N�g�̐���
	Local<ObjectTemplate> global = ObjectTemplate::New( Isolate::GetCurrent());
	
	// Image �N���X�o�^
	CVsdImageIF::InitializeClass( global );
	CVsdFontIF::InitializeClass( global );
	CVsdFilterIF::InitializeClass( global, m_pVsd );
	CVsdFilterLogIF::InitializeClass( global, m_pVsd );
	CVsdFileIF::InitializeClass( global );
	CScriptIF::InitializeClass( global );
	COleIF::InitializeClass( global );
	
	global->Set( String::NewFromOneByte( Isolate::GetCurrent(), ( uint8_t *)"__CVsdFilter" ), External::New( Isolate::GetCurrent(), m_pVsd ));
	global->Set( String::NewFromOneByte( Isolate::GetCurrent(), ( uint8_t *)"__CScript" ), External::New( Isolate::GetCurrent(), this ));
	
	// �O���[�o���I�u�W�F�N�g������𐶐�
	m_Context = *(new Persistent<Context>( Isolate::GetCurrent(),
		Context::New( Isolate::GetCurrent(), NULL, global )
	));
	
	if( wszFileName ) return RunFile( wszFileName );
	
	return ERR_OK;
}

/*** �X�N���v�g�t�@�C���̎��s ***********************************************/

UINT CScript::RunFile( LPCWSTR szFileName ){
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	
	HandleScope handle_scope( Isolate::GetCurrent());
	
	// ������X�R�[�v�𐶐�
	Context::Scope context_scope( Isolate::GetCurrent()->GetCurrentContext());
	
	TryCatch try_catch;
	
	UINT uRet = RunFileCore( szFileName );
	
	if( uRet == ERR_SCRIPT || try_catch.HasCaught()){
		// Print errors that happened during compilation.
		m_szErrorMsg = ReportException( m_szErrorMsg, try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	return uRet;
}

UINT CScript::RunFileCore( LPCWSTR szFileName ){
	
	FILE *fp;
	// �X�N���v�g ���[�h
	CPushDir push_dir( m_pVsd->m_szPluginDirA );
	fp = _wfopen( szFileName, L"r" );
	
	if( fp == NULL ) return m_uError = ERR_CANT_OPEN_FILE;
	
	// �t�@�C���T�C�Y�擾
	fseek( fp, 0, SEEK_END );
	fpos_t Size;
	fgetpos( fp, &Size );
	fseek( fp, 0, SEEK_SET );
	
	char *szBuf = new char[ ( UINT )Size + 1 ];
	
	int iReadSize = fread( szBuf, 1, ( size_t )Size, fp );
	fclose( fp );
	szBuf[ iReadSize ] = '\0';
	
	Local<Script> script = Script::Compile(
		String::NewFromOneByte( Isolate::GetCurrent(), ( uint8_t *)szBuf ), String::NewFromTwoByte( Isolate::GetCurrent(), ( uint16_t *)szFileName )
	);
	
	delete [] szBuf;
	
	if( script.IsEmpty()){
		// Print errors that happened during compilation.
		return m_uError = ERR_SCRIPT;
	}
	
	// �Ƃ肠��������������
	Local<Value> result = script->Run();
	
	return m_uError = ERR_OK;
}

/*** function ���w����s�C�����Ȃ� ******************************************/

UINT CScript::Run( LPCWSTR szFunc, BOOL bNoFunc ){
	
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	HandleScope handle_scope( Isolate::GetCurrent());
	Context::Scope context_scope( Isolate::GetCurrent()->GetCurrentContext());
	
	return RunArg( szFunc, 0, NULL, bNoFunc );
}

UINT CScript::Run_s( LPCWSTR szFunc, LPCWSTR str0, BOOL bNoFunc ){
	
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	HandleScope handle_scope( Isolate::GetCurrent());
	Context::Scope context_scope( Isolate::GetCurrent()->GetCurrentContext());
	
	#define SetStringArg( arg, str ) \
		if( str ) Args[ arg ] = String::NewFromTwoByte( Isolate::GetCurrent(), ( uint16_t *)str ); \
		else      Args[ arg ] = Undefined( Isolate::GetCurrent());
	
	Handle<Value> Args[ 1 ];
	SetStringArg( 0, str0 );
	
	return RunArg( szFunc, 1, Args, bNoFunc );
}

UINT CScript::Run_ss( LPCWSTR szFunc, LPCWSTR str0, LPCWSTR str1, BOOL bNoFunc ){
	
	Isolate::Scope IsolateScope( Isolate::GetCurrent());
	HandleScope handle_scope( Isolate::GetCurrent());
	Context::Scope context_scope( Isolate::GetCurrent()->GetCurrentContext());
	
	Handle<Value> Args[ 2 ];
	SetStringArg( 0, str0 );
	SetStringArg( 1, str1 );
	
	return RunArg( szFunc, 2, Args, bNoFunc );
}

UINT CScript::RunArg( LPCWSTR szFunc, int iArgNum, Handle<Value> Args[], BOOL bNoFunc ){
	TryCatch try_catch;
	
	Local<Function> hFunction = Local<Function>::Cast( Isolate::GetCurrent()->GetCurrentContext()->Global()->Get( String::NewFromTwoByte( Isolate::GetCurrent(), ( uint16_t *)szFunc )));
	if( hFunction->IsUndefined()){
		if( bNoFunc ) return ERR_OK;
		if( !m_szErrorMsg ) m_szErrorMsg = new WCHAR[ MSGBUF_SIZE ];
		
		swprintf( m_szErrorMsg, MSGBUF_SIZE, L"Undefined function \"%s()\"\n", szFunc );
		return m_uError = ERR_SCRIPT;
	}
	Local<Value> result = hFunction->Call( Isolate::GetCurrent()->GetCurrentContext()->Global(), iArgNum, Args );
	
	if( try_catch.HasCaught()){
		m_szErrorMsg = ReportException( m_szErrorMsg, try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	// �K�[�x�b�W�R���N�V����?
	//while( !V8::IdleNotification());
	//V8::IdleNotification();
	
	return m_uError = ERR_OK;
}

/*** ���O���[�h by JavaScript **********************************************/

BOOL LogReaderCallback( const char *szPath, const char *szFile, void *pParam ){
	if( !IsExt( szFile, "js" )) return TRUE;
	
	CScript &Script = *( CScript *)pParam;
	
	char szBuf[ MAX_PATH + 1 ];
	strcat( strcpy( szBuf, szPath ), szFile );
	
	LPWSTR pFile = NULL;
	Script.RunFile( StringNew( pFile, szBuf ));
	delete pFile;
	
	if( Script.m_uError ){
		// �G���[
		Script.m_pVsd->DispErrorMessage( Script.GetErrorMessage());
		return FALSE;
	}
	
	return TRUE;
}

UINT CScript::InitLogReader( void ){
	Initialize( L"_system/InitLogReader.js" );
	if( m_uError ){
		// �G���[
		m_pVsd->DispErrorMessage( GetErrorMessage());
		return m_uError;
	}
	
	// �X�N���v�g���[�h
	char szBuf[ MAX_PATH + 1 ];
	strcpy( szBuf, m_pVsd->m_szPluginDirA );
	strcat( szBuf, LOG_READER_DIR "\\" );
	if( !ListTree( szBuf, "*", LogReaderCallback, this )){
		return ERR_CANT_OPEN_FILE;
	}
	
	Run( L"SortLogReaderInfo", TRUE );
	if( m_uError ){
		// �G���[
		m_pVsd->DispErrorMessage( GetErrorMessage());
		return m_uError;
	}
	
	return ERR_OK;
}
