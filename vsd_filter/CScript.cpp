/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.cpp - JavaScript
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "dds_lib/dds_lib.h"
#include "../vsd/main.h"
#include "CScript.h"
#include "CVsdFont.h"
#include "CVsdLog.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "CVsdFile.h"
#include "error_code.h"
#include "ScriptIF.h"

using namespace v8;

/*** static �����o�i�G�L�t���j***********************************************/

CVsdFilter *CScript::m_Vsd;

LPCWSTR CScript::m_szErrorMsgID[] = {
	#define DEF_ERROR( id, msg )	L##msg,
	#include "def_error.h"
};

/*** Exception ���b�Z�[�W�\�� ***********************************************/

#define MSGBUF_SIZE	( 4 * 1024 )

void CScript::ReportException( TryCatch* try_catch ){
	HandleScope handle_scope;
	String::Value exception( try_catch->Exception());
	Handle<Message> message = try_catch->Message();
	
	if( !m_szErrorMsg ) m_szErrorMsg = new WCHAR[ MSGBUF_SIZE ];
	LPWSTR p = m_szErrorMsg;
	
	if ( message.IsEmpty()){
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		swprintf( p, MSGBUF_SIZE, L"%s\n", *exception );
		p = wcschr( p, '\0' );
	}else{
		// Print ( filename ):( line number ): ( message ).
		String::Value filename( message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		swprintf( p, MSGBUF_SIZE - ( p - m_szErrorMsg ), L"%s:%i: %s\n", *filename, linenum, *exception );
		p = wcschr( p, '\0' );
		// Print line of source code.
		String::Value sourceline( message->GetSourceLine());
		swprintf( p, MSGBUF_SIZE - ( p - m_szErrorMsg ), L"%s\n", *sourceline );
		p = wcschr( p, '\0' );
		// Print wavy underline ( GetUnderline is deprecated ).
		int start = message->GetStartColumn();
		for ( int i = 0; i < start; i++ ){
			*p++ = L' ';
		}
		int end = message->GetEndColumn();
		for ( int i = start; i < end; i++ ){
			*p++ = L'^';
		}
		*p++ = L'\n';
		*p = L'\0';
		
		/*
		String::Utf8Value stack_trace( try_catch->StackTrace());
		if ( stack_trace.length() > 0 ){
			const char* stack_trace_string = ToCString( stack_trace );
			sprintf( p, "%s\n", stack_trace_string );
			p = wcschr( p, '\0' );
		}
		*/
	}
}

/*** �R���X�g���N�^ *********************************************************/

CScript::CScript( CVsdFilter *pVsd ){
	DebugMsgD( ":CScript::CScript():%X\n", GetCurrentThreadId());
	
	#ifdef AVS_PLUGIN
		m_pIsolate = v8::Isolate::New();
		v8::Isolate::Scope IsolateScope( m_pIsolate );
	#endif
	
	DebugMsgD( ":CScript::CScript():m_pIsolate = %X\n", m_pIsolate );
	
	DebugMsgD( ":CScript::CScript():m_Context\n" );
	m_Context.Clear();
	
	if( pVsd ) m_Vsd = pVsd;
	
	m_szErrorMsg	= NULL;
	m_uError		= ERR_OK;
}

/*** �f�X�g���N�^ ***********************************************************/

CScript::~CScript(){
	DebugMsgD( ":CScript::~CScript():%X\n", GetCurrentThreadId());
	DebugMsgD( ":CScript::~CScript():m_pIsolate = %X\n", m_pIsolate );
	
	{
		#ifdef AVS_PLUGIN
			v8::Isolate::Scope IsolateScope( m_pIsolate );
		#endif
		m_Context.Dispose();
		while( !v8::V8::IdleNotification());
	}
	#ifdef AVS_PLUGIN
		m_pIsolate->Dispose();
	#endif
	
	delete [] m_szErrorMsg;
}

/*** �f�o�b�O�p *************************************************************/

static v8::Handle<v8::Value> Func_DebugPrintD( const v8::Arguments& args ){
	v8::String::AsciiValue str( args[ 0 ] );
	DebugMsgD( "%s\n", *str );
	return v8::Undefined();
}

static v8::Handle<v8::Value> Func_DebugPrintW( const v8::Arguments& args ){
	v8::String::Value str( args[ 0 ] );
	
	MessageBoxW(
		NULL, ( LPCWSTR )*str,
		L"VSD filter JavaScript message",
		MB_OK
	);
	return v8::Undefined();
}

/*** JavaScript interface �̃Z�b�g�A�b�v ************************************/

void CScript::Initialize( void ){
	#ifdef AVS_PLUGIN
		v8::Isolate::Scope IsolateScope( m_pIsolate );
	#endif
	
	// ����
	HandleScope handle_scope;
	
	// �O���[�o���I�u�W�F�N�g�̐���
	Handle<ObjectTemplate> global = ObjectTemplate::New();
	
	// Image �N���X�o�^
	CVsdImageIF::InitializeClass( global );
	CVsdFontIF::InitializeClass( global );
	CVsdFilterIF::InitializeClass( global );
	CVsdFileIF::InitializeClass( global );
	
	// �f�o�b�O�p�� DebugPrint* �o�^
	global->Set( v8::String::New( "DebugPrintD" ),  v8::FunctionTemplate::New( Func_DebugPrintD ));
	global->Set( v8::String::New( "DebugPrintW" ),  v8::FunctionTemplate::New( Func_DebugPrintW ));
	
	// �O���[�o���I�u�W�F�N�g������𐶐�
	m_Context = Context::New( NULL, global );
}

/*** �X�N���v�g�t�@�C���̎��s ***********************************************/

UINT CScript::RunFile( LPCWSTR szFileName ){
	#ifdef AVS_PLUGIN
		v8::Isolate::Scope IsolateScope( m_pIsolate );
	#endif
	
	HandleScope handle_scope;
	
	// ������X�R�[�v�𐶐�
	Context::Scope context_scope( m_Context );
	
	TryCatch try_catch;
	
	// PluginDir �� cd
	TCHAR	szCurDir[ MAX_PATH ];
	getcwd( szCurDir, MAX_PATH );	// �J�����g dir
	chdir( m_Vsd->m_szPluginDirA );
	
	// �X�N���v�g ���[�h
	FILE *fp;
	fp = _wfopen( szFileName, L"r" );
	
	chdir( szCurDir );	// pwd �����ɖ߂�
	if( fp == NULL ) return m_uError = ERR_FILE_NOT_FOUND;
	
	// �t�@�C���T�C�Y�擾
	fseek( fp, 0, SEEK_END );
	fpos_t Size;
	fgetpos( fp, &Size );
	fseek( fp, 0, SEEK_SET );
	
	char *szBuf = new char[ ( UINT )Size + 1 ];
	
	int iReadSize = fread( szBuf, 1, ( size_t )Size, fp );
	fclose( fp );
	szBuf[ iReadSize ] = '\0';
	
	Handle<Script> script = Script::Compile(
		String::New( szBuf ), String::New(( uint16_t *)szFileName )
	);
	
	delete [] szBuf;
	
	if( script.IsEmpty()){
		// Print errors that happened during compilation.
		ReportException( &try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	// �Ƃ肠��������������
	Handle<Value> result = script->Run();
	
	if( result.IsEmpty()){
		assert( try_catch.HasCaught());
		// Print errors that happened during execution.
		ReportException( &try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	assert( !try_catch.HasCaught());
	/*
	if( !result->IsUndefined()) {
		// If all went well and the result wasn't undefined then print
		// the returned value.
		return m_uError = result->Int32Value();
	}
	*/
	// �K�[�x�b�W�R���N�V����?
	//while( !v8::V8::IdleNotification());
	
	return m_uError = ERR_OK;
}

/*** function ���w����s�C�����Ȃ� ******************************************/

UINT CScript::Run( LPCWSTR szFunc ){
	#ifdef AVS_PLUGIN
		v8::Isolate::Scope IsolateScope( m_pIsolate );
	#endif
	HandleScope handle_scope;
	Context::Scope context_scope( m_Context );
	
	return RunArg( szFunc, 0, NULL );
}

UINT CScript::Run_s( LPCWSTR szFunc, LPCWSTR str0 ){
	#ifdef AVS_PLUGIN
		v8::Isolate::Scope IsolateScope( m_pIsolate );
	#endif
	HandleScope handle_scope;
	Context::Scope context_scope( m_Context );
	
	Handle<Value> Args[] = { String::New(( uint16_t *)str0 ) };
	return RunArg( szFunc, 1, Args );
}

UINT CScript::RunArg( LPCWSTR szFunc, int iArgNum, Handle<Value> Args[] ){
	TryCatch try_catch;
	
	Local<Function> hFunction = Local<Function>::Cast( m_Context->Global()->Get( String::New(( uint16_t *)szFunc )));
	if( hFunction->IsUndefined()){
		
		if( !m_szErrorMsg ) m_szErrorMsg = new WCHAR[ MSGBUF_SIZE ];
		
		swprintf( m_szErrorMsg, MSGBUF_SIZE, L"Undefined function \"%s()\"", szFunc );
		return m_uError = ERR_SCRIPT;
	}
	Handle<Value> result = hFunction->Call( hFunction, iArgNum, Args );
	
	if( result.IsEmpty()){
		//assert( try_catch.HasCaught());
		try_catch.HasCaught();
		// Print errors that happened during execution.
		ReportException( &try_catch );
		return m_uError = ERR_SCRIPT;
	}
	
	assert( !try_catch.HasCaught());
	/*
	if( !result->IsUndefined()) {
		// If all went well and the result wasn't undefined then print
		// the returned value.
		return m_uError = result->Int32Value();
	}
	*/
	// �K�[�x�b�W�R���N�V����?
	//while( !v8::V8::IdleNotification());
	//v8::V8::IdleNotification();
	
	return m_uError = ERR_OK;
}
