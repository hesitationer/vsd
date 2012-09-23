@echo off
set perlscr=%0 %*
set perlscr=%perlscr:\=/%
C:\cygwin\bin\bash --login -i -c 'cd "%CD%";CYGWIN=nodosfilewarning perl -x %perlscr%'
goto :EOF

##############################################################################
#!/usr/bin/perl -w
# .tab=4

$ENV{ 'PATH' } = "$ENV{ 'HOME' }/bin:" . $ENV{ 'PATH' };

open( fpOut, "| nkf -s > ScriptIF.h" );

MakeJsIF( 'CVsdFilter', '__VSD_System__', << '-----', << '-----' );
		CVsdFilter* obj = CScript::m_pVsd;
-----
	/*** DrawArc ****************************************************************/
	
	static v8::Handle<v8::Value> Func_DrawArc( const v8::Arguments& args ){
		int iLen = args.Length();
		if( CheckArgs( 7 <= iLen && iLen <= 9 )) return v8::Undefined();
		
		CVsdFilter *thisObj = GetThis<CVsdFilter>( args.This());
		if( !thisObj ) return v8::Undefined();
		
		if( iLen >= 9 ){
			thisObj->DrawArc(
				args[ 0 ]->Int32Value(),
				args[ 1 ]->Int32Value(),
				args[ 2 ]->Int32Value(),
				args[ 3 ]->Int32Value(),
				args[ 4 ]->Int32Value(),
				args[ 5 ]->Int32Value(),
				args[ 6 ]->NumberValue(),
				args[ 7 ]->NumberValue(),
				PIXEL_RABY::Argb2Raby( args[ 8 ]->Int32Value())
			);
		}else{
			thisObj->DrawArc(
				args[ 0 ]->Int32Value(),
				args[ 1 ]->Int32Value(),
				args[ 2 ]->Int32Value(),
				args[ 3 ]->Int32Value(),
				args[ 4 ]->NumberValue(),
				args[ 5 ]->NumberValue(),
				PIXEL_RABY::Argb2Raby( args[ 6 ]->Int32Value()),
				iLen <= 7 ? 0 : args[ 7 ]->Int32Value()
			);
		}
		return v8::Undefined();
	}
	
	/*** �f�o�b�O�p *************************************************************/
	
	// �֐��I�u�W�F�N�g print �̎���
	static v8::Handle<v8::Value> Func_print(const v8::Arguments& args) {
		v8::String::AsciiValue str( args[ 0 ] );
		DebugMsgD( "%s\n", *str );
		return v8::Undefined();
	}
	
-----

MakeJsIF( 'CVsdImage', 'Image', << '-----', '' );
		// �����`�F�b�N
		if ( args.Length() <= 0 ) return v8::Undefined();
		
		CVsdImage* obj;
		
		// arg[ 0 ] �� Image �������ꍇ�C���̃R�s�[�����
		if( args[ 0 ]->IsObject()){
			v8::Local<v8::Object> Image0 = args[ 0 ]->ToObject();
			if( strcmp( *( v8::String::AsciiValue )( Image0->GetConstructorName()), "Image" ) == 0 ){
				CVsdImage *obj0 = GetThis<CVsdImage>( Image0 );
				if( !obj0 ) return v8::Undefined();
				
				obj = new CVsdImage( *obj0 );
			}else{
				return v8::ThrowException( v8::Exception::Error( v8::String::New(
					"arg[ 0 ] must be Image or string"
				)));
			}
		}else{
			// �t�@�C�����w��ŉ摜���[�h
			obj = new CVsdImage();
			v8::String::Value FileName( args[ 0 ] );
			
			if( obj->Load(( LPCWSTR )*FileName ) != ERR_OK ){
				delete obj;
				return v8::Undefined();
			}
		}
-----

MakeJsIF( 'CVsdFont', 'Font', << '-----', '' );
		// �����`�F�b�N
		if ( args.Length() < 2 ) return v8::Undefined();
		
		v8::String::Value FontName( args[ 0 ] );
		CVsdFont *obj = new CVsdFont(
			( LPCWSTR )*FontName,
			args[ 1 ]->Int32Value(),
			args.Length() <= 2 ? 0 : args[ 2 ]->Int32Value()
		);
-----

MakeJsIF( 'CVsdFile', 'File', << '-----', '' );
		CVsdFile *obj = new CVsdFile();
-----

sub MakeJsIF {
	my( $Class, $JsClass, $NewObject, $FunctionIF ) = @_;
	
	$Accessor	= '';
	$AccessorIF	= '';
	$Function	= '';
	
	$IfNotVsd = $Class eq 'CVsdFilter' ? '//' : '';
	
	open( fpIn,	"< $Class.h" );
	while( <fpIn> ){
		if( /!js_func\b/ ){
			
			# �֐���
			/([\w_]+)\s+\*?([\w_]+)\s*\(/;
			( $RetType, $FuncName ) = ( $1, $2 );
			
			$ArgNum = 0;
			$ArgMin = 0;
			@Args = ();
			@Defs = ();
			
			$Line = $_;
			
			# ����
			if( $Line !~ /\)(?:\s*=\s*0\s*)?;/ ){
				while( <fpIn> ){
					$Line .= $_;
					last if( $Line =~ /\)(?:\s*=\s*0\s*)?;/ );
				}
			}
			$_ = $Line;
			s/[\x0D]//g;
			s/^.*?\([^\)]*?\n// || s/^.*?\(//;
			s/[\n\s]*\).*$//s;
			s/,\s*/\n/g;
			s#\n\s*//# //#g;
			
			@Line = split( /\n/, $_ );
			
			foreach $_ ( @Line ){
				/(\S+)/;	# �^
				$Type = $1;
				
				$Default = /!default:(\S+)/ ? $1 : undef;
				$ArgPos  = /!arg:(\d+)/ ? $1 : $ArgNum;
				
				if( $Type =~ /^CVsd(.+)/ ){
					$_ = $1;
					
					$ArgPos_p1 = $ArgPos + 1;
					push( @Defs, "v8::Local<v8::Object> $_$ArgNum = args[ $ArgPos ]->ToObject();" );
					push( @Defs, "if( CheckClass( $_$ArgNum, \"$_\", \"arg[ $ArgPos_p1 ] must be $_\" )) return v8::Undefined();" );
					push( @Defs, "$Type *obj$ArgNum = GetThis<$Type>( $_$ArgNum );" );
					push( @Defs, "if( !obj$ArgNum ) return v8::Undefined();" );
					$Args[ $ArgNum ] = "*obj$ArgNum";
				}
				
				elsif( $Type eq 'char' ){
					# string �^
					push( @Defs, "v8::String::AsciiValue str$ArgNum( args[ $ArgPos ] );" );
					$Args[ $ArgNum ] = "*str$ArgNum";
				}
				
				elsif( $Type =~ /^LPC?WSTR$/ ){
					# WCHAR string �^
					push( @Defs, "v8::String::Value str$ArgNum( args[ $ArgPos ] );" );
					$Args[ $ArgNum ] = "( $Type )*str$ArgNum";
				}
				
				elsif( $Type eq 'double' ){
					if( defined( $Default )){
						$Args[ $ArgNum ] = "iLen <= $ArgPos ? $Default : args[ $ArgPos ]->NumberValue()";
						--$ArgMin;
					}else{
						$Args[ $ArgNum ] = "args[ $ArgPos ]->NumberValue()";
					}
				}
				
				elsif( $Type eq 'int' || $Type eq 'UINT' ){
					# int/UINT �^
					if( defined( $Default )){
						$Args[ $ArgNum ] = "iLen <= $ArgPos ? $Default : args[ $ArgPos ]->Int32Value()";
						--$ArgMin;
					}else{
						$Args[ $ArgNum ] = "args[ $ArgPos ]->Int32Value()";
					}
				}
				
				elsif( $Type eq 'tRABY' ){
					# (�E�́E)�����B!!
					if( defined( $Default )){
						$Args[ $ArgNum ] = "iLen <= $ArgPos ? $Default : PIXEL_RABY::Argb2Raby( args[ $ArgPos ]->Int32Value())";
						--$ArgMin;
					}else{
						$Args[ $ArgNum ] = "PIXEL_RABY::Argb2Raby( args[ $ArgPos ]->Int32Value())";
					}
				}
				
				elsif( $Type eq 'void' ){
					next;
				}
				
				#else{
				#	$Args[ $ArgNum ] = "????";
				#}
				++$ArgMin;
				++$ArgNum;
			}
			
			$Defs = join( "\n\t\t", @Defs );
			$Args = join( ",\n\t\t\t", @Args );
			$Args = "\n\t\t\t$Args\n\t\t" if( $Args ne '' );
			
			$Len = $ArgMin == $ArgNum ?
				"iLen == $ArgNum" :
				"$ArgMin <= iLen && iLen <= $ArgNum";
			
			# �Ԃ�l
			if( $RetType eq 'void' ){
				$RetVar   = '';
				$RetValue = 'v8::Undefined()';
			}
			
			elsif( $RetType eq 'int' || $RetType eq 'UINT' ){
				$RetVar   = "int ret = ";
				$RetValue = "v8::Integer::New( ret )"
			}
			
			elsif( $RetType eq 'char' ){
				$RetVar   = "char *ret = ";
				$RetValue = "v8::String::New( ret )"
			}
			
			elsif( $RetType =~ /^LPC?WSTR$/ ){
				$RetVar   = "$RetType ret = ";
				$RetValue = "v8::String::New(( uint16_t *)ret )"
			}
			
			elsif( $RetType eq 'double' ){
				$RetVar   = "double ret = ";
				$RetValue = "v8::Number::New( ret )"
			}
			
			else{
				$RetVar   = '??? = ';
				$RetValue = '???';
			}
#-----
			$FunctionIF .= << "-----";
	static v8::Handle<v8::Value> Func_$FuncName( const v8::Arguments& args ){
		int iLen = args.Length();
		if( CheckArgs( $Len )) return v8::Undefined();
		$Defs
		$Class *thisObj = GetThis<$Class>( args.This());
		if( !thisObj ) return v8::Undefined();
		${RetVar}thisObj->$FuncName($Args);
		
		return $RetValue;
	}
-----
		}
		
		elsif( /!js_var:(\w+)/ ){
			$JSvar = $1;
			
			s/[\x0D\x0A]//g;
			s/\s*[{=;].*//;
			s/\(.*\)/()/;
			/(\w+\W*)$/;
			
			$RealVar = $1;
			
			$Type =
				/\b(?:int|UINT)\b/	? "Integer" :
				/\bdouble\b/		? "Number" :
				/\bchar\b/			? "String" :
				/\bLPC?WSTR\b/		? "String" :
									  "???";
			
			$Cast = '';
			if( /\bLPC?WSTR\b/ ){
				$Cast	= '( uint16_t *)';
			}
#-----
			$AccessorIF .= << "-----";
	static v8::Handle<v8::Value> Get_$JSvar( v8::Local<v8::String> propertyName, const v8::AccessorInfo& info ){
		$Class *obj = GetThis<$Class>( info.Holder());
		return obj ? v8::${Type}::New($Cast obj->$RealVar ) : v8::Undefined();
	}
-----
		}
	}
	close( fpIn );
	
	$AccessorIF =~ s/Get_(\w+)/AddAccessor( $1, $Class )/ge;
	$FunctionIF =~ s/Func_(\w+)/AddFunction( $1, $Class )/ge;
	
	print fpOut << "-----";
/****************************************************************************/

class ${Class}IF {
  private:
	// �N���X�R���X�g���N�^
	static v8::Handle<v8::Value> New( const v8::Arguments& args ){
		
$NewObject
		// internal field �Ƀo�b�N�G���h�I�u�W�F�N�g��ݒ�
		v8::Local<v8::Object> thisObject = args.This();
		thisObject->SetInternalField( 0, v8::External::New( obj ));
		
		// JS �I�u�W�F�N�g�� GC �����Ƃ��Ƀf�X�g���N�^���Ă΂�邨�܂��Ȃ�
		v8::Persistent<v8::Object> objectHolder = v8::Persistent<v8::Object>::New( thisObject );
		objectHolder.MakeWeak( obj, Dispose );
		
		#ifdef DEBUG
			DebugMsgD( ">>>new js obj $Class:%d:%X\\n", ++m_iCnt, obj );
		#endif
		// �R���X�g���N�^�� this ��Ԃ����ƁB
		return thisObject;
	}
	
	// �N���X�f�X�g���N�^
	static void Dispose( v8::Persistent<v8::Value> handle, void* pVoid ){
	$IfNotVsd	delete static_cast<$Class*>( pVoid );
		#ifdef DEBUG
			DebugMsgD( "<<<del js obj $Class:%d:%X\\n", m_iCnt--, pVoid );
		#endif
		handle.Dispose();
	}
	
	///// �v���p�e�B�A�N�Z�T /////
$AccessorIF
	///// ���\�b�h�R�[���o�b�N /////
$FunctionIF
  public:
	// this �ւ̃A�N�Z�X�w���p
	template<typename T>
	static T* GetThis( v8::Local<v8::Object> handle ){
		if( handle->GetInternalField( 0 )->IsUndefined()){
			v8::ThrowException( v8::Exception::TypeError( v8::String::New( "Invalid object ( maybe \\"new\\" failed )" )));
			return NULL;
		}
		
		void* pThis = v8::Local<v8::External>::Cast( handle->GetInternalField( 0 ))->Value();
		return static_cast<T*>( pThis );
	}
	
	// �����̐��`�F�b�N
	static BOOL CheckArgs( BOOL cond ){
		if( !( cond )){
			v8::ThrowException( v8::Exception::Error( v8::String::New(
				"invalid number of args"
			)));
			return TRUE;
		}
		return FALSE;
	}
	
	static BOOL CheckClass( v8::Local<v8::Object> obj, char *name, char *msg ){
		if( strcmp( *( v8::String::AsciiValue )( obj->GetConstructorName()), name )){
			v8::ThrowException( v8::Exception::TypeError( v8::String::New( msg )));
			return TRUE;
		}
		return FALSE;
	}
	
	// �N���X�e���v���[�g�̏�����
	static void InitializeClass( v8::Handle<v8::ObjectTemplate> global ){
		// �R���X�g���N�^���쐬
		v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New( New );
		tmpl->SetClassName( v8::String::New( "$JsClass" ));
		
		// �t�B�[���h�Ȃǂ͂������
		v8::Handle<v8::ObjectTemplate> inst = tmpl->InstanceTemplate();
		inst->SetInternalFieldCount( 1 );
$Accessor
		// ���\�b�h�͂������
		v8::Handle<v8::ObjectTemplate> proto = tmpl->PrototypeTemplate();
$Function
		// �O���[�o���I�u�W�F�N�g�ɃN���X���`
		global->Set( v8::String::New( "$JsClass" ), tmpl );
	}
	
	#ifdef DEBUG
	static int m_iCnt;
	#endif
};
#ifdef DEBUG
int ${Class}IF::m_iCnt = 0;
#endif
-----
}

##############################################################################

sub AddAccessor {
	my( $Name, $Class )= @_;
	$Accessor .= << "-----";
		inst->SetAccessor( v8::String::New( "$Name" ), Get_$Name );
-----
	return "Get_$Name";
}

sub AddFunction {
	my( $Name, $Class )= @_;
	$Function .= << "-----";
		proto->Set( v8::String::New( "$Name" ), v8::FunctionTemplate::New( Func_$Name ));
-----
	return "Func_$Name";
}
