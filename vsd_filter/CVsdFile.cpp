/*****************************************************************************
	
	VSD -- vehicle data logger system  Copyright(C) by DDS
	
	CVsdFile.cpp - CVsdFile class implementation
	
*****************************************************************************/

#include "StdAfx.h"
#include "CVsdFile.h"

#ifdef PUBLIC_MODE
	BOOL IsFileWriteEnabled( void );
#endif

/*** �t�@�C���I�[�v�� *******************************************************/

int CVsdFile::Open( LPCWSTR szFile, LPCWSTR szMode ){
	
	LPCWSTR pMode = szMode;
	
	#ifdef PUBLIC_MODE
		// �t�@�C�����C�g�s�����͎��s�R�[�h��Ԃ�
		if(
			wcschr( szMode, L'w' ) != NULL &&
			!IsFileWriteEnabled()
		){
			return 1;
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
		return m_gzfp != NULL ? 0 : 1;
	}
	
	// ���� fopen
	m_fp = _wfopen( szFile, szMode );
	return m_fp != NULL ? 0 : 1;
}

/*** �t�@�C���N���[�Y *******************************************************/

void CVsdFile::Close( void ){
	if( m_gzfp ){
		gzclose( m_gzfp );
		m_gzfp = NULL;
	}
	
	if( m_fp ){
		fclose( m_fp );
		m_fp = NULL;
	}
}

/*** ���C�����[�h ***********************************************************/

char *CVsdFile::ReadLine( void ){
	*m_cBuf = '\0';
	
	if( m_gzfp ){
		gzgets( m_gzfp, m_cBuf, BUF_LEN );
	}else if( m_fp ){
		fgets( m_cBuf, BUF_LEN, m_fp );
	}

	return m_cBuf;
}

/*** ���C�����[�h ***********************************************************/

int CVsdFile::WriteLine( char *str ){
	
	if( m_gzfp ){
		return gzputs( m_gzfp, str );
	}else if( m_fp ){
		return fputs( str, m_fp );
	}

	return -1;
}

/*** seek *******************************************************************/

int CVsdFile::Seek( int iOffs, int iOrg ){
	if( m_gzfp ){
		return gzseek( m_gzfp, iOffs, iOrg ) >= 0 ? 0 : -1;
	}else if( m_fp ){
		return fseek( m_fp, iOffs, iOrg );
	}
	
	return -1;
}

/*** �o�C�i�����[�h *********************************************************/

UCHAR *CVsdFile::ReadBin( int iSize ){
	static UCHAR cBuf[ 8 ];
	
	*( int *)cBuf = *( int *)( cBuf + 4 ) = 0;
	
	if( m_gzfp ){
		gzread( m_gzfp, cBuf, iSize );
	}else if( m_fp ){
		fread( cBuf, 1, iSize, m_fp );
	}
	return cBuf;
}

/*** �o�C�i�����[�h *********************************************************/

int CVsdFile::WriteBin( void *pBuf, int iSize ){
	if( m_gzfp ){
		return gzread( m_gzfp, pBuf, iSize );
	}else if( m_fp ){
		return fwrite( pBuf, 1, iSize, m_fp );
	}
	return 0;
}

/*** eof �`�F�b�N ***********************************************************/

int CVsdFile::IsEOF( void ){
	if( m_gzfp ){
		return gzeof( m_gzfp );
	}else if( m_fp ){
		return feof( m_fp );
	}
	return 1;
}
