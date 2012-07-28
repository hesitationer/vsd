/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFont.h - CVsdFont class header
	
*****************************************************************************/

#ifndef _CVsdFont_h_
#define _CVsdFont_h_

/*** new type ***************************************************************/

typedef struct {
	BYTE	*pBuf;
	int		iW, iH;
	int		iOrgY;
} tFontGlyph;

class CVsdFont {
  public:
	CVsdFont( LOGFONT &logfont );
	CVsdFont( const char *szFontName, int iSize, int iAttr );
	~CVsdFont();
	
	void CreateFont( LOGFONT &logfont );
	
	int GetW( void ){ return m_iFontW; }
	int GetH( void ){ return m_iFontH; }
	
	int	m_iFontW, m_iFontH;
	
	tFontGlyph *m_FontGlyph;
	
	// �t�H���g
	DWORD GetPix( UCHAR c, int x, int y ){
		int	i = c - ' ';
		return 0;
	}
	
	enum {
		ATTR_BOLD		= 1 << 0,
		ATTR_ITALIC		= 1 << 1,
		ATTR_OUTLINE	= 1 << 2
	};
	
	/*** JavaScript interface ***********************************************/
	
  private:
	// �N���X�R���X�g���N�^
	static v8::Handle<v8::Value> New( const v8::Arguments& args ){
		// �����`�F�b�N
		if ( args.Length() < 2 ) return v8::Undefined();
		
		v8::String::AsciiValue FontName( args[ 0 ] );
		CVsdFont *backend = new CVsdFont(
			*FontName,
			args[ 1 ]->Int32Value(),
			args.Length() <= 2 ? 0 : args[ 2 ]->Int32Value()
		);
		
		// internal field �Ƀo�b�N�G���h�I�u�W�F�N�g��ݒ�
		v8::Local<v8::Object> thisObject = args.This();
		thisObject->SetInternalField( 0, v8::External::New( backend ));
		
		// JS �I�u�W�F�N�g�� GC �����Ƃ��Ƀf�X�g���N�^���Ă΂�邨�܂��Ȃ�
		v8::Persistent<v8::Object> objectHolder = v8::Persistent<v8::Object>::New( thisObject );
		objectHolder.MakeWeak( backend, CVsdFont::Dispose );
		
		// �R���X�g���N�^�� this ��Ԃ����ƁB
		return thisObject;
	}
	
	// �N���X�f�X�g���N�^
	static void Dispose( v8::Persistent<v8::Value> handle, void* pVoid ){
		delete static_cast<CVsdFont*>( pVoid );
	}
	
	///// �v���p�e�B�A�N�Z�T /////
	
	#define DEF_SCR_VAR( name, var ) \
		static v8::Handle<v8::Value> Get_ ## name( \
			v8::Local<v8::String> propertyName, \
			const v8::AccessorInfo& info \
		){ \
			 const CVsdFont* backend = GetThis( info.Holder()); \
			 return v8::Integer::New( backend->var ); \
		}
	#include "def_font_var.h"
	
	///// ���\�b�h�R�[���o�b�N /////
	
	/*
	static v8::Local<v8::Value> Add( const v8::Arguments& args ){
		CVsdFont* backend = GetThis( args.This());
		if ( args.Length() > 0 ){
			backend->Add( args[0]->Int32Value());
		}else{
			backend->Add();
		}
		return v8::Undefined();
	}
	*/
	
  public:
	// this �ւ̃A�N�Z�X�w���p
	static CVsdFont* GetThis( v8::Local<v8::Object> handle ){
		 void* pThis = v8::Local<v8::External>::Cast( handle->GetInternalField( 0 ))->Value();
		 return static_cast<CVsdFont*>( pThis );
	}
	
	// �N���X�e���v���[�g�̏�����
	static void InitializeClass( v8::Handle<v8::ObjectTemplate> global ){
		// �R���X�g���N�^���쐬
		v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New( CVsdFont::New );
		tmpl->SetClassName( v8::String::New( "Font" ));
		
		// �t�B�[���h�Ȃǂ͂������
		v8::Handle<v8::ObjectTemplate> inst = tmpl->InstanceTemplate();
		inst->SetInternalFieldCount( 1 );
		#define DEF_SCR_VAR( name, var ) \
			inst->SetAccessor( v8::String::New( #name ), CVsdFont::Get_ ## name );
		#include "def_font_var.h"
		
		// ���\�b�h�͂������
		//v8::Local<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
		//proto->Set( v8::String::New( "add" ), FunctionTemplate::New( CVsdFont::Add ));
		
		// �O���[�o���I�u�W�F�N�g�ɃN���X���`
		global->Set( v8::String::New( "Font" ), tmpl );
	}
};
#endif
