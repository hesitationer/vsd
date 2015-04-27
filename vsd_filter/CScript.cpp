/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.cpp - JavaScript
	
*****************************************************************************/

#include "StdAfx.h"

#include "CScript.h"
#include "ScriptIF.h"

using namespace v8;

/*** static メンバ（；´д⊂）***********************************************/

LPCWSTR CScript::m_szErrorMsgID[] = {
	#define DEF_ERROR( id, msg )	L##msg,
	#include "def_error.h"
};

/*** コンストラクタ *********************************************************/

CScript::CScript( CVsdFilter *pVsd ){
	DebugMsgD( ":CScript::CScript():%X\n", GetCurrentThreadId());
	
	m_pIsolate = v8::Isolate::New();
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	
	DebugMsgD( ":CScript::CScript():m_pIsolate = %X\n", m_pIsolate );
	
	DebugMsgD( ":CScript::CScript():m_Context\n" );
	m_Context.Clear();
	
	if( pVsd ) m_pVsd = pVsd;
	//m_pSemaphore = new CSemaphore;
	
	m_szErrorMsg	= NULL;
	m_uError		= ERR_OK;
}

/*** デストラクタ ***********************************************************/

CScript::~CScript(){
	DebugMsgD( ":CScript::~CScript():%X\n", GetCurrentThreadId());
	DebugMsgD( ":CScript::~CScript():m_pIsolate = %X\n", m_pIsolate );
	
	Dispose();
	
	{
		v8::Isolate::Scope IsolateScope( m_pIsolate );
		m_Context.Dispose();
		while( !v8::V8::IdleNotification());
	}
	m_pIsolate->Dispose();
	
	//delete m_pSemaphore;
	
	delete [] m_szErrorMsg;
}

/*** Exception メッセージ表示 ***********************************************/

#define MSGBUF_SIZE	( 4 * 1024 )

LPWSTR CScript::ReportException( LPWSTR pMsg, TryCatch& try_catch ){
	HandleScope handle_scope;
	String::Value exception( try_catch.Exception());
	Handle<Message> message = try_catch.Message();
	
	if( !pMsg ) pMsg = new WCHAR[ MSGBUF_SIZE ];
	UINT	u = 0;
	
	if ( message.IsEmpty()){
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		swprintf( pMsg, MSGBUF_SIZE, L"%s\n", *exception );
		for( ; u < MSGBUF_SIZE && pMsg[ u ]; ++u );
	}else{
		// Print ( filename ):( line number ): ( message ).
		String::Value filename( message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		swprintf( pMsg, MSGBUF_SIZE - u, L"%s:%i: %s\n", *filename, linenum, *exception );
		
		for( ; u < MSGBUF_SIZE && pMsg[ u ]; ++u );
		
		// Print line of source code.
		String::Value sourceline( message->GetSourceLine());
		swprintf( pMsg + u, MSGBUF_SIZE - u, L"%s\n", *sourceline );
		
		// TAB->SP 変換と，p は '\0' を指すようにする
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

/*** JavaScript オブジェクトディスポーザ ************************************/

void CScript::Dispose( void ){
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	v8::HandleScope handle_scope;
	v8::Context::Scope context_scope( m_Context );
	
	// global obj 取得
	v8::Local<v8::Object> hGlobal = m_Context->Global();
	
	// Log の key 取得
	v8::Local<v8::Array> Keys = hGlobal->GetPropertyNames();
	
	for( UINT u = 0; u < Keys->Length(); ++u ){
		#ifdef DEBUG
			v8::String::AsciiValue strKey( Keys->Get( u ));
			char *pKey = *strKey;
			DebugMsgD( "js var %s = undef\n", pKey );
		#endif
		hGlobal->Set( Keys->Get( u ), v8::Undefined());
	}
	
	while( !v8::V8::IdleNotification());
}

/*** Print ******************************************************************/

void CScript::Print( LPCWSTR strMsg ){
	CVsdFilter::Print( strMsg );
}

/*** include ****************************************************************/

static v8::Handle<v8::Value> Func_Include( const v8::Arguments& args ){
	
	int iLen = args.Length();
	if( CScript::CheckArgs( iLen == 2 )) return v8::Undefined();
	
	CScript *obj = static_cast<CScript *>( v8::Local<v8::External>::Cast( args[ 1 ] )->Value());
	if( !obj ) return v8::Undefined();
	
	v8::String::Value str( args[ 0 ] );
	UINT uRet = obj->RunFileCore(( LPCWSTR )*str );
	
	if( uRet == ERR_FILE_NOT_FOUND ){
		return v8::ThrowException( v8::Exception::Error(
			v8::String::Concat(
				v8::String::New( "Include file not found:\n  " ),
				v8::Local<v8::String>::Cast( args[ 0 ] )
			)
		));
	}
	return v8::Undefined();
}

/*** JavaScript interface のセットアップ ************************************/

void CScript::Initialize( void ){
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	
	// 準備
	HandleScope handle_scope;
	
	// グローバルオブジェクトの生成
	Handle<ObjectTemplate> global = ObjectTemplate::New();
	
	// Image クラス登録
	CVsdImageIF::InitializeClass( global );
	CVsdFontIF::InitializeClass( global );
	CVsdFilterIF::InitializeClass( global, m_pVsd );
	CVsdFilter_LogIF::InitializeClass( global, m_pVsd );
	CVsdFileIF::InitializeClass( global );
	CScriptIF::InitializeClass( global );
	COleIF::InitializeClass( global );
	
	global->Set( v8::String::New( "__CVsdFilter" ), v8::External::New( m_pVsd ));
	global->Set( v8::String::New( "__CScript" ), v8::External::New( this ));
	global->Set( v8::String::New( "__Include" ), v8::FunctionTemplate::New( Func_Include ));
	
	// グローバルオブジェクトから環境を生成
	m_Context = Context::New( NULL, global );
}

/*** スクリプトファイルの実行 ***********************************************/

UINT CScript::RunFile( LPCWSTR szFileName ){
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	
	HandleScope handle_scope;
	
	// 環境からスコープを生成
	Context::Scope context_scope( m_Context );
	
	TryCatch try_catch;
	
	UINT uRet = RunFileCore( szFileName );
	
	if( uRet == ERR_SCRIPT || try_catch.HasCaught()){
		// Print errors that happened during compilation.
		m_szErrorMsg = ReportException( m_szErrorMsg, try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	return uRet;
}

#define INCLUDE_CMD	"//#include"
UINT CScript::RunFileCore( LPCWSTR szFileName ){
	
	FILE *fp;
	// スクリプト ロード
	CPushDir push_dir( m_pVsd->m_szPluginDirA );
	fp = _wfopen( szFileName, L"r" );
	
	if( fp == NULL ) return m_uError = ERR_FILE_NOT_FOUND;
	
	// ファイルサイズ取得
	fseek( fp, 0, SEEK_END );
	fpos_t Size;
	fgetpos( fp, &Size );
	fseek( fp, 0, SEEK_SET );
	
	char *szBuf = new char[ ( UINT )Size + 1 ];
	
	int iReadSize = fread( szBuf, 1, ( size_t )Size, fp );
	fclose( fp );
	szBuf[ iReadSize ] = '\0';
	
	// '//#include' 検索
	for( char *p = szBuf; p < szBuf + iReadSize; ){
		
		// skip space
		while( *p == ' ' || *p == '\t' ) ++p;
		
		if( strncmp( p, INCLUDE_CMD, sizeof( INCLUDE_CMD ) - 1 ) == 0 ){
			p += sizeof( INCLUDE_CMD ) - 1;
			// skip space
			while( *p == ' ' || *p == '\t' ) ++p;
			
			if( *p == '"' ){
				++p;
				
				// 終端 " を探す
				char *q;
				for( q = p; *q != '"' && *q != '\0' && *q != 0xA; ++q );
				
				if( *q == '"' ){
					WCHAR wszIncFile[ MAX_PATH + 1 ];
					
					UINT uSize = MultiByteToWideChar(
						CP_UTF8,		// コードページ
						0,				// 文字の種類を指定するフラグ
						p,				// マップ元文字列のアドレス
						q - p,			// マップ元文字列のバイト数
						wszIncFile,		// マップ先ワイド文字列を入れるバッファのアドレス
						MAX_PATH * sizeof( WCHAR )	// バッファのサイズ
					);
					
					wszIncFile[ uSize ] = 0;
					if(( m_uError = RunFileCore( wszIncFile )) != ERR_OK ){
						
						if( !m_szErrorMsg ) m_szErrorMsg = new WCHAR[ MSGBUF_SIZE ];
						swprintf( m_szErrorMsg, MSGBUF_SIZE, L"Include file \"%s\" not found\n", wszIncFile );
						return m_uError;
					}
				}
			}
		}
		
		// 0xA まで skip
		while( *p != '\0' && *p++ != 0xA );
	}
	
	Handle<Script> script = Script::Compile(
		String::New( szBuf ), String::New(( uint16_t *)szFileName )
	);
	
	delete [] szBuf;
	
	if( script.IsEmpty()){
		// Print errors that happened during compilation.
		return m_uError = ERR_SCRIPT;
	}
	
	// とりあえず初期化処理
	Handle<Value> result = script->Run();
	
	return m_uError = ERR_OK;
}

/*** function 名指定実行，引数なし ******************************************/

UINT CScript::Run( LPCWSTR szFunc, BOOL bNoFunc ){
	//CSemaphoreLock lock( m_pSemaphore );
	
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	HandleScope handle_scope;
	Context::Scope context_scope( m_Context );
	
	return RunArg( szFunc, 0, NULL, bNoFunc );
}

UINT CScript::Run_s( LPCWSTR szFunc, LPCWSTR str0, BOOL bNoFunc ){
	//CSemaphoreLock lock( m_pSemaphore );
	
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	HandleScope handle_scope;
	Context::Scope context_scope( m_Context );
	
	Handle<Value> Args[] = {
		str0 ? String::New(( uint16_t *)str0 ) : v8::Undefined()
	};
	return RunArg( szFunc, 1, Args, bNoFunc );
}

UINT CScript::Run_ss( LPCWSTR szFunc, LPCWSTR str0, LPCWSTR str1, BOOL bNoFunc ){
	//CSemaphoreLock lock( m_pSemaphore );
	
	v8::Isolate::Scope IsolateScope( m_pIsolate );
	HandleScope handle_scope;
	Context::Scope context_scope( m_Context );
	
	Handle<Value> Args[] = {
		str0 ? String::New(( uint16_t *)str0 ) : v8::Undefined(),
		str1 ? String::New(( uint16_t *)str1 ) : v8::Undefined()
	};
	return RunArg( szFunc, 2, Args, bNoFunc );
}

UINT CScript::RunArg( LPCWSTR szFunc, int iArgNum, Handle<Value> Args[], BOOL bNoFunc ){
	TryCatch try_catch;
	
	Local<Function> hFunction = Local<Function>::Cast( m_Context->Global()->Get( String::New(( uint16_t *)szFunc )));
	if( hFunction->IsUndefined()){
		if( bNoFunc ) return ERR_OK;
		if( !m_szErrorMsg ) m_szErrorMsg = new WCHAR[ MSGBUF_SIZE ];
		
		swprintf( m_szErrorMsg, MSGBUF_SIZE, L"Undefined function \"%s()\"\n", szFunc );
		return m_uError = ERR_SCRIPT;
	}
	Handle<Value> result = hFunction->Call( m_Context->Global(), iArgNum, Args );
	
	if( try_catch.HasCaught()){
		m_szErrorMsg = ReportException( m_szErrorMsg, try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	/*
	if( !result->IsUndefined()) {
		// If all went well and the result wasn't undefined then print
		// the returned value.
		return m_uError = result->Int32Value();
	}
	*/
	// ガーベッジコレクション?
	//while( !v8::V8::IdleNotification());
	//v8::V8::IdleNotification();
	
	return m_uError = ERR_OK;
}

/*** ログリード by JavaScript **********************************************/

BOOL LogReaderCallback( const char *szPath, const char *szFile, void *pParam ){
	if( !IsExt( szFile, "js" )) return TRUE;
	
	CScript &Script = *( CScript *)pParam;
	
	char szBuf[ MAX_PATH + 1 ];
	strcat( strcpy( szBuf, szPath ), szFile );
	
	LPWSTR pFile = NULL;
	Script.RunFile( StringNew( pFile, szBuf ));
	delete pFile;
	
	if( Script.m_uError ){
		// エラー
		Script.m_pVsd->DispErrorMessage( Script.GetErrorMessage());
		return FALSE;
	}
	
	return TRUE;
}

UINT CScript::InitLogReader( void ){
	Initialize();
	
	{
		v8::Isolate::Scope IsolateScope( m_pIsolate );
		v8::HandleScope handle_scope;
		v8::Context::Scope context_scope( m_Context );
		
		// log 用の global array 登録
		m_Context->Global()->Set( v8::String::New( "Log" ), v8::Array::New( 0 ));
		m_Context->Global()->Set( v8::String::New( "LogReaderInfo" ), v8::Array::New( 0 ));
	}
	
	// スクリプトロード
	char szBuf[ MAX_PATH + 1 ];
	strcpy( szBuf, m_pVsd->m_szPluginDirA );
	strcat( szBuf, LOG_READER_DIR "\\" );
	if( !ListTree( szBuf, "*", LogReaderCallback, this )){
		return ERR_FILE_NOT_FOUND;
	}
	
	Run( L"SortLogReaderInfo", TRUE );
	if( m_uError ){
		// エラー
		m_pVsd->DispErrorMessage( GetErrorMessage());
		return m_uError;
	}
	
	return ERR_OK;
}
