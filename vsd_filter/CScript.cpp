/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CScript.cpp - JavaScript
	
*****************************************************************************/

#include <v8.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "CScript.h"
#include "CVsdFont.h"
#include "CVsdLog.h"
#include "pixel.h"
#include "CVsdImage.h"
#include "CVsdFilter.h"
#include "dds.h"

using namespace v8;

// �֐��I�u�W�F�N�g print �̎��� 
Handle<Value> Func_print(const Arguments& args) {
  String::AsciiValue str(args[0]);
  DebugMsgD("%s\n", *str);
  return Undefined();
}

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

/*** �}�N�� *****************************************************************/

#define CheckArgs( cond ) \
	if( !( cond )){ \
		return v8::ThrowException( v8::Exception::SyntaxError( v8::String::New( \
			"invalid number of args" \
		))); \
	}

/*** �R�[���o�b�N�֐� *******************************************************/

Handle<Value> Func_DrawLine( const Arguments& args ){
	
	int iLen = args.Length();
	CheckArgs( iLen == 5 || iLen == 6 );
	
	PIXEL_YCA yc; Color2YCA( yc, args[ 4 ]->Int32Value());
	
	CScript::m_Vsd->DrawLine(
		args[ 0 ]->Int32Value(), // x1
		args[ 1 ]->Int32Value(), // y1
		args[ 2 ]->Int32Value(), // x2
		args[ 3 ]->Int32Value(), // y2
		iLen <= 5 ? 1 : args[ 5 ]->Int32Value(), // width
		yc, 0
	);
	
	return Undefined();
}

/*** �R���X�g���N�^ *********************************************************/

CScript::CScript( CVsdFilter *pVsd ){
	m_context.Clear();
	m_script.Clear();
	
	m_Vsd = pVsd;
}

/*** �f�X�g���N�^ ***********************************************************/

CScript::~CScript(){
	m_context.Dispose();
}

/*** ���[�h�E�R���p�C�� *****************************************************/

#define SCRIPT_SIZE	( 64 * 1024 )

BOOL CScript::Load( char *szFileName ){
	// ����
	HandleScope handle_scope;
	
	// �O���[�o���I�u�W�F�N�g�̐���
	Handle<ObjectTemplate> global = ObjectTemplate::New();
	
	// �֐��I�u�W�F�N�g�̒�`
	#define DEF_SCR_FUNC( name ) \
		global->Set( \
			String::New( #name ), \
			FunctionTemplate::New( Func_ ## name ) \
		);
	#include "def_scr_func.h"
	
	// �O���[�o���I�u�W�F�N�g������𐶐�
	m_context = Context::New( NULL, global );
	
	TryCatch try_catch;
	
	// �_�~�[�̃X�R�[�v�𐶐�  Script::New ����Ƃ���
	// ���炩�̃R���e�L�X�g�� Enter ���Ă����Ȃ���΂Ȃ�Ȃ��炵��
	Context::Scope context_scope( m_context );
	
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
	
	Handle<Script> script = Script::New(
		ScriptBody, String::New( szFileName )
	);
	if( script.IsEmpty()){
		// Print errors that happened during compilation.
		ReportException( &try_catch );
		return false;
	}
	
	m_script = handle_scope.Close( script );
	return TRUE;
}

/*** Run ********************************************************************/

BOOL CScript::Run( void ){
	HandleScope handle_scope;
	TryCatch try_catch;
	
	// ������X�R�[�v�𐶐�
	Context::Scope context_scope( m_context );
	
	Handle<Value> result = m_script->Run();
	
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
