/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.cpp - JavaScript
	
*****************************************************************************/

#include "StdAfx.h"

#include "dds.h"
#include "CScript.h"
#include "CVsdFont.h"
#include "CVsdLog.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "ScriptIF.h"

using namespace v8;

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void ReportException(TryCatch* try_catch) {
  HandleScope handle_scope;
  String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  Handle<Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    DebugMsgD("%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    DebugMsgD("%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    DebugMsgD("%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      DebugMsgD(" ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      DebugMsgD("^");
    }
    DebugMsgD("\n");
    String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      DebugMsgD("%s\n", stack_trace_string);
    }
  }
}

/*** static �����o�i�G�L�t���j***********************************************/

CVsdFilter *CScript::m_Vsd;

/*** �R���X�g���N�^ *********************************************************/

CScript::CScript( CVsdFilter *pVsd ){
	m_context.Clear();
	m_Vsd = pVsd;
}

/*** �f�X�g���N�^ ***********************************************************/

CScript::~CScript(){
	m_context.Dispose();
}

/*** ���[�h�E�R���p�C�� *****************************************************/

#define SCRIPT_SIZE	( 64 * 1024 )

BOOL CScript::Initialize( char *szFileName ){
	// ����
	HandleScope handle_scope;
	
	// �O���[�o���I�u�W�F�N�g�̐���
	Handle<ObjectTemplate> global = ObjectTemplate::New();
	
	// Image �N���X�o�^
	CVsdImageIF::InitializeClass( global );
	CVsdFontIF::InitializeClass( global );
	CVsdFilterIF::InitializeClass( global );
	
	// �O���[�o���I�u�W�F�N�g������𐶐�
	m_context = Context::New( NULL, global );
	
	TryCatch try_catch;
	
	char *szBuf = new char[ SCRIPT_SIZE ];
	
	// �X�N���v�g ���[�h
	FILE *fp;
	if(( fp = fopen( szFileName, "r" )) == NULL ){
		// �G���[����
		return FALSE;
	}
	
	int iReadSize = fread( szBuf, 1, SCRIPT_SIZE, fp );
	fclose( fp );
	szBuf[ iReadSize ] = '\0';
	
	Handle<String> ScriptBody = String::New( szBuf );
	delete [] szBuf;
	
	// ������X�R�[�v�𐶐�
	Context::Scope context_scope( m_context );
	
	Handle<Script> script = Script::Compile(
		ScriptBody, String::New( szFileName )
	);
	
	if( script.IsEmpty()){
		// Print errors that happened during compilation.
		ReportException( &try_catch );
		return false;
	}
	
	// �Ƃ肠��������������
	Handle<Value> result = script->Run();
	
	if( result.IsEmpty()){
		assert( try_catch.HasCaught());
		// Print errors that happened during execution.
		ReportException( &try_catch );
		return FALSE;
	}else{
		assert( !try_catch.HasCaught());
		if( !result->IsUndefined()) {
			// If all went well and the result wasn't undefined then print
			// the returned value.
			String::Utf8Value str( result );
			const char* cstr = ToCString( str );
			DebugMsgD( "%s\n", cstr );
		}
		return TRUE;
	}
	
	return TRUE;
}

/*** function ���w����s�C�����Ȃ� ******************************************/

BOOL CScript::Run( const char *szFunc ){
	HandleScope handle_scope;
	Context::Scope context_scope( m_context );
	
	Local<Function> hFunction = Local<Function>::Cast( m_context->Global()->Get( String::New( szFunc )));
	Handle<Value> result = hFunction->Call( hFunction, 0, 0 );
	return TRUE;
}
