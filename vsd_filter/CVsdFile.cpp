/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFile.cpp - CVsdFile class implementation
	
*****************************************************************************/

#include "StdAfx.h"
#include "CVsdFile.h"
#include "CScript.h"

#ifdef PUBLIC_MODE
	BOOL IsFileWriteEnabled( void );
#endif

#define V8ErrorClosedHandle()		V8Error( "operation not permitted on closed handle" )
#define V8ErrorZipNotSupported()	V8Error( "operation not supported on zip-mode" )
#define V8ErrorZipNotOpened()		V8Error( "file in zip not opened, execute ZipNextFile() first." )

/*** �t�@�C���I�[�v�� *******************************************************/

int CVsdFile::Open( LPCWSTR szFile, LPCWSTR szMode ){
	
	LPCWSTR pMode = szMode;
	
	#ifdef PUBLIC_MODE
		// �t�@�C�����C�g�s�����͎��s�R�[�h��Ԃ�
		if(
			wcschr( szMode, L'w' ) != NULL &&
			!IsFileWriteEnabled()
		){
			V8Error( "write operation not permitted by user" );
			return -1;
		}
	#endif
	
	// gz open
	if( *pMode == 'z' ){
		++pMode;
		char *pFile = NULL;
		char *pMode = NULL;
		
		StringNew( pFile, szFile );
		StringNew( pMode, szMode );
		
		m_gzfp = gzopen( pFile, pMode );
		delete [] pFile;
		delete [] pMode;
		
		if( m_gzfp ) m_uMode = MODE_GZIP;
	}
	
	// zip open
	else if( *pMode == 'Z' ){
		char *pFile = NULL;
		
		StringNew( pFile, szFile );
		m_unzfp = unzOpen( pFile );
		delete [] pFile;
		
		if( m_unzfp ) m_uMode = MODE_ZIP;
	}
	
	// ���� fopen
	else{
		m_fp = _wfopen( szFile, szMode );
	}
	
	if( m_fp == NULL ) V8Error( ERR_CANT_OPEN_FILE );
	return 0;
}

/*** �t�@�C���N���[�Y *******************************************************/

void CVsdFile::Close( void ){
	if( m_fp ) switch( m_uMode ){
		case MODE_NORMAL:		fclose( m_fp );
		Case MODE_GZIP:			gzclose( m_gzfp );
		
		Case MODE_ZIP_OPENED:	unzCloseCurrentFile( m_unzfp );
		case MODE_ZIP:			unzClose( m_unzfp );
	}
	m_fp = NULL;
}

/*** ���C�����[�h ***********************************************************/

v8::Handle<v8::Value> CVsdFile::ReadLine( void ){
	*m_cBuf = '\0';
	
	if( !m_fp ){
		V8ErrorClosedHandle();
		return v8::Undefined();
	}
	
	switch( m_uMode ){
		case MODE_NORMAL:		fgets( m_cBuf, FILEBUF_LEN, m_fp );
		Case MODE_GZIP:			gzgets( m_gzfp, m_cBuf, FILEBUF_LEN );
		
		Case MODE_ZIP: {
			V8ErrorZipNotOpened();
			return v8::Undefined();
		}
		
		Case MODE_ZIP_OPENED: {
			
			// \n �T�[�`
			char *p = ( char *)memchr( m_cBuf + m_uBufPtr, '\n', m_uBufSize - m_uBufPtr );
			
			// ������Ȃ��̂ŁCbuf read ����
			if( !p ){
				ReadZip();
				p = ( char *)memchr( m_cBuf + m_uBufPtr, '\n', m_uBufSize - m_uBufPtr );
				
				// �Ăь�����Ȃ������̂ŁCp �� buf �I�[���w��
				if( !p ) p = m_cBuf + m_uBufSize - 1;
			}
			
			// �����񒷕t���� v8::String �ϊ�
			int iStr = m_uBufPtr;
			m_uBufPtr = ( p - m_cBuf + 1 );
			return v8::String::New( m_cBuf + iStr, m_uBufPtr - iStr );
		}
	}
	return v8::String::New( m_cBuf );
}

/*** ���C�����C�g ***********************************************************/

int CVsdFile::WriteLine( char *str ){
	
	if( m_fp ) switch( m_uMode ){
		case MODE_NORMAL:		return fputs( str, m_fp );
		case MODE_GZIP:			return gzputs( m_gzfp, str );
		
		case MODE_ZIP:
		case MODE_ZIP_OPENED:	V8ErrorZipNotSupported();
	}
	else V8ErrorClosedHandle();
	
	return 0;
}

/*** seek *******************************************************************/

int CVsdFile::Seek( int iOffs, int iOrg ){
	
	if( m_fp ) switch( m_uMode ){
		case MODE_NORMAL:		return fseek( m_fp, iOffs, iOrg );
		case MODE_GZIP:			return gzseek( m_gzfp, iOffs, iOrg ) >= 0 ? 0 : -1;
		
		case MODE_ZIP:
		case MODE_ZIP_OPENED:	V8ErrorZipNotSupported();
	}
	else V8ErrorClosedHandle();
	
	return 0;
}

/*** �o�C�i�����[�h *********************************************************/

UCHAR *CVsdFile::ReadBin( int iSize ){
	static UCHAR cBuf[ 8 ];
	
	*( int *)cBuf = *( int *)( cBuf + 4 ) = 0;
	
	if( m_fp )	switch( m_uMode ){
		case MODE_NORMAL:		fread( cBuf, 1, iSize, m_fp );
		Case MODE_GZIP:			gzread( m_gzfp, cBuf, iSize );
		Case MODE_ZIP:			V8ErrorZipNotOpened();
		Case MODE_ZIP_OPENED: {
			// buf �c�ʂ�����Ȃ�������C�ǂ�
			if( iSize > ( int )( m_uBufSize - m_uBufPtr )) ReadZip();
			if( iSize > ( int )( m_uBufSize - m_uBufPtr )) iSize = m_uBufSize - m_uBufPtr;
			
			// ���[�h�o�b�t�@�ɃR�s�[
			memcpy( cBuf, m_cBuf + m_uBufPtr, iSize );
			m_uBufPtr += iSize;
		}
	}
	return cBuf;
}

/*** �o�C�i�����C�g *********************************************************/

int CVsdFile::WriteBin( void *pBuf, int iSize ){
	if( m_fp ) switch( m_uMode ){
		case MODE_NORMAL:		return fwrite( pBuf, 1, iSize, m_fp );
		case MODE_GZIP:			return gzread( m_gzfp, pBuf, iSize );
		
		case MODE_ZIP:
		case MODE_ZIP_OPENED:	V8ErrorZipNotSupported();
	}
	else V8ErrorClosedHandle();
	return 0;
}

/*** eof �`�F�b�N ***********************************************************/

int CVsdFile::IsEOF( void ){
	if( m_fp ) switch( m_uMode ){
		case MODE_NORMAL:		return feof( m_fp );
		case MODE_GZIP:			return gzeof( m_gzfp );
		
		case MODE_ZIP:			V8ErrorZipNotSupported();
		
		case MODE_ZIP_OPENED: return
			!( m_uFlag & FLAG_EOF ) ? 0 :
			m_uBufPtr == m_uBufSize ? -1 : 0;
	}
	else V8ErrorClosedHandle();
	return 0;
}

/*** zip ���̃t�@�C�� *******************************************************/

v8::Handle<v8::Value> CVsdFile::ZipNextFile( void ){
	unz_file_info fileInfo;
	
	LPWSTR	wszFileName = NULL;
	
	// zip ����Ȃ���� ret
	if( !( MODE_ZIP <= m_uMode && m_uMode <= MODE_ZIP_OPENED )){
		V8Error( "not opened by zip-mode" );
		return v8::Undefined();
	}
	
	m_uFlag &= ~FLAG_EOF;	// eof �N���A
	
	// �t�@�C�����J���Ă�����C����
	if( m_uMode == MODE_ZIP_OPENED ){
		unzCloseCurrentFile( m_unzfp );
	}
	m_uMode = MODE_ZIP;
	
	while( 1 ){
		if( m_uFlag & FLAG_2ND_FILE ){
			// �t�@�C���S����������
			if( unzGoToNextFile( m_unzfp ) == UNZ_END_OF_LIST_OF_FILE ){
				return v8::Undefined();
			}
		}
		m_uFlag |= FLAG_2ND_FILE;
		
		// ���ɓ��t�@�C�����擾
		if(
			unzGetCurrentFileInfo(
				m_unzfp, &fileInfo,
				m_cBuf, FILEBUF_LEN - 1,
				NULL, 0, NULL, 0
			) != UNZ_OK
		){
			V8Error( "zlib: unzGetCurrentFileInfo failed (maybe wrong zip format?)" );
			return v8::Undefined();
		}
		
		// unicode �ϊ�
		StringNew( wszFileName, m_cBuf );
		
		// '/' �ŏI����Ă����� dir �Ȃ̂ŃX�L�b�v
		int iLen = wcslen( wszFileName ) - 1;
		if( iLen >= 0 && wszFileName[ iLen ] != '/' ) break;
	}
	
	// ���ɓ��t�@�C���I�[�v��
	if( unzOpenCurrentFile( m_unzfp ) == UNZ_OK ){
		m_uMode = MODE_ZIP_OPENED;
		
		v8::Local<v8::String> v8strFileName = v8::String::New(( uint16_t *)wszFileName );
		delete [] wszFileName;
		
		return v8strFileName;
	}
	
	V8Error( "zlib: can't open file in zip (maybe wrong zip format?)" );
	return v8::Undefined();
}

/*** zip �t�@�C���� body ���[�h *********************************************/

int CVsdFile::ReadZip( void ){
	
	// �c���Ă���f�[�^��z��擪�� move
	if( m_uBufPtr != 0 && m_uBufPtr != m_uBufSize ){
		memmove( m_cBuf, m_cBuf + m_uBufPtr, m_uBufSize - m_uBufPtr );
	}
	m_uBufSize -= m_uBufPtr;
	m_uBufPtr = 0;
	
	// ���[�h
	int iBufRemained = FILEBUF_LEN - m_uBufSize;
	int iReadSize = unzReadCurrentFile( m_unzfp, m_cBuf + m_uBufSize, iBufRemained );
	m_uBufSize += iReadSize;
	
	if( iReadSize < iBufRemained ) m_uFlag |= FLAG_EOF;
	
	return iReadSize;
}
